#include "stdafx.h"
#include "BanManager.h"
#include "config.h"
#include "constants.h"
#include "db.h"
#include "utils.h"
#include "p2p.h"
#include "char_manager.h"
#include "desc_manager.h"
#include "desc.h"
#include "char.h"
#include "ikarus_shop_manager.h"

BanManager::BanManager()
{
}

BanManager::~BanManager()
{
}

bool BanManager::Block(DWORD aid, DWORD pid, DWORD duration, DWORD who, std::string reason, bool instant_block)
{
	auto pkMsg = DBManager::instance().AccountQuery("SELECT account_id FROM account_block WHERE account_id=%d AND `status`=0", aid);
	SQLResult* pRes = pkMsg->Get();
	if (pRes->uiNumRows > 0)
		return false;

	auto szReason = reason.c_str();
	char buf[512 + 1];
	DBManager::instance().EscapeString(buf, sizeof(buf), szReason, std::strlen(szReason));

	DBManager::instance().AccountQuery("INSERT INTO account_block VALUES (%d, %d, %d, %d, '%s', %u, %s)",
		aid, duration, pid, who, buf, instant_block ? 1 : 0, instant_block ? "Now()" : "'0000-00-00 00:00'");

	if (instant_block)
	{
		BlockAccountQuery(aid, duration);
		if (pid > 0)
			SendDisconnect(pid);
	}
	return true;
}

void BanManager::BlockHWID(const char * hwid)
{
	BlockHwidQuery(hwid);
	SendDisconnectHwid(hwid);
}

void BanManager::BlockAccountQuery(DWORD aid, DWORD duration)
{
	if (duration >= 365 * 24 * 3600) // perm ban
	{
		DBManager::instance().AccountQuery("UPDATE account SET status='BLOCK' WHERE id=%d", aid);
	}
	else // temp ban
	{
		DBManager::instance().AccountQuery("UPDATE account SET availDt=FROM_UNIXTIME(%d) WHERE id=%d", get_global_time() + duration, aid);
	}
}

void BanManager::BlockHwidQuery(const char* hwid)
{
	char buf[HWID_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(buf, sizeof(buf), hwid, std::strlen(hwid));
	DBManager::instance().AccountQuery("REPLACE INTO hardware_status VALUES ('%s', 'BLOCK')", buf);
	DBManager::instance().AccountQuery("UPDATE account SET status='BLOCK' WHERE hwid='%s'", buf);
}

int BanManager::FlushBlock()
{
	std::unique_ptr<SQLMsg> pkMsg(DBManager::instance().AccountQuery(
"SELECT account_id, pid, ban_duration FROM account_block WHERE status=0"));

	SQLResult* pRes = pkMsg->Get();
	if (pRes->uiNumRows > 0)
	{
		MYSQL_ROW data;
		int col;
		while ((data = mysql_fetch_row(pRes->pSQLResult)))
		{
			col = 0;

			DWORD aid = 0;
			str_to_number(aid, data[col++]);

			DWORD pid = 0;
			str_to_number(pid, data[col++]);

			DWORD duration = 0;
			str_to_number(duration, data[col++]);

			if (aid > 0 && duration > 0)
			{
				BlockAccountQuery(aid, duration);

				if (pid > 0)
				{
					SendDisconnect(pid);
				}
			}
		}
		DBManager::instance().AccountQuery("UPDATE account_block SET status=1, flush_time=NOW() WHERE status=0");
	}
	return pRes->uiNumRows;
}

void BanManager::SendDisconnect(DWORD pid) // use only after ban
{
	LPCHARACTER foundPlayer = CHARACTER_MANAGER::instance().FindByPID(pid);
	if (foundPlayer && foundPlayer->GetDesc())
	{
		DESC_MANAGER::instance().DestroyLoginKey(foundPlayer->GetDesc());
		foundPlayer->GetDesc()->SetPhase(PHASE_CLOSE);
	}
	else if (auto pkCCI = P2P_MANAGER::instance().FindByPID(pid))
	{
		TPacketGGDisconnectPlayerId p;
		p.bHeader = HEADER_GG_DISCONNECT_PLAYER_ID;
		p.pid = pid;
		P2P_MANAGER::instance().Send(&p, sizeof(p));
	}

	CloseOfflineShop(pid);
}

void BanManager::SendDisconnectHwid(const char* hwid)
{
	DisconnectHwid(hwid);

	// send to other game cores
	TPacketGGDisconnectHwid p;
	p.bHeader = HEADER_GG_DISCONNECT_HWID;
	strncpy(p.hwid, hwid, sizeof(p.hwid));
	P2P_MANAGER::instance().Send(&p, sizeof(p));
}

void BanManager::DisconnectHwid(const char* hwid)
{
	for (auto it : CHARACTER_MANAGER::instance().GetPCMap())
	{
		LPCHARACTER ch = it.second;
		if (ch && ch->GetDesc())
		{
			LPDESC d = ch->GetDesc();
			if (strcmp(hwid, d->GetAccountTable().hwid))
				continue;

			sys_log(0, "DISCONNECT HWID PLAYER success pid %d ", ch->GetPlayerID());
			DESC_MANAGER::instance().DestroyLoginKey(d);
			d->SetPhase(PHASE_CLOSE);

			CloseOfflineShop(ch->GetPlayerID());
		}
	}
}

BanManager::BlockPlayerData BanManager::GetBlockPlayerData(DWORD pid)
{
	BlockPlayerData data{};
	data.pid = 0;

	auto pkMsg = DBManager::instance().DirectQuery("SELECT account_id, name FROM player WHERE id=%d", pid);
	if (pkMsg->Get()->uiNumRows > 0)
	{
		MYSQL_ROW row = mysql_fetch_row(pkMsg->Get()->pSQLResult);

		data.pid = pid;
		str_to_number(data.aid, row[0]);

		strncpy(data.name, row[1], sizeof(data.name));
	}
	return data;
}

BanManager::BlockHwidData BanManager::GetBlockHwidData(DWORD aid)
{
	BlockHwidData data{};

	auto pkMsg = DBManager::instance().AccountQuery("SELECT hwid FROM account WHERE id=%d", aid);
	if (pkMsg->Get()->uiNumRows > 0)
	{
		MYSQL_ROW row = mysql_fetch_row(pkMsg->Get()->pSQLResult);
		if (row[0] != nullptr)
			strncpy(data.hwid, row[0], sizeof(data.hwid));
	}
	return data;
}

bool BanManager::CloseOfflineShop(DWORD pid)
{
	auto pkShop = ikashop::GetManager().GetShopByOwnerID(pid);
	if (!pkShop) {
		return false;
	}
	else {
		ikashop::GetManager().SendShopForceSoftCloseDBPacket(pid, true);
		return true;
	}
}

// Files shared by GameCore.top
