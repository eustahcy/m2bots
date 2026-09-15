#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "input.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "protocol.h"
#include "locale_service.h"
#include "db.h"
#include "Hardware.h"

#ifndef __WIN32__
	#include "limit_time.h"
#endif

#include "utils.h"

#include "../../common/PulseManager.h"

// #define ENABLE_ACCOUNT_W_SPECIALCHARS
bool FN_IS_VALID_LOGIN_STRING(const char *str)
{
	const char*	tmp;

	if (!str || !*str)
		return false;

	if (strlen(str) < 2)
		return false;

	for (tmp = str; *tmp; ++tmp)
	{
		if (isdigit(*tmp) || isalpha(*tmp))
			continue;

#ifdef ENABLE_ACCOUNT_W_SPECIALCHARS

		switch (*tmp)
		{
			case ' ':
			case '_':
			case '-':
			case '.':
			case '!':
			case '@':
			case '#':
			case '$':
			case '%':
			case '^':
			case '&':
			case '*':
			case '(':
			case ')':
				continue;
		}
#endif
		return false;
	}

	return true;
}

bool Login_IsInChannelService(const char* c_login)
{
	if (c_login[0] == '[')
		return true;
	return false;
}

CInputAuth::CInputAuth()
{
}

std::string GetAuthLoginQuery(const char* c_pszLogin, const char* c_pszPassword, const char* c_pszHardware)
{
	// @fixme138 alternative for mysql8
	#define _MYSQL_NATIVE_PASSWORD(str) "CONCAT('*', UPPER(SHA1(UNHEX(SHA1(" str ")))))"

	char szQuery[1024];
auto query = "SELECT " 
#ifdef _WIN32
    _MYSQL_NATIVE_PASSWORD("'%s'")
#else
    "'%s'"
#endif
    ",ac.password,'%s',ac.social_id,ac.id,ac.status,hw.status,ac.availDt - NOW() > 0,"
    "UNIX_TIMESTAMP(ac.premium_expire),"
    "UNIX_TIMESTAMP(ac.silver_expire),"
    "UNIX_TIMESTAMP(ac.gold_expire),"
    "UNIX_TIMESTAMP(ac.safebox_expire),"
    "UNIX_TIMESTAMP(ac.autoloot_expire),"
    "UNIX_TIMESTAMP(ac.fish_mind_expire),"
    "UNIX_TIMESTAMP(ac.marriage_fast_expire),"
    "UNIX_TIMESTAMP(ac.money_drop_rate_expire),"
    "UNIX_TIMESTAMP(ac.shop_expire),"
    "UNIX_TIMESTAMP(ac.create_time) "
    "FROM account.account AS ac "  // DODAJ account.
    "LEFT JOIN account.hardware_status AS hw ON hw.hwid='%s' "  // DODAJ account.
    "WHERE login = '%s'";

	snprintf(szQuery, sizeof(szQuery), query, c_pszPassword, c_pszHardware, c_pszHardware, c_pszLogin);
	return szQuery;
}

void CInputAuth::CheckClientVersion(LPDESC d, const char* c_pData)
{
	TPacketCGCheckVersion* pinfo = (TPacketCGCheckVersion*)c_pData;
	d->SetClientVersion(pinfo->dwVersion);
	return;
}

void CInputAuth::Login(LPDESC d, const char * c_pData)
{
	TPacketCGLogin3 * pinfo = (TPacketCGLogin3 *) c_pData;

	if (!g_bAuthServer)
	{
		sys_err ("CInputAuth class is not for game server. IP %s might be a hacker.",
			inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return;
	}

	if (d->GetClientVersion() < g_dwClientVersion)
	{
		LoginFailure(d, "UPDATE");
		return;
	}

	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(pinfo->login, login, sizeof(login));

	char passwd[PASSWD_MAX_LEN + 1];
	strlcpy(passwd, pinfo->passwd, sizeof(passwd));

	// hwid
	char szHardware[HWID_MAX_LEN + 1];
	std::string sDecodedHWID = CheckHardware::instance().DecodeHWID(pinfo->hwid);
	if (!sDecodedHWID.size() || sDecodedHWID[0] == '\0')
	{
		LoginFailure(d, "BLOCK");
		return;
	}

	if (sDecodedHWID.size())
		strlcpy(szHardware, sDecodedHWID.c_str(), sizeof(szHardware));
	else
		std::memset(szHardware, 0, sizeof(szHardware));
	// end of hwid

	sys_log(0, "InputAuth::Login : %s(%d) desc %p",
			login, strlen(login), get_pointer(d));

	// check login string
	if (false == FN_IS_VALID_LOGIN_STRING(login))
	{
		sys_log(0, "InputAuth::Login : IS_NOT_VALID_LOGIN_STRING(%s) desc %p",
				login, get_pointer(d));
		LoginFailure(d, "NOID");
		return;
	}

	if (!PulseManager::Instance().IncreaseClock(inet_addr(d->GetHostName()), ePulse::Login, std::chrono::milliseconds(500)))
	{
		LoginFailure(d, "FAILURE");
		return;
	}


	if (g_bNoMoreClient)
	{
		TPacketGCLoginFailure failurePacket;

		failurePacket.header = HEADER_GC_LOGIN_FAILURE;
		strlcpy(failurePacket.szStatus, "SHUTDOWN", sizeof(failurePacket.szStatus));

		d->Packet(&failurePacket, sizeof(failurePacket));
		return;
	}

	if (DESC_MANAGER::instance().FindByLoginName(login))
	{
		LoginFailure(d, "ALREADY");
		return;
	}

	DWORD dwKey = DESC_MANAGER::instance().CreateLoginKey(d);
	DWORD dwPanamaKey = dwKey ^ pinfo->adwClientKey[0] ^ pinfo->adwClientKey[1] ^ pinfo->adwClientKey[2] ^ pinfo->adwClientKey[3];
	d->SetPanamaKey(dwPanamaKey);

	sys_log(0, "InputAuth::Login : key %u:0x%x login %s", dwKey, dwPanamaKey, login);

	TPacketCGLogin3 * p = M2_NEW TPacketCGLogin3;
	thecore_memcpy(p, pinfo, sizeof(TPacketCGLogin3));

	char szPasswd[PASSWD_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szPasswd, sizeof(szPasswd), passwd, strlen(passwd));

	char szLogin[LOGIN_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szLogin, sizeof(szLogin), login, strlen(login));

	char szHWID[HWID_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szHWID, sizeof(szHWID), szHardware, strlen(szHardware));

	// CHANNEL_SERVICE_LOGIN
	if (Login_IsInChannelService(szLogin))
	{
		sys_log(0, "ChannelServiceLogin [%s]", szLogin);

		DBManager::instance().ReturnQuery(QID_AUTH_LOGIN, dwKey, p,
				GetAuthLoginQuery(szLogin, szPasswd, szHardware).c_str());
	}
	// END_OF_CHANNEL_SERVICE_LOGIN
	else
	{
#ifdef __WIN32__
		DBManager::instance().ReturnQuery(QID_AUTH_LOGIN, dwKey, p,
			GetAuthLoginQuery(szLogin, szPasswd, szHardware).c_str());
#else
		// @fixme138 1. PASSWORD('%s') -> %s 2. szPasswd wrapped inside mysql_hash_password(%s).c_str()
		// nie wysylamy szPasswd po escape stringu bo i tak sam string nie trafia jako query tylko hash
		// a powoduje to blad hasla przy haslach ze znakiem "'" lub "/" i "\"
		DBManager::instance().ReturnQuery(QID_AUTH_LOGIN, dwKey, p,
			GetAuthLoginQuery(szLogin, mysql_hash_password(passwd).c_str(), szHardware).c_str());
#endif
	}
}

int CInputAuth::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	if (!g_bAuthServer)
	{
		sys_err ("CInputAuth class is not for game server. IP %s might be a hacker.",
			inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return 0;
	}

	if (d && !PulseManager::Instance().IncreaseCount(inet_addr(d->GetHostName()), ePulse::AnalyzeAuth, std::chrono::milliseconds(1000), 7))
	{
		d->DelayedDisconnect(1);
		return 0;
	}

	int iExtraLen = 0;

	if (test_server)
		sys_log(0, " InputAuth Analyze Header[%d] ", bHeader);

	switch (bHeader)
	{
		case HEADER_CG_PONG:
			Pong(d);
			break;

		case HEADER_CG_LOGIN3:
			Login(d, c_pData);
			break;

		case HEADER_CG_CHECK_VERSION:
			CheckClientVersion(d, c_pData);
			break;

		case HEADER_CG_HANDSHAKE:
			break;

		default:
			sys_err("This phase does not handle this header %d (0x%x)(phase: AUTH)", bHeader, bHeader);
			break;
	}

	return iExtraLen;
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
