#include "stdafx.h"
#include "constants.h"
#include "gm.h"
#include "messenger_manager.h"
#include "buffer_manager.h"
#include "desc_client.h"
#include "log.h"
#include "config.h"
#include "p2p.h"
#include "crc32.h"
#include "char.h"
#include "char_manager.h"
#include "questmanager.h"
#include "playerbot_manager.h"

// @fixme142 BEGIN
static char	__account[CHARACTER_NAME_MAX_LEN*2+1];
static char	__companion[CHARACTER_NAME_MAX_LEN*2+1];
// @fixme142 END

MessengerManager::MessengerManager()
{
}

MessengerManager::~MessengerManager()
{
}

void MessengerManager::Initialize()
{
}

bool MessengerManager::IsFriend(std::string account, std::string companion)
{
	auto it = m_Relation.find(account);
	if (it == m_Relation.end())
		return false;

	const auto& friendSet = it->second;
	return friendSet.find(companion) != friendSet.end();
}

DWORD MessengerManager::GetFriendCount(std::string account)
{
	auto it = m_Relation.find(account);
	if (it == m_Relation.end())
		return 0;

	return it->second.size();
}

void MessengerManager::Destroy()
{
}

void MessengerManager::P2PLogin(MessengerManager::keyA account)
{
	Login(account);
}

void MessengerManager::P2PLogout(MessengerManager::keyA account)
{
	Logout(account);
}

void MessengerManager::Login(MessengerManager::keyA account)
{
	if (m_set_loginAccount.find(account) != m_set_loginAccount.end())
		return;

	// @fixme142 BEGIN
	DBManager::instance().EscapeString(__account, sizeof(__account), account.c_str(), account.size());
	if (account.compare(__account))
		return;
	// @fixme142 END

	DBManager::instance().FuncQuery(msl::bind1st(std::mem_fn(&MessengerManager::LoadList), this),
		"SELECT account, companion FROM messenger_list%s WHERE account='%s'", get_table_postfix(), __account);

	m_set_loginAccount.emplace(account);
}

void MessengerManager::LoadList(SQLMsg * msg)
{
	if (NULL == msg)
		return;

	if (NULL == msg->Get())
		return;

	if (msg->Get()->uiNumRows == 0)
		return;

	std::string account;

	sys_log(1, "Messenger::LoadList");

	for (uint i = 0; i < msg->Get()->uiNumRows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

		if (row[0] && row[1])
		{
			if (account.length() == 0)
				account = row[0];

			m_Relation[row[0]].emplace(row[1]);
			m_InverseRelation[row[1]].emplace(row[0]);
		}
	}

	SendList(account);

	std::set<MessengerManager::keyT>::iterator it;

	for (it = m_InverseRelation[account].begin(); it != m_InverseRelation[account].end(); ++it)
		SendLogin(*it, account);
}

void MessengerManager::Logout(MessengerManager::keyA account)
{
	if (m_set_loginAccount.find(account) == m_set_loginAccount.end())
		return;

	m_set_loginAccount.erase(account);

	std::set<MessengerManager::keyT>::iterator it;

	for (it = m_InverseRelation[account].begin(); it != m_InverseRelation[account].end(); ++it)
	{
		SendLogout(*it, account);
	}

	std::map<keyT, std::set<keyT> >::iterator it2 = m_Relation.begin();

	while (it2 != m_Relation.end())
	{
		it2->second.erase(account);
		++it2;
	}

	m_Relation.erase(account);
}

void MessengerManager::RequestToAdd(LPCHARACTER ch, LPCHARACTER target)
{
	if (!ch || !target)
		return;

	if (!ch->IsPC() || !target->IsPC())
		return;

	if (GetFriendCount(ch->GetName()) >= MESSENGER_MAX_FRIEND_LIMIT)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MESSENGER_FRIEND_MAX_LIMIT%d"), MESSENGER_MAX_FRIEND_LIMIT);
		return;
	}

	if (IsBlock(ch->GetPlayerID(), target->GetName()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MESSENGER_ADD_IS_BLOCKED"));
		return;
	}

	if (IsFriend(ch->GetName(), target->GetName()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are already friends with %s."), target->GetName());
		return;
	}

	if (quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->IsRunning() == true)
	{
	    ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The recipient cannot receive friend invitation."));
	    return;
	}

	if (quest::CQuestManager::instance().GetPCForce(target->GetPlayerID())->IsRunning() == true)
		return;

	DWORD dw1 = GetCRC32(ch->GetName(), strlen(ch->GetName()));
	DWORD dw2 = GetCRC32(target->GetName(), strlen(target->GetName()));

	char buf[64];
	snprintf(buf, sizeof(buf), "%u:%u", dw1, dw2);
	DWORD dwComplex = GetCRC32(buf, strlen(buf));

	m_set_requestToAdd.emplace(dwComplex);

	target->ChatPacket(CHAT_TYPE_COMMAND, "messenger_auth %s", ch->GetName());

	// Companion mod: "messenger_auth" opens a dialog in the target's client, and
	// a Playerbot has none - so an owner adding their own companion waited for
	// an answer that was never coming. Answer for it, through the same call
	// do_messenger_auth makes: account is the side that was asked (the bot),
	// companion the side that asked (the owner), which is the order the request
	// key above was built in.
}

// @fixme130 void -> bool
bool MessengerManager::AuthToAdd(MessengerManager::keyA account, MessengerManager::keyA companion, bool bDeny)
{
	DWORD dw1 = GetCRC32(companion.c_str(), companion.length());
	DWORD dw2 = GetCRC32(account.c_str(), account.length());

	char buf[64];
	snprintf(buf, sizeof(buf), "%u:%u", dw1, dw2);
	DWORD dwComplex = GetCRC32(buf, strlen(buf));

	if (m_set_requestToAdd.find(dwComplex) == m_set_requestToAdd.end())
	{
		sys_log(0, "MessengerManager::AuthToAdd : request not exist %s -> %s", companion.c_str(), account.c_str());
		return false;
	}

	m_set_requestToAdd.erase(dwComplex);

	if (!bDeny)
	{
		AddToList(companion, account);
		AddToList(account, companion);
	}
	return true;
}

void MessengerManager::__AddToList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	m_Relation[account].emplace(companion);
	m_InverseRelation[companion].emplace(account);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (d)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Friends] You have added %s as a friend."), companion.c_str());
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(companion.c_str());

	if (tch)
		SendLogin(account, companion);
	else
		SendLogout(account, companion);
}

void MessengerManager::AddToList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	if (companion.size() == 0)
		return;

	if (m_Relation[account].find(companion) != m_Relation[account].end())
		return;

	// @fixme142 BEGIN
	DBManager::instance().EscapeString(__account, sizeof(__account), account.c_str(), account.size());
	DBManager::instance().EscapeString(__companion, sizeof(__companion), companion.c_str(), companion.size());
	if (account.compare(__account) || companion.compare(__companion))
		return;
	// @fixme142 END

	sys_log(0, "Messenger Add %s %s", account.c_str(), companion.c_str());
	DBManager::instance().Query("INSERT INTO messenger_list%s VALUES ('%s', '%s')",
			get_table_postfix(), __account, __companion);

	__AddToList(account, companion);

	TPacketGGMessenger p2ppck;

	p2ppck.bHeader = HEADER_GG_MESSENGER_ADD;
	strlcpy(p2ppck.szAccount, account.c_str(), sizeof(p2ppck.szAccount));
	strlcpy(p2ppck.szCompanion, companion.c_str(), sizeof(p2ppck.szCompanion));
	P2P_MANAGER::instance().Send(&p2ppck, sizeof(TPacketGGMessenger));
}

void MessengerManager::__RemoveFromList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	m_Relation[account].erase(companion);
	m_InverseRelation[companion].erase(account);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (d)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Friends] You have deleted %s from the list."), companion.c_str());
}

void MessengerManager::RemoveFromList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	if (companion.size() == 0)
		return;

	// @fixme142 BEGIN
	DBManager::instance().EscapeString(__account, sizeof(__account), account.c_str(), account.size());
	DBManager::instance().EscapeString(__companion, sizeof(__companion), companion.c_str(), companion.size());
	if (account.compare(__account) || companion.compare(__companion))
		return;
	// @fixme142 END

	sys_log(1, "Messenger Remove %s %s", account.c_str(), companion.c_str());
	DBManager::instance().Query("DELETE FROM messenger_list%s WHERE account='%s' AND companion = '%s'",
			get_table_postfix(), __account, __companion);

	__RemoveFromList(account, companion);

	TPacketGGMessenger p2ppck;

	p2ppck.bHeader = HEADER_GG_MESSENGER_REMOVE;
	strlcpy(p2ppck.szAccount, account.c_str(), sizeof(p2ppck.szAccount));
	strlcpy(p2ppck.szCompanion, companion.c_str(), sizeof(p2ppck.szCompanion));
	P2P_MANAGER::instance().Send(&p2ppck, sizeof(TPacketGGMessenger));
}

void MessengerManager::RemoveAllList(keyA account)
{
	std::set<keyT>	company(m_Relation[account]);

	// @fixme142 BEGIN
	DBManager::instance().EscapeString(__account, sizeof(__account), account.c_str(), account.size());
	if (account.compare(__account))
		return;
	// @fixme142 END

	DBManager::instance().Query("DELETE FROM messenger_list%s WHERE account='%s' OR companion='%s'",
			get_table_postfix(), __account, __account);

	for (std::set<keyT>::iterator iter = company.begin();
			iter != company.end();
			iter++ )
	{
		this->RemoveFromList(account, *iter);
		this->RemoveFromList(*iter, account); // @fixme183
	}

	for (std::set<keyT>::iterator iter = company.begin();
			iter != company.end();
			)
	{
		company.erase(iter++);
	}

	company.clear();
}

void MessengerManager::SendBlockList(LPCHARACTER who)
{
	if (!who)
		return;

	LPDESC d = who->GetDesc();
	if (!d)
		return;

	auto blockCount = GetBlockCount(who->GetPlayerID());
	if (blockCount < 1)
		return;

	TEMP_BUFFER buff;

	TPacketGCMessenger pack;
	pack.header = HEADER_GC_MESSENGER;
	pack.subheader = MESSENGER_SUBHEADER_GC_BLOCK_LIST;
	pack.size = sizeof(TPacketGCMessenger) + sizeof(TPacketGCMessengerBlockList) + sizeof(TPacketGCMessengerBlockItem) * blockCount;

	TPacketGCMessengerBlockList sub{};
	sub.count = blockCount;

	buff.write(&pack, sizeof(pack));
	buff.write(&sub, sizeof(sub));

	for (auto name : m_BlockMap[who->GetPlayerID()])
	{
		TPacketGCMessengerBlockItem item{};
		strncpy(item.name, name.c_str(), sizeof(item.name));
		item.isAdd = true;
		buff.write(&item, sizeof(item));
	}

	d->Packet(buff.read_peek(), buff.size());
}

void MessengerManager::SendBlock(LPCHARACTER who, const char* name, bool isBlock)
{
	if (!who)
		return;

	LPDESC d = who->GetDesc();
	if (!d)
		return;

	TEMP_BUFFER buff;

	TPacketGCMessenger pack{};
	pack.header = HEADER_GC_MESSENGER;
	pack.subheader = MESSENGER_SUBHEADER_GC_BLOCK_LIST;
	pack.size = sizeof(TPacketGCMessenger) + sizeof(TPacketGCMessengerBlockList) + sizeof(TPacketGCMessengerBlockItem);

	TPacketGCMessengerBlockList sub{};
	sub.count = 1;

	buff.write(&pack, sizeof(pack));
	buff.write(&sub, sizeof(sub));

	TPacketGCMessengerBlockItem item{};
	strncpy(item.name, name, sizeof(item.name));
	item.isAdd = isBlock;
	buff.write(&item, sizeof(item));

	d->Packet(buff.read_peek(), buff.size());
}

void MessengerManager::SendList(MessengerManager::keyA account)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());

	if (!ch)
		return;

	LPDESC d = ch->GetDesc();

	if (!d)
		return;

	if (m_Relation.find(account) == m_Relation.end())
		return;

	if (m_Relation[account].empty())
		return;

	TPacketGCMessenger pack;

	pack.header		= HEADER_GC_MESSENGER;
	pack.subheader	= MESSENGER_SUBHEADER_GC_LIST;
	pack.size		= sizeof(TPacketGCMessenger);

	TPacketGCMessengerListOffline pack_offline;
	TPacketGCMessengerListOnline pack_online;

	TEMP_BUFFER buf(128 * 1024); // 128k

	itertype(m_Relation[account]) it = m_Relation[account].begin(), eit = m_Relation[account].end();

	while (it != eit)
	{
		if (m_set_loginAccount.find(*it) != m_set_loginAccount.end())
		{
			pack_online.connected = 1;

			// Online
			pack_online.length = it->size();

			buf.write(&pack_online, sizeof(TPacketGCMessengerListOnline));
			buf.write(it->c_str(), it->size());
		}
		else
		{
			pack_offline.connected = 0;

			// Offline
			pack_offline.length = it->size();

			buf.write(&pack_offline, sizeof(TPacketGCMessengerListOffline));
			buf.write(it->c_str(), it->size());
		}

		++it;
	}

	pack.size += buf.size();

	d->BufferedPacket(&pack, sizeof(TPacketGCMessenger));
	d->Packet(buf.read_peek(), buf.size());
}

void MessengerManager::SendLogin(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	if (ch->GetGMLevel() == GM_PLAYER && gm_get_level(companion.c_str()) != GM_PLAYER)
		return;

	BYTE bLen = companion.size();

	TPacketGCMessenger pack;

	pack.header			= HEADER_GC_MESSENGER;
	pack.subheader		= MESSENGER_SUBHEADER_GC_LOGIN;
	pack.size			= sizeof(TPacketGCMessenger) + sizeof(BYTE) + bLen;

	d->BufferedPacket(&pack, sizeof(TPacketGCMessenger));
	d->BufferedPacket(&bLen, sizeof(BYTE));
	d->Packet(companion.c_str(), companion.size());
}

void MessengerManager::SendLogout(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	if (!companion.size())
		return;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (!d)
		return;

	BYTE bLen = companion.size();

	TPacketGCMessenger pack;

	pack.header		= HEADER_GC_MESSENGER;
	pack.subheader	= MESSENGER_SUBHEADER_GC_LOGOUT;
	pack.size		= sizeof(TPacketGCMessenger) + sizeof(BYTE) + bLen;

	d->BufferedPacket(&pack, sizeof(TPacketGCMessenger));
	d->BufferedPacket(&bLen, sizeof(BYTE));
	d->Packet(companion.c_str(), companion.size());
}

// BLOCK MESSENGER
bool MessengerManager::IsBlock(DWORD whoPID, std::string targetName)
{
	auto it = m_BlockMap.find(whoPID);
	if (it == m_BlockMap.end())
		return false;

	const auto& blockSet = it->second;
	if (blockSet.empty())
		return false;

	return blockSet.find(targetName) != blockSet.end();
}

DWORD MessengerManager::GetBlockCount(DWORD whoPID)
{
	auto it = m_BlockMap.find(whoPID);
	if (it == m_BlockMap.end())
		return 0;

	return it->second.size();
}

void MessengerManager::LoadBlockList(LPCHARACTER who)
{
	if (!who)
		return;

	DWORD pid = who->GetPlayerID();

	TPacketGGBlockMessenger p{};
	p.header = HEADER_GG_BLOCK_MESSENGER;
	p.subheader = SUBHEADER_BLOCK_MESSENGER_LOAD;

	TSubPacketGGBlockMessengerLoad sub{};
	sub.pid = pid;

	m_BlockMap[pid].clear();

	char buf[512];
	sprintf(buf, "SELECT blocked FROM messenger_block_list WHERE pid=%ld", pid);

	auto pkMsg(DBManager::instance().DirectQuery(buf));
	SQLResult* pRes = pkMsg->Get();

	if (pRes->uiNumRows > 0)
	{
		sub.itemCount = pRes->uiNumRows;

		TEMP_BUFFER buff;
		buff.write(&p, sizeof(p));
		buff.write(&sub, sizeof(sub));

		MYSQL_ROW	data;
		while ((data = mysql_fetch_row(pRes->pSQLResult)))
		{
			m_BlockMap[pid].insert(data[0]);

			char playerName[PLAYER_NAME_MAX_LEN + 1];
			strncpy(playerName, data[0], sizeof(playerName));

			buff.write(&playerName, sizeof(playerName));
		}

		SendBlockList(who);

		P2P_MANAGER::instance().Send(buff.read_peek(), buff.size());
	}
}

void MessengerManager::__InsertBlockListP2P(DWORD pid, std::string name)
{
	m_BlockMap[pid].insert(name);
}

void SendMessengerBlockP2PPacket(BYTE subheader, DWORD pid, const char * targetName)
{
	TPacketGGBlockMessenger p{};
	p.header = HEADER_GG_BLOCK_MESSENGER;
	p.subheader = subheader;

	TSubPacketGGBlockMessengerBlock sub{};
	sub.pid = pid;
	strncpy(sub.name, targetName, sizeof(sub.name));

	TEMP_BUFFER buff;
	buff.write(&p, sizeof(p));
	buff.write(&sub, sizeof(sub));

	P2P_MANAGER::instance().Send(buff.read_peek(), buff.size());
}

void MessengerManager::Block(LPCHARACTER ch, std::string targetName)
{
	if (!ch)
		return;

	if (targetName.find('[') != std::string::npos)
	{
		ch->ChatDebug("nie mozesz zablokowac GMa");
		return;
	}

	if (IsFriend(ch->GetName(), targetName))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MESSENGER_BLOCK_IS_FRIEND"));
		return;
	}

	DWORD mePID = ch->GetPlayerID();
	DWORD whoPID = 0;

	if (IsBlock(mePID, targetName))
		return;

	if (GetBlockCount(mePID) >= MESSENGER_MAX_BLOCK_LIMIT)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MESSENGER_BLOCK_MAX_LIMIT%d"), MESSENGER_MAX_BLOCK_LIMIT);
		return;
	}

	if (const auto targetCh = CHARACTER_MANAGER::instance().FindPC(targetName.c_str()))
	{
		whoPID = targetCh->GetPlayerID();
	}
	else if (const auto pkCCI = P2P_MANAGER::instance().Find(targetName.c_str()))
	{
		whoPID = pkCCI->dwPID;
	}

	if (whoPID == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MESSENGER_BLOCK_NO_PLAYER"));
		return;
	}

	if (whoPID == mePID)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MESSENGER_BLOCK_YOURSELF"));
		return;
	}

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MESSENGER_BLOCK_SUCCESS%s"), targetName.c_str());

	char buf[PLAYER_NAME_MAX_LEN*2 + 1];
	DBManager::instance().EscapeString(buf, sizeof(buf), targetName.c_str(), targetName.length());

	sys_log(0, "Messenger BlockAdd %d %s", mePID, buf);
	DBManager::instance().Query("INSERT INTO messenger_block_list%s VALUES (%d, '%s')",
		get_table_postfix(), mePID, buf);

	m_BlockMap[mePID].insert(targetName);

	SendBlock(ch, targetName.c_str(), true);

	SendMessengerBlockP2PPacket(SUBHEADER_BLOCK_MESSENGER_BLOCK, mePID, targetName.c_str());
}

void MessengerManager::__BlockP2P(DWORD mePID, std::string targetName)
{
	if (IsBlock(mePID, targetName))
		return;

	m_BlockMap[mePID].insert(targetName);
}

void MessengerManager::Unblock(LPCHARACTER ch, std::string targetName)
{
	if (!ch)
		return;

	if (!IsBlock(ch->GetPlayerID(), targetName))
		return;
	
	char buf[PLAYER_NAME_MAX_LEN*2 + 1];
	DBManager::instance().EscapeString(buf, sizeof(buf), targetName.c_str(), targetName.length());

	sys_log(0, "Messenger Unblock %d %s", ch->GetPlayerID(), buf);

	DBManager::instance().Query("DELETE FROM messenger_block_list%s WHERE pid=%d AND blocked = '%s'",
		get_table_postfix(), ch->GetPlayerID(), buf);

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MESSENGER_UNBLOCK_SUCCESS%s"), targetName.c_str());

	m_BlockMap[ch->GetPlayerID()].erase(targetName);

	SendBlock(ch, targetName.c_str(), false);

	SendMessengerBlockP2PPacket(SUBHEADER_BLOCK_MESSENGER_UNBLOCK, ch->GetPlayerID(), targetName.c_str());
}

void MessengerManager::__UnblockP2P(DWORD mePID, std::string targetName)
{
	if (!IsBlock(mePID, targetName))
		return;

	m_BlockMap[mePID].erase(targetName);
}

// END OF BLOCK MESSENGER
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
