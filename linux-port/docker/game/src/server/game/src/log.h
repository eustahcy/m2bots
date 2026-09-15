#ifndef __INC_LOG_MANAGER_H__
#define __INC_LOG_MANAGER_H__

#include "../../libsql/AsyncSQL.h"
#include "any_function.h"
#include "locale_service.h"

// #ifdef ENABLE_NEWSTUFF
enum log_level {LOG_LEVEL_NONE=0, LOG_LEVEL_MIN=1, LOG_LEVEL_MID=2, LOG_LEVEL_MAX=3};
// #endif

#ifdef ENABLE_NEWSTUFF
#define LOG_LEVEL_CHECK_N_RET(x) { if (g_iDbLogLevel < x) return; }
#else
#define LOG_LEVEL_CHECK_N_RET(x) { }
#endif

#ifdef ENABLE_NEWSTUFF
#define LOG_LEVEL_CHECK(x, fnc)	\
	{\
		if (g_iDbLogLevel >= (x))\
			fnc;\
	}
#else
#define LOG_LEVEL_CHECK(x, fnc)	{ fnc; }
#endif

enum GOLDBAR_HOW
{
	PERSONAL_SHOP_BUY	= 1 ,
	PERSONAL_SHOP_SELL	= 2 ,
	SHOP_BUY			= 3 ,
	SHOP_SELL			= 4 ,
	EXCHANGE_TAKE		= 5 ,
	EXCHANGE_GIVE		= 6 ,
	QUEST				= 7 ,
};

class LogManager : public singleton<LogManager>
{
	public:
		LogManager();
		virtual ~LogManager();

		bool		IsConnected();

		bool		Connect(const char * host, const int port, const char * user, const char * pwd, const char * db);

		void		ItemLog(DWORD dwPID, DWORD x, DWORD y, DWORD dwItemID, const char * c_pszText, const char * c_pszHint, const char * c_pszIP, DWORD dwVnum);
		void		ItemLog(LPCHARACTER ch, LPITEM item, const char * c_pszText, const char * c_pszHint, bool addItemMetaData=false);
		void		ItemLog(LPCHARACTER ch, int itemID, int itemVnum, const char * c_pszText, const char * c_pszHint);
		void		ItemSocketLog(DWORD who, DWORD itemID, DWORD itemVnum, BYTE socket, int value, const char* what="SOCKET");
		void		ItemAttrLog(DWORD who, DWORD itemID, DWORD itemVnum, BYTE type, int value, const char* what="ATTR");
		void		ItemMetaLog(DWORD who, DWORD itemID, DWORD itemVnum, const char* value, const char* what);

		void		CharLog(DWORD dwPID, DWORD x, DWORD y, long long dw, const char * c_pszText, const char * c_pszHint, const char * c_pszIP);
		void		CharLog(LPCHARACTER ch, long long dw, const char * c_pszText, const char * c_pszHint);

		void		LoginLog(bool isLogin, LPCHARACTER ch);
		void		MoneyLog(BYTE type, DWORD vnum, YANG gold);
		void		ExchangeLog(const char* type, LPCHARACTER who, DWORD targetPid, DWORD vnum, ITEM_COUNT count, DWORD itemid);
		void		ExchangeGoldLog(const char* type, LPCHARACTER who, DWORD targetPid, YANG gold);
		void		HackLog(const char * c_pszHackName, const char * c_pszLogin, const char * c_pszName, const char * c_pszIP);
		void		HackLog(const char * c_pszHackName, LPCHARACTER ch);
		void		RequestInfoLog(DWORD pid, BYTE type, const char * c_pszArg1, const char* c_pszArg2);
		void		GoldBarLog(DWORD dwPID, DWORD dwItemID, GOLDBAR_HOW eHow, const char * c_pszHint);
		void		CubeLog(DWORD dwPID, DWORD x, DWORD y, DWORD item_vnum, DWORD item_uid, int item_count, bool success);
		void		GMCommandLog(DWORD dwPID, const char * szName, const char * szIP, BYTE byChannel, const char * szCommand);
		void		SpeedHackLog(DWORD pid, DWORD x, DWORD y, int hack_count);
		void		ChangeNameLog(DWORD pid, const char * old_name, const char * new_name, const char * ip);
		void		RefineLog(DWORD pid, const char * item_name, DWORD item_id, int item_refine_level, int is_success, const char * how);
		void		LevelLog(LPCHARACTER pChar, unsigned int level, unsigned int playhour);
		void		BootLog(const char * c_pszHostName, BYTE bChannel);
		void		QuestRewardLog(const char * c_pszQuestName, DWORD dwPID, DWORD dwLevel, long long llValue1, int iValue2, BYTE type);
		void		QuestStateLog(const char * c_pszQuestName, const char* c_pszQuestState, DWORD dwPID);
		void		DetailLoginLog(bool isLogin, LPCHARACTER ch);
		void		DetailLoginLog(bool isLogin, LPDESC d, DWORD pid);
		void		DragonSlayLog(DWORD dwGuildID, DWORD dwDragonVnum, DWORD dwStartTime, DWORD dwEndTime);
		void		ChatLog(DWORD where, DWORD who_id, const char* who_name, DWORD whom_id, const char* whom_name, const char* type, const char* msg, const char* ip);
		void		CaptchaLog(LPCHARACTER ch, const char* action, const char* code);
		void		VoucherCodeLog(LPCHARACTER ch, const char* code);
		void		GoldSessionLog(LPCHARACTER ch);
		void		ReportPlayerLog(LPCHARACTER who, LPCHARACTER target, BYTE reason);
		void		FishLog(DWORD playerId, DWORD itemVnum, ITEM_COUNT count, int rodLevel, int baitVnum);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		void		AcceLog(DWORD dwPID, DWORD x, DWORD y, DWORD item_vnum, DWORD item_uid, int item_count, int abs_chance, bool success);
#endif
		void		Query(const char* c_pszFormat, ...);

		size_t EscapeString(char* dst, size_t dstSize, const char *src, size_t srcSize);
	private:

		CAsyncSQL	m_sql;
		bool		m_bIsConnect;
};

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
