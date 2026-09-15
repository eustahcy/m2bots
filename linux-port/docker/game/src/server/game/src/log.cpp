#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "log.h"

#include "char.h"
#include "desc.h"
#include "item.h"
#include "utils.h"

static char	__escape_hint[1024];

LogManager::LogManager() : m_bIsConnect(false)
{
}

LogManager::~LogManager()
{
}

bool LogManager::Connect(const char * host, const int port, const char * user, const char * pwd, const char * db)
{
	if (m_sql.Setup(host, user, pwd, db, g_stLocale.c_str(), false, port))
		m_bIsConnect = true;

	return m_bIsConnect;
}

void LogManager::Query(const char * c_pszFormat, ...)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);

	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, sizeof(szQuery), c_pszFormat, args);
	va_end(args);

	if (test_server)
		sys_log(0, "LOG: %s", szQuery);

	m_sql.AsyncQuery(szQuery);
}

bool LogManager::IsConnected()
{
	return m_bIsConnect;
}

size_t LogManager::EscapeString(char* dst, size_t dstSize, const char *src, size_t srcSize)
{
	return m_sql.EscapeString(dst, dstSize, src, srcSize);
}

void LogManager::ItemLog(DWORD dwPID, DWORD x, DWORD y, DWORD dwItemID, const char * c_pszText, const char * c_pszHint, const char * c_pszIP, DWORD dwVnum)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), c_pszHint, strlen(c_pszHint));

	Query("INSERT INTO log%s (type, time, who, x, y, what, how, hint, ip, vnum) VALUES('ITEM', NOW(), %u, %u, %u, %u, '%s', '%s', '%s', %u)",
			get_table_postfix(), dwPID, x, y, dwItemID, c_pszText, __escape_hint, c_pszIP, dwVnum);
}

void LogManager::ItemLog(LPCHARACTER ch, LPITEM item, const char * c_pszText, const char * c_pszHint, bool addItemMetaData)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	if (NULL == ch || NULL == item)
	{
		sys_err("character or item nil (ch %p item %p text %s)", get_pointer(ch), get_pointer(item), c_pszText);
		return;
	}
	
	char buf[192 + 1];
	if (addItemMetaData)
	{
		std::ostringstream sockets;
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++) {
			sockets << item->GetSocket(i);
			if (i < ITEM_SOCKET_MAX_NUM - 1) {
				sockets << ",";
			}
		}

		std::ostringstream attrs;
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++) {
			auto attr = item->GetAttribute(i);
			attrs << static_cast<int>(attr.bType) << "," << attr.sValue;
			if (i < ITEM_ATTRIBUTE_MAX_NUM - 1) {
				attrs << "|";
			}
		}

		snprintf(buf, sizeof(buf), "%s %s %s", c_pszHint, sockets.str().c_str(), attrs.str().c_str());
	}
	else
		snprintf(buf, sizeof(buf), "%s", c_pszHint);

	ItemLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(), item->GetID(),
			NULL == c_pszText ? "" : c_pszText,
		buf, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "",
		   	item->GetOriginalVnum());
}

void LogManager::ItemLog(LPCHARACTER ch, int itemID, int itemVnum, const char * c_pszText, const char * c_pszHint)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	ItemLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(), itemID, c_pszText, c_pszHint, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "", itemVnum);
}

void LogManager::ItemSocketLog(DWORD who, DWORD itemID, DWORD itemVnum, BYTE socket, int value, const char* what)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MID);

	char buf[80 + 1];
	snprintf(buf, sizeof(buf), "%u %d", socket, value);

	ItemMetaLog(who, itemID, itemVnum, buf, what);
}

void LogManager::ItemAttrLog(DWORD who, DWORD itemID, DWORD itemVnum, BYTE type, int value, const char* what)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MID);

	char buf[80 + 1];
	snprintf(buf, sizeof(buf), "%u %d", type, value);
	ItemMetaLog(who, itemID, itemVnum, buf, what);
}

void LogManager::ItemMetaLog(DWORD who, DWORD itemID, DWORD itemVnum, const char* value, const char* what)
{
	Query("INSERT INTO item_meta_log%s (itemid, time, who, what, value, vnum) VALUES(%d, NOW(), %d, '%s', '%s', %d)",
		get_table_postfix(), itemID, who, what, value, itemVnum);
}

void LogManager::CharLog(DWORD dwPID, DWORD x, DWORD y, long long dwValue, const char * c_pszText, const char * c_pszHint, const char * c_pszIP)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), c_pszHint, strlen(c_pszHint));

	Query("INSERT INTO log%s (`type`, `time`, `who`, `x`, `y`, `what`, `how`, `hint`, `ip`) VALUES('CHARACTER', NOW(), %d, %d, %d, %lld, '%s', '%s', '%s')",
			get_table_postfix(), dwPID, x, y, dwValue, c_pszText, __escape_hint, c_pszIP);
}

void LogManager::CharLog(LPCHARACTER ch, long long dw, const char * c_pszText, const char * c_pszHint)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	if (ch)
		CharLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(), dw, c_pszText, c_pszHint, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "");
	else
		CharLog(0, 0, 0, dw, c_pszText, c_pszHint, "");
}

void LogManager::LoginLog(bool isLogin, LPCHARACTER ch)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	if (!ch || !ch->GetDesc())
		return;

	char buf[42 + 1];
	snprintf(buf, sizeof(buf), "gold: %lld alignment: %d", ch->GetGold(), ch->GetAlignment());

	auto accTable = ch->GetDesc()->GetAccountTable();
	Query("INSERT INTO loginlog%s (type, time, channel, account_id, pid, mapIndex, x, y, playtime, ip, hwid, info) VALUES (%s, NOW(), %d, %d, %d, %ld, %ld, %ld, %d, '%s', '%s', '%s')",
			get_table_postfix(), isLogin ? "'LOGIN'" : "'LOGOUT'",
		g_bChannel, accTable.id,
		ch->GetPlayerID(), ch->GetMapIndex(), ch->GetX(), ch->GetY(),
		ch->GetRealPoint(POINT_PLAYTIME),
		inet_ntoa(ch->GetDesc()->GetAddr().sin_addr), accTable.hwid, buf);
}

void LogManager::MoneyLog(BYTE type, DWORD vnum, YANG gold)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MAX);
	if (type == MONEY_LOG_RESERVED || type >= MONEY_LOG_TYPE_MAX_NUM)
	{
		sys_err("TYPE ERROR: type %d vnum %u gold %lld", type, vnum, gold);
		return;
	}

	Query("INSERT INTO money_log%s VALUES (NOW(), %d, %d, %lld)", get_table_postfix(), type, vnum, gold);
}

void LogManager::ExchangeLog(const char* type, LPCHARACTER who, DWORD targetPid, DWORD vnum, ITEM_COUNT count, DWORD itemid)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);

	char buf[32 + 1];
	snprintf(buf, sizeof(buf), "%u %ld %ld %ld", g_bChannel, who->GetMapIndex(), who->GetX(), who->GetY());

	Query("INSERT INTO exchange_log%s VALUES ('%s', NOW(), %d, %d, %d, %d, %d, '%s')", get_table_postfix(), type, who->GetPlayerID(), targetPid, itemid, vnum, count, buf);
}

void LogManager::ExchangeGoldLog(const char* type, LPCHARACTER who, DWORD targetPid, YANG gold)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);

	char buf[32 + 1];
	snprintf(buf, sizeof(buf), "%u %ld %ld %ld", g_bChannel, who->GetMapIndex(), who->GetX(), who->GetY());

	Query("INSERT INTO exchange_log%s VALUES ('%s', NOW(), %d, %d, 0, 0, %lld, '%s')", get_table_postfix(), type, who->GetPlayerID(), targetPid, gold, buf);
}

void LogManager::HackLog(const char * c_pszHackName, const char * c_pszLogin, const char * c_pszName, const char * c_pszIP)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MID);
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), c_pszHackName, strlen(c_pszHackName));

	Query("INSERT INTO hack_log (time, login, name, ip, server, why) VALUES(NOW(), '%s', '%s', '%s', '%s', '%s')", c_pszLogin, c_pszName, c_pszIP, g_stHostname.c_str(), __escape_hint);
}

void LogManager::HackLog(const char * c_pszHackName, LPCHARACTER ch)
{
	if (!ch)
		return;

	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MID);
	if (ch->GetDesc())
	{
		HackLog(c_pszHackName,
				ch->GetDesc()->GetAccountTable().login,
				ch->GetName(),
				ch->GetDesc()->GetHostName());
	}
}

void LogManager::RequestInfoLog(DWORD pid, BYTE type, const char* c_pszArg1, const char* c_pszArg2)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MID);
	char buf1[255*2 + 1];
	m_sql.EscapeString(buf1, sizeof(buf1), c_pszArg1, strlen(c_pszArg1));

	char buf2[255*2 + 1];
	m_sql.EscapeString(buf2, sizeof(buf2), c_pszArg2, strlen(c_pszArg2));

	Query("INSERT INTO request_info_log (pid, time, type, arg1, arg2) VALUES(%d, NOW(), %u, '%s', '%s')", pid, type, buf1, buf2);
}

void LogManager::GoldBarLog(DWORD dwPID, DWORD dwItemID, GOLDBAR_HOW eHow, const char* c_pszHint)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	char szHow[32+1];

	switch (eHow)
	{
		case PERSONAL_SHOP_BUY:
			snprintf(szHow, sizeof(szHow), "'BUY'");
			break;

		case PERSONAL_SHOP_SELL:
			snprintf(szHow, sizeof(szHow), "'SELL'");
			break;

		case SHOP_BUY:
			snprintf(szHow, sizeof(szHow), "'SHOP_BUY'");
			break;

		case SHOP_SELL:
			snprintf(szHow, sizeof(szHow), "'SHOP_SELL'");
			break;

		case EXCHANGE_TAKE:
			snprintf(szHow, sizeof(szHow), "'EXCHANGE_TAKE'");
			break;

		case EXCHANGE_GIVE:
			snprintf(szHow, sizeof(szHow), "'EXCHANGE_GIVE'");
			break;

		case QUEST:
			snprintf(szHow, sizeof(szHow), "'QUEST'");
			break;

		default:
			snprintf(szHow, sizeof(szHow), "''");
			break;
	}

	Query("INSERT INTO goldlog%s (date, time, pid, what, how, hint) VALUES(CURDATE(), CURTIME(), %u, %u, %s, '%s')",
			get_table_postfix(), dwPID, dwItemID, szHow, c_pszHint);
}

void LogManager::CubeLog(DWORD dwPID, DWORD x, DWORD y, DWORD item_vnum, DWORD item_uid, int item_count, bool success)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MID);
	Query("INSERT INTO cube%s (pid, time, x, y, item_vnum, item_uid, item_count, success) "
			"VALUES(%u, NOW(), %u, %u, %u, %u, %d, %d)",
			get_table_postfix(), dwPID, x, y, item_vnum, item_uid, item_count, success?1:0);
}

void LogManager::SpeedHackLog(DWORD pid, DWORD x, DWORD y, int hack_count)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MID);
	Query("INSERT INTO speed_hack%s (pid, time, x, y, hack_count) "
			"VALUES(%u, NOW(), %u, %u, %d)",
			get_table_postfix(), pid, x, y, hack_count);
}

void LogManager::ChangeNameLog(DWORD pid, const char *old_name, const char *new_name, const char *ip)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	Query("INSERT INTO change_name%s (pid, old_name, new_name, time, ip) "
			"VALUES(%u, '%s', '%s', NOW(), '%s') ",
			get_table_postfix(), pid, old_name, new_name, ip);
}

void LogManager::GMCommandLog(DWORD dwPID, const char* szName, const char* szIP, BYTE byChannel, const char* szCommand)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), szCommand, strlen(szCommand));

	Query("INSERT INTO command_log%s (userid, server, ip, port, username, command, date ) "
			"VALUES(%u, 999, '%s', %u, '%s', '%s', NOW()) ",
			get_table_postfix(), dwPID, szIP, byChannel, szName, __escape_hint);
}

void LogManager::RefineLog(DWORD pid, const char* item_name, DWORD item_id, int item_refine_level, int is_success, const char* how)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), item_name, strlen(item_name));

	Query("INSERT INTO refinelog%s (pid, item_name, item_id, step, time, is_success, setType) VALUES(%u, '%s', %u, %d, NOW(), %d, '%s')",
			get_table_postfix(), pid, __escape_hint, item_id, item_refine_level, is_success, how);
}

void LogManager::LevelLog(LPCHARACTER pChar, unsigned int level, unsigned int playhour)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	DWORD aid = 0;

	if (NULL != pChar->GetDesc())
	{
		aid = pChar->GetDesc()->GetAccountTable().id;
	}

	Query("REPLACE INTO levellog%s (level, time, account_id, pid, playtime) VALUES(%u, NOW(), %d, %d, %d)",
			get_table_postfix(), level, aid, pChar->GetPlayerID(), playhour);
}

void LogManager::BootLog(const char * c_pszHostName, BYTE bChannel)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	Query("INSERT INTO bootlog (time, hostname, channel) VALUES(NOW(), '%s', %d)",
			c_pszHostName, bChannel);
}

void LogManager::QuestRewardLog(const char * c_pszQuestName, DWORD dwPID, DWORD dwLevel, long long llValue1, int iValue2, BYTE type)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MAX);
	Query("INSERT INTO quest_reward_log%s VALUES('%s',%d,%u,%u,%lld,%d,NOW())",
			get_table_postfix(),
			c_pszQuestName,
			dwPID,
			dwLevel,
			type,
			llValue1,
			iValue2);
}

void LogManager::QuestStateLog(const char* c_pszQuestName, const char* c_pszQuestState, DWORD dwPID)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	Query("INSERT INTO quest_state_log%s (quest_name, quest_state, time, pid) VALUES('%s', '%s', NOW(), %d)",
		get_table_postfix(),
		c_pszQuestName,
		c_pszQuestState,
		dwPID);
}

void LogManager::DetailLoginLog(bool isLogin, LPCHARACTER ch)
{
	if (!ch || !ch->GetDesc())
		return;

	DetailLoginLog(isLogin, ch->GetDesc(), ch->GetPlayerID());
}

void LogManager::DetailLoginLog(bool isLogin, LPDESC d, DWORD pid)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MID);
	if (NULL == d)
		return;

	Query("SET @i = (SELECT COALESCE(MAX(id), 0) FROM loginlog2 WHERE account_id = %d AND pid = %d)",
		d->GetAccountTable().id,
		pid);

	if (true == isLogin)
	{
		auto accTable = d->GetAccountTable();
		Query("INSERT INTO loginlog2(id, type, login_time, channel, account_id, pid, ip, hwid) "
			"VALUES(@i+1,'INVALID', NOW(), %u, %d, %d, '%s', '%s')",
			g_bChannel,
			accTable.id,
			pid,
			inet_ntoa(d->GetAddr().sin_addr),
			accTable.hwid);
	}
	else
	{
		Query("UPDATE loginlog2 SET type='VALID', logout_time=NOW(), playtime=TIMEDIFF(logout_time,login_time) WHERE id=@i and pid=%d", pid);
	}
}

void LogManager::DragonSlayLog(DWORD dwGuildID, DWORD dwDragonVnum, DWORD dwStartTime, DWORD dwEndTime)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MAX);
	Query( "INSERT INTO dragon_slay_log%s VALUES( %d, %d, FROM_UNIXTIME(%d), FROM_UNIXTIME(%d) )",
			get_table_postfix(),
			dwGuildID, dwDragonVnum, dwStartTime, dwEndTime);
}

void LogManager::ChatLog(DWORD where, DWORD who_id, const char* who_name, DWORD whom_id, const char* whom_name, const char* type, const char* msg, const char* ip)
{
	Query("INSERT INTO `chat_log%s` (`where`, `who_id`, `who_name`, `whom_id`, `whom_name`, `type`, `msg`, `when`, `ip`) "
		"VALUES (%u, %u, '%s', %u, '%s', '%s', '%s', NOW(), '%s');",
		get_table_postfix(),
		where, who_id, who_name, whom_id, whom_name, type, msg, ip);
}

void LogManager::CaptchaLog(LPCHARACTER ch, const char* action, const char* code)
{
	if (!ch || !ch->HasPlayerData())
		return;

	char buf[CAPTCHA_MAX_LEN * 2 + 1];
	m_sql.EscapeString(buf, sizeof(buf), code, strlen(code));

	auto captcha = ch->playerData->GetPlayerCaptcha();
	DWORD leftDuration = 0;
	if (captcha.expireTime > get_global_time())
		leftDuration = captcha.expireTime - get_global_time();

	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	Query("INSERT INTO captcha_log%s VALUES( %d, %d, NOW(), '%s', '%s', %d )",
		get_table_postfix(),
		ch->GetPlayerID(), captcha.invokerPID, action, buf, leftDuration);
}

void LogManager::VoucherCodeLog(LPCHARACTER ch, const char* code)
{
	if (!ch)
		return;

	if (!ch->GetDesc())
		return;

	char buf[CODE_VOUCHER_MAX_LEN * 2 + 1];
	m_sql.EscapeString(buf, sizeof(buf), code, strlen(code));

	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);
	Query("INSERT INTO voucher_code_log%s VALUES( '%s', %d, %d, NOW())",
		get_table_postfix(),
		buf, ch->GetPlayerID(), ch->GetDesc()->GetAccountTable().id);
}

void LogManager::GoldSessionLog(LPCHARACTER ch)
{
	if (!ch || !ch->playerData)
		return;

	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);

	Query("INSERT INTO gold_session_log%s VALUES(%d, %lld, NOW(), %d)",
		get_table_postfix(),
		ch->GetPlayerID(), ch->playerData->GetTotalGoldSession(), get_global_time() - ch->playerData->startGoldSessionTimeStamp);
}

void LogManager::ReportPlayerLog(LPCHARACTER who, LPCHARACTER target, BYTE reason)
{
	if (!who)
		return;

	if (!target)
		return;

	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);

	std::string strReason = "UNKNOWN";
	if (reason == 1)
		strReason = "BOT";
	else if (reason == 2)
		strReason = "FISHBOT";
	else if (reason == 3)
		strReason = "HACK";
	else if (reason == 4)
		strReason = "INSULT";
	else
		return;
	
	Query("INSERT INTO report_player_log%s VALUES(%d, %d, '%s', '%s', NOW())",
		get_table_postfix(),
		who->GetPlayerID(), target->GetPlayerID(), target->GetName(), strReason.c_str());
}

void LogManager::FishLog(DWORD playerId, DWORD itemVnum, ITEM_COUNT count, int rodLevel, int baitVnum)
{
	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MIN);

	Query("INSERT INTO fish_log%s VALUES(NOW(), %d, %d, %d, %d, %d)",
		get_table_postfix(), playerId, itemVnum, count, rodLevel, baitVnum);
}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
void LogManager::AcceLog(DWORD dwPID, DWORD x, DWORD y, DWORD item_vnum, DWORD item_uid, int item_count, int abs_chance, bool success)
{
	Query("INSERT INTO acce%s (pid, time, x, y, item_vnum, item_uid, item_count, item_abs_chance, success) VALUES(%u, NOW(), %u, %u, %u, %u, %d, %d, %d)", get_table_postfix(), dwPID, x, y, item_vnum, item_uid, item_count, abs_chance, success ? 1 : 0);
}
#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
