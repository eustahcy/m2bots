#ifndef __INC_TABLES_H__
#define __INC_TABLES_H__

#include "length.h"
#include "item_length.h"
#include "CommonDefines.h"

typedef	DWORD IDENT;

typedef DWORD	ITEM_COUNT;
typedef long long	YANG;

enum
{
	HEADER_GD_LOGIN				= 1,
	HEADER_GD_LOGOUT			= 2,

	HEADER_GD_PLAYER_LOAD		= 3,
	HEADER_GD_PLAYER_SAVE		= 4,
	HEADER_GD_PLAYER_CREATE		= 5,
	HEADER_GD_PLAYER_DELETE		= 6,

	HEADER_GD_LOGIN_KEY			= 7,
	// 8 empty
	HEADER_GD_BOOT				= 9,
	HEADER_GD_PLAYER_COUNT		= 10,
	HEADER_GD_QUEST_SAVE		= 11,
	HEADER_GD_SAFEBOX_LOAD		= 12,
	HEADER_GD_SAFEBOX_SAVE		= 13,
	HEADER_GD_SAFEBOX_CHANGE_SIZE	= 14,
	HEADER_GD_EMPIRE_SELECT		= 15,

	HEADER_GD_SAFEBOX_CHANGE_PASSWORD		= 16,
	HEADER_GD_SAFEBOX_CHANGE_PASSWORD_SECOND	= 17, // Not really a packet, used internal
	HEADER_GD_DIRECT_ENTER		= 18,

	HEADER_GD_GUILD_SKILL_UPDATE	= 19,
	HEADER_GD_GUILD_EXP_UPDATE		= 20,
	HEADER_GD_GUILD_ADD_MEMBER		= 21,
	HEADER_GD_GUILD_REMOVE_MEMBER	= 22,
	HEADER_GD_GUILD_CHANGE_GRADE	= 23,
	HEADER_GD_GUILD_CHANGE_MEMBER_DATA	= 24,
	HEADER_GD_GUILD_DISBAND		= 25,
	HEADER_GD_GUILD_WAR			= 26,
	HEADER_GD_GUILD_WAR_SCORE		= 27,
	HEADER_GD_GUILD_CREATE		= 28,

	HEADER_GD_SPECIAL_FLAG_SAVE = 29,

	HEADER_GD_ITEM_SAVE			= 30,
	HEADER_GD_ITEM_DESTROY		= 31,

	HEADER_GD_ADD_AFFECT		= 32,
	HEADER_GD_REMOVE_AFFECT		= 33,

	HEADER_GD_ITEM_FLUSH		= 35,

	HEADER_GD_PARTY_CREATE		= 36,
	HEADER_GD_PARTY_DELETE		= 37,
	HEADER_GD_PARTY_ADD			= 38,
	HEADER_GD_PARTY_REMOVE		= 39,
	HEADER_GD_PARTY_STATE_CHANGE	= 40,
	HEADER_GD_PARTY_HEAL_USE		= 41,

	HEADER_GD_RELOAD_PROTO		= 43,

	HEADER_GD_CHANGE_NAME		= 44,

	HEADER_GD_GUILD_CHANGE_LADDER_POINT	= 46,
	HEADER_GD_GUILD_USE_SKILL		= 47,

	HEADER_GD_REQUEST_EMPIRE_PRIV	= 48,
	HEADER_GD_REQUEST_GUILD_PRIV	= 49,

	HEADER_GD_MONEY_LOG				= 50,

	HEADER_GD_GUILD_DEPOSIT_MONEY				= 51,
	HEADER_GD_GUILD_WITHDRAW_MONEY				= 52,
	HEADER_GD_GUILD_WITHDRAW_MONEY_GIVE_REPLY	= 53,

	HEADER_GD_REQUEST_CHARACTER_PRIV	= 54,

	HEADER_GD_SET_EVENT_FLAG			= 55,

	HEADER_GD_PARTY_SET_MEMBER_LEVEL	= 56,

	HEADER_GD_GUILD_WAR_BET		= 57,

	HEADER_GD_CREATE_OBJECT		= 60,
	HEADER_GD_DELETE_OBJECT		= 61,
	HEADER_GD_UPDATE_LAND		= 62,

	HEADER_GD_MARRIAGE_ADD		= 70,
	HEADER_GD_MARRIAGE_UPDATE	= 71,
	HEADER_GD_MARRIAGE_REMOVE	= 72,

	HEADER_GD_WEDDING_REQUEST	= 73,
	HEADER_GD_WEDDING_READY		= 74,
	HEADER_GD_WEDDING_END		= 75,

	HEADER_GD_AUTH_LOGIN		= 100,
	HEADER_GD_LOGIN_BY_KEY		= 101,

	HEADER_GD_MALL_LOAD = 107,

	HEADER_GD_MYSHOP_PRICELIST_UPDATE	= 108,
	HEADER_GD_MYSHOP_PRICELIST_REQ		= 109,

	HEADER_GD_BLOCK_CHAT				= 110,
	HEADER_GD_USAGE						= 111,


	HEADER_GD_RELOAD_ADMIN			= 115,
	HEADER_GD_BREAK_MARRIAGE		= 116,
	HEADER_GD_ELECT_MONARCH			= 117,
	HEADER_GD_CANDIDACY				= 118,
	HEADER_GD_ADD_MONARCH_MONEY		= 119,
	HEADER_GD_TAKE_MONARCH_MONEY	= 120,
	HEADER_GD_COME_TO_VOTE			= 121,
	HEADER_GD_RMCANDIDACY			= 122,
	HEADER_GD_SETMONARCH			= 123,
	HEADER_GD_RMMONARCH				= 124,
	HEADER_GD_DEC_MONARCH_MONEY		= 125,

	HEADER_GD_CHANGE_MONARCH_LORD	= 126,

	HEADER_GD_REQ_CHANGE_GUILD_MASTER	= 129,

	HEADER_GD_REQ_SPARE_ITEM_ID_RANGE	= 130,

	HEADER_GD_UPDATE_HORSE_NAME		= 131,
	HEADER_GD_REQ_HORSE_NAME		= 132,

	HEADER_GD_DC					= 133,

	HEADER_GD_VALID_LOGOUT			= 134,
	HEADER_GD_RELOAD_ITEMSHOP		= 135,

	HEADER_GD_REQUEST_CHARGE_CASH	= 137,

	HEADER_GD_DELETE_AWARDID		= 138,	// delete gift notify icon

	HEADER_GD_UPDATE_CHANNELSTATUS	= 139,
	HEADER_GD_REQUEST_CHANNELSTATUS	= 140,
#ifdef ENABLE_MOVE_CHANNEL
	HEADER_GD_MOVE_CHANNEL = 141,
#endif

	HEADER_GD_MAINTENANCE = 143,
	HEADER_GD_SET_PREMIUM_PASS = 144,

	HEADER_GD_SPECIAL_SHOP_SEED = 145,
	HEADER_GD_BOT_PLAYER_LOAD	= 150,


#ifdef ENABLE_IKASHOP_RENEWAL
	HEADER_GD_NEW_OFFLINESHOP		= 153,
#endif

	HEADER_GD_REQUEST_USE_VOUCHER	= 154,

	HEADER_GD_SETUP			= 0xff,

	///////////////////////////////////////////////
	HEADER_DG_NOTICE			= 1,
	HEADER_DG_LOCALE_NOTICE		= 2,
	HEADER_DG_GUILD_WAR_NOTICE	= 3,

	HEADER_DG_LOGIN_SUCCESS			= 30,
	HEADER_DG_LOGIN_NOT_EXIST		= 31,
	HEADER_DG_LOGIN_WRONG_PASSWD	= 33,
	HEADER_DG_LOGIN_ALREADY			= 34,

	HEADER_DG_PLAYER_LOAD_SUCCESS	= 35,
	HEADER_DG_PLAYER_LOAD_FAILED	= 36,
	HEADER_DG_PLAYER_CREATE_SUCCESS	= 37,
	HEADER_DG_PLAYER_CREATE_ALREADY	= 38,
	HEADER_DG_PLAYER_CREATE_FAILED	= 39,
	HEADER_DG_PLAYER_DELETE_SUCCESS	= 40,
	HEADER_DG_PLAYER_DELETE_FAILED	= 41,

	HEADER_DG_ITEM_LOAD			= 42,

	HEADER_DG_BOOT				= 43,
	HEADER_DG_QUEST_LOAD		= 44,

	HEADER_DG_SAFEBOX_LOAD					= 45,
	HEADER_DG_SAFEBOX_CHANGE_SIZE			= 46,
	HEADER_DG_SAFEBOX_WRONG_PASSWORD		= 47,
	HEADER_DG_SAFEBOX_CHANGE_PASSWORD_ANSWER = 48,

	HEADER_DG_EMPIRE_SELECT		= 49,

	HEADER_DG_AFFECT_LOAD		= 50,
	HEADER_DG_MALL_LOAD			= 51,

	HEADER_DG_SPECIAL_FLAG_LOAD = 52,

	HEADER_DG_USAGE				= 53,

	HEADER_DG_DIRECT_ENTER		= 55,

	HEADER_DG_GUILD_SKILL_UPDATE	= 56,
	HEADER_DG_GUILD_SKILL_RECHARGE	= 57,
	HEADER_DG_GUILD_EXP_UPDATE		= 58,

	HEADER_DG_PARTY_CREATE		= 59,
	HEADER_DG_PARTY_DELETE		= 60,
	HEADER_DG_PARTY_ADD			= 61,
	HEADER_DG_PARTY_REMOVE		= 62,
	HEADER_DG_PARTY_STATE_CHANGE	= 63,
	HEADER_DG_PARTY_HEAL_USE		= 64,
	HEADER_DG_PARTY_SET_MEMBER_LEVEL	= 65,

	HEADER_DG_TIME			= 90,
	HEADER_DG_ITEM_ID_RANGE		= 91,

	HEADER_DG_GUILD_ADD_MEMBER		= 92,
	HEADER_DG_GUILD_REMOVE_MEMBER	= 93,
	HEADER_DG_GUILD_CHANGE_GRADE	= 94,
	HEADER_DG_GUILD_CHANGE_MEMBER_DATA	= 95,
	HEADER_DG_GUILD_DISBAND		= 96,
	HEADER_DG_GUILD_WAR			= 97,
	HEADER_DG_GUILD_WAR_SCORE		= 98,
	HEADER_DG_GUILD_TIME_UPDATE		= 99,
	HEADER_DG_GUILD_LOAD		= 100,
	HEADER_DG_GUILD_LADDER		= 101,
	HEADER_DG_GUILD_SKILL_USABLE_CHANGE	= 102,
	HEADER_DG_GUILD_MONEY_CHANGE	= 103,
	HEADER_DG_GUILD_WITHDRAW_MONEY_GIVE	= 104,

	HEADER_DG_SET_EVENT_FLAG		= 105,

	HEADER_DG_GUILD_WAR_RESERVE_ADD	= 106,
	HEADER_DG_GUILD_WAR_RESERVE_DEL	= 107,
	HEADER_DG_GUILD_WAR_BET		= 108,

	HEADER_DG_RELOAD_PROTO		= 120,
	HEADER_DG_CHANGE_NAME		= 121,

	HEADER_DG_AUTH_LOGIN		= 122,

	HEADER_DG_CHANGE_EMPIRE_PRIV	= 124,
	HEADER_DG_CHANGE_GUILD_PRIV		= 125,

	HEADER_DG_MONEY_LOG			= 126,

	HEADER_DG_CHANGE_CHARACTER_PRIV	= 127,

	HEADER_DG_CREATE_OBJECT		= 140,
	HEADER_DG_DELETE_OBJECT		= 141,
	HEADER_DG_UPDATE_LAND		= 142,

	HEADER_DG_RELOAD_ITEMSHOP = 143,

	HEADER_DG_MARRIAGE_ADD		= 150,
	HEADER_DG_MARRIAGE_UPDATE		= 151,
	HEADER_DG_MARRIAGE_REMOVE		= 152,

	HEADER_DG_WEDDING_REQUEST		= 153,
	HEADER_DG_WEDDING_READY		= 154,
	HEADER_DG_WEDDING_START		= 155,
	HEADER_DG_WEDDING_END		= 156,

	HEADER_DG_MYSHOP_PRICELIST_RES	= 157,
	HEADER_DG_RELOAD_ADMIN = 158,
	HEADER_DG_BREAK_MARRIAGE = 159,
	HEADER_DG_ELECT_MONARCH			= 160,
	HEADER_DG_CANDIDACY				= 161,
	HEADER_DG_ADD_MONARCH_MONEY		= 162,
	HEADER_DG_TAKE_MONARCH_MONEY	= 163,
	HEADER_DG_COME_TO_VOTE			= 164,
	HEADER_DG_RMCANDIDACY			= 165,
	HEADER_DG_SETMONARCH			= 166,
	HEADER_DG_RMMONARCH			= 167,
	HEADER_DG_DEC_MONARCH_MONEY = 168,

	HEADER_DG_CHANGE_MONARCH_LORD_ACK = 169,
	HEADER_DG_UPDATE_MONARCH_INFO	= 170,

	HEADER_DG_SPECIAL_SHOP_SEED = 171,

	HEADER_DG_ACK_CHANGE_GUILD_MASTER = 173,

	HEADER_DG_ACK_SPARE_ITEM_ID_RANGE = 174,

	HEADER_DG_UPDATE_HORSE_NAME 	= 175,
	HEADER_DG_ACK_HORSE_NAME		= 176,

	HEADER_DG_NEED_LOGIN_LOG		= 177,

	HEADER_DG_RESULT_CHARGE_CASH	= 179,
	HEADER_DG_ITEMAWARD_INFORMER	= 180,	//gift notify
	HEADER_DG_RESPOND_CHANNELSTATUS	= 181,
#ifdef ENABLE_IKASHOP_RENEWAL
	HEADER_DG_NEW_OFFLINESHOP		= 182,
#endif
	HEADER_DG_RESPOND_MOVE_CHANNEL = 183,

	HEADER_DG_USE_CODE_VOUCHER		= 184,

	HEADER_DG_MAP_LOCATIONS			= 0xfe,
	HEADER_DG_P2P					= 0xff,
};

/* ----------------------------------------------
 * table
 * ----------------------------------------------
 */

/* game Server -> DB Server */
#pragma pack(1)
enum ERequestChargeType
{
	ERequestCharge_Cash = 0,
	ERequestCharge_Marks,
};

typedef struct SRequestChargeCash
{
	DWORD		dwAID;		// id(primary key) - Account Table
	long		lAmount;
	long		lSecondAmount;
	ERequestChargeType	eChargeType;
} TRequestChargeCash;

struct TRequestUseCodeVoucher
{
	DWORD		dwPID;
	DWORD		dwAID;
	char		code[CODE_VOUCHER_MAX_LEN+1];
};

struct TRequestUseCodeVoucherResponse
{
	DWORD		dwPID;
	char		szCode[CODE_VOUCHER_MAX_LEN + 1];
	BYTE		bResponse;
};

typedef struct SSimplePlayer
{
	DWORD		dwID;
	char		szName[CHARACTER_NAME_MAX_LEN + 1];
	BYTE		byJob;
	BYTE		byLevel;
	DWORD		dwPlayMinutes;
	BYTE		byST, byHT, byDX, byIQ;
	DWORD		wMainPart; // @fixme502
	BYTE		bChangeName;
	DWORD		wHairPart; // @fixme502
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	DWORD		wAccePart;
#endif
	BYTE		bDummy[4];
	long		x, y;
	long		lAddr;
	WORD		wPort;
	BYTE		skill_group;
} TSimplePlayer;

typedef struct SAccountTable
{
	DWORD		id;
	char		login[LOGIN_MAX_LEN + 1];
	char		passwd[PASSWD_MAX_LEN + 1];
	char		social_id[SOCIAL_ID_MAX_LEN + 1];
	char		status[ACCOUNT_STATUS_MAX_LEN + 1];
	BYTE		bEmpire;
	int			iPremium;
	int			iMarriagePremiumChannelPass;
	BYTE		bDestChannel;
	char		hwid[HWID_MAX_LEN + 1];
	TSimplePlayer	players[PLAYER_PER_ACCOUNT];
} TAccountTable;


typedef struct SPacketDGCreateSuccess
{
	BYTE		bAccountCharacterIndex;
	TSimplePlayer	player;
} TPacketDGCreateSuccess;

typedef struct TPlayerItemAttribute
{
	BYTE	bType;
	short	sValue;

	bool operator==(const TPlayerItemAttribute& other) const {
		return bType == other.bType && sValue == other.sValue;
	}
} TPlayerItemAttribute;

typedef struct SPlayerItem // reflect all the changes in this structure in UserInterface/Packet.h
{
	DWORD	id;
	BYTE	window;
	WORD	pos;
	ITEM_COUNT	count;

	DWORD	vnum;
	long	alSockets[ITEM_SOCKET_MAX_NUM];

	TPlayerItemAttribute    aAttr[ITEM_ATTRIBUTE_MAX_NUM];

	DWORD	owner;
} TPlayerItem;

typedef struct SQuickslot
{
	BYTE	type;
	BYTE	pos;
} TQuickslot;

typedef struct SPlayerSkill
{
	BYTE	bMasterType;
	BYTE	bLevel;
	time_t	tNextRead;
} TPlayerSkill;

struct	THorseInfo
{
	BYTE	bLevel;
	BYTE	bRiding;
	short	sStamina;
	short	sHealth;
	DWORD	dwHorseHealthDropTime;
};

typedef struct SPlayerTable
{
	DWORD	id;

	char	name[CHARACTER_NAME_MAX_LEN + 1];
	char	ip[IP_ADDRESS_LENGTH + 1];

	WORD	job;
	BYTE	voice;

	BYTE	level;
	BYTE	level_step;
	short	st, ht, dx, iq;

	DWORD	exp;
	YANG	gold;

	BYTE	dir;
	INT		x, y, z;
	INT		lMapIndex;

	long	lExitX, lExitY;
	long	lExitMapIndex;

	// @fixme400
	int		hp;
	int		sp;

	short	sRandomHP;
	short	sRandomSP;

	int         playtime;

	short	stat_point;
	short	skill_point;
	short	sub_skill_point;
	short	horse_skill_point;
#ifdef ENABLE_CHEQUE_SYSTEM
	int		cheque;
#endif

	TPlayerSkill skills[SKILL_MAX_NUM];

	TQuickslot  quickslot[QUICKSLOT_MAX_NUM];

	BYTE	part_base;
	DWORD	parts[PART_MAX_NUM]; // @fixme502

	short	stamina;

	BYTE	skill_group;
	long	lAlignment;

	short	stat_reset_count;

	THorseInfo	horse;

	DWORD	logoff_interval;

	int		aiPremiumTimes[PREMIUM_MAX_NUM];
} TPlayerTable;

struct TPacketDGGuildWarNotice
{
	char	guild1[CHARACTER_NAME_MAX_LEN + 1];
	char	guild2[CHARACTER_NAME_MAX_LEN + 1];
	int		duration;
};

typedef struct SMobSkillLevel
{
	DWORD	dwVnum;
	BYTE	bLevel;
} TMobSkillLevel;

typedef struct SEntityTable
{
	DWORD dwVnum;
} TEntityTable;

typedef struct SMobTable : public SEntityTable
{
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	char	szLocaleName[CHARACTER_NAME_MAX_LEN + 1];

	BYTE	bType;			// Monster, NPC
	BYTE	bRank;			// PAWN, KNIGHT, KING
	BYTE	bBattleType;		// MELEE, etc..
	BYTE	bLevel;			// Level
	BYTE	bSize;

	DWORD	dwGoldMin;
	DWORD	dwGoldMax;
	DWORD	dwExp;
	DWORD	dwMaxHP;
	BYTE	bRegenCycle;
	BYTE	bRegenPercent;
	WORD	wDef;

	DWORD	dwAIFlag;
	DWORD	dwRaceFlag;
	DWORD	dwImmuneFlag;

	BYTE	bStr, bDex, bCon, bInt;
	DWORD	dwDamageRange[2];

	short	sAttackSpeed;
	short	sMovingSpeed;
	BYTE	bAggresiveHPPct;
	WORD	wAggressiveSight;
	WORD	wAttackRange;

	char	cEnchants[MOB_ENCHANTS_MAX_NUM];
	char	cResists[MOB_RESISTS_MAX_NUM];

	DWORD	dwResurrectionVnum;
	DWORD	dwDropItemVnum;

	BYTE	bMountCapacity;
	BYTE	bOnClickType;

	BYTE	bEmpire;
	char	szFolder[64 + 1];

	float	fDamMultiply;

	DWORD	dwSummonVnum;
	DWORD	dwDrainSP;
	DWORD	dwMobColor;
	DWORD	dwPolymorphItemVnum;

	TMobSkillLevel Skills[MOB_SKILL_MAX_NUM];

	BYTE	bBerserkPoint;
	BYTE	bStoneSkinPoint;
	BYTE	bGodSpeedPoint;
	BYTE	bDeathBlowPoint;
	BYTE	bRevivePoint;
	BYTE	fraction;
} TMobTable;

typedef struct SSkillTable
{
	DWORD	dwVnum;
	char	szName[32 + 1];
	BYTE	bType;
	BYTE	bMaxLevel;
	DWORD	dwSplashRange;

	char	szPointOn[64];
	char	szPointPoly[100 + 1];
	char	szSPCostPoly[100 + 1];
	char	szDurationPoly[100 + 1];
	char	szDurationSPCostPoly[100 + 1];
	char	szCooldownPoly[100 + 1];
	char	szMasterBonusPoly[100 + 1];
	//char	szAttackGradePoly[100 + 1];
	char	szGrandMasterAddSPCostPoly[100 + 1];
	DWORD	dwFlag;
	DWORD	dwAffectFlag;

	// Data for secondary skill
	char 	szPointOn2[64];
	char 	szPointPoly2[100 + 1];
	char 	szDurationPoly2[100 + 1];
	DWORD 	dwAffectFlag2;

	// Data for grand master point
	char 	szPointOn3[64];
	char 	szPointPoly3[100 + 1];
	char 	szDurationPoly3[100 + 1];
	DWORD 	dwAffectFlag3;

	BYTE	bLevelStep;
	BYTE	bLevelLimit;
	DWORD	preSkillVnum;
	BYTE	preSkillLevel;

	long	lMaxHit;
	char	szSplashAroundDamageAdjustPoly[100 + 1];

	BYTE	bSkillAttrType;

	DWORD	dwTargetRange;
} TSkillTable;

typedef struct SItemShopPromotion
{
	DWORD dwPrice;
	DWORD dwStartTime;
	DWORD dwEndTime;
} TItemShopPromotion;

typedef struct SItemShopTimeAuction
{
	DWORD dwCurrentBuyAmount; // progress if sent to client
	DWORD dwMaxBuyAmount;
	DWORD dwAccountLimit;
	DWORD dwStartTime;
	DWORD dwEndTime;
} TItemShopTimeAuction;

typedef struct SItemShopItem
{
	DWORD dwIndex;
	DWORD dwVnum;
	DWORD dwCount;
	DWORD dwPrice;
	BYTE bCurrency;
	BYTE bMinLevel;
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	TItemShopPromotion promotion;
	TItemShopTimeAuction timeAuction;
} TItemShopItem;

typedef struct SItemShopUpdateAuctionItem
{
	DWORD dwIndex;
	DWORD dwProgress;
	DWORD dwAccountLimit;
	DWORD dwStartTime;
	DWORD dwEndTime;
} TItemShopUpdateAuctionItem;

typedef struct SShopItemTable
{
	DWORD		vnum;
	ITEM_COUNT	count;

    TItemPos	pos;
	YANG		price;
#ifdef ENABLE_CHEQUE_SYSTEM
	DWORD		cheque;
#endif
	BYTE		display_pos;
} TShopItemTable;

typedef struct SShopTable
{
	DWORD		dwVnum;
	DWORD		dwNPCVnum;

	ITEM_COUNT	byItemCount;
	TShopItemTable	items[SHOP_HOST_ITEM_MAX_NUM];
} TShopTable;

typedef struct SSpecialFlagTable
{
	DWORD		dwPID;
	DWORD		dwAID;
	char		szFlag[SPECIAL_FLAG_MAX_LEN + 1];
	long long	llValue;
} TSpecialFlagTable;


#define QUEST_NAME_MAX_LEN	32
#define QUEST_STATE_MAX_LEN	64

typedef struct SQuestTable
{
	DWORD		dwPID;
	char		szName[QUEST_NAME_MAX_LEN + 1];
	char		szState[QUEST_STATE_MAX_LEN + 1];
	long		lValue;
} TQuestTable;

typedef struct SItemLimit
{
	BYTE	bType;
	long	lValue;
} TItemLimit;

typedef struct SItemApply
{
	BYTE	bType;
	long	lValue;
} TItemApply;

typedef struct SItemTable : public SEntityTable
{
	DWORD		dwVnumRange;
	char        szName[ITEM_NAME_MAX_LEN + 1];
	char	szLocaleName[ITEM_NAME_MAX_LEN + 1];
	BYTE	bType;
	BYTE	bSubType;

	BYTE        bWeight;
	BYTE	bSize;
	DWORD	dwMaxStack;

	DWORD	dwAntiFlags;
	DWORD	dwFlags;
	DWORD	dwWearFlags;
	DWORD	dwImmuneFlag;

	YANG       dwGold;
	YANG       dwShopBuyPrice;

	TItemLimit	aLimits[ITEM_LIMIT_MAX_NUM];
	TItemApply	aApplies[ITEM_APPLY_MAX_NUM];
	long        alValues[ITEM_VALUES_MAX_NUM];
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	DWORD	dwRefinedVnum;
	WORD	wRefineSet;
	BYTE	bAlterToMagicItemPct;
	BYTE	bSpecular;
	BYTE	bGainSocketPct;

	short int	sAddonType;

	char		cLimitRealTimeFirstUseIndex;
	char		cLimitTimerBasedOnWearIndex;
} TItemTable;

struct TItemAttrTable
{
	TItemAttrTable() :
		dwApplyIndex(0),
		dwProb(0)
	{
		szApply[0] = 0;
		memset(&lValues, 0, sizeof(lValues));
		memset(&bMaxLevelBySet, 0, sizeof(bMaxLevelBySet));
	}

	char    szApply[APPLY_NAME_MAX_LEN + 1];
	DWORD   dwApplyIndex;
	DWORD   dwProb;
	long    lValues[ITEM_ATTRIBUTE_MAX_LEVEL];
	BYTE    bMaxLevelBySet[ATTRIBUTE_SET_MAX_NUM];
};

typedef struct SConnectTable
{
	char	login[LOGIN_MAX_LEN + 1];
	IDENT	ident;
} TConnectTable;

typedef struct SLoginPacket
{
	char	login[LOGIN_MAX_LEN + 1];
	char	passwd[PASSWD_MAX_LEN + 1];
} TLoginPacket;

typedef struct SPlayerLoadPacket
{
	DWORD	account_id;
	DWORD	player_id;
	BYTE	account_index;
} TPlayerLoadPacket;

typedef struct SBotPlayerLoadPacket
{
	DWORD	player_id;
	BYTE	empire;
	DWORD	account_id;
} TBotPlayerLoadPacket;

typedef struct SPlayerCreatePacket
{
	char		login[LOGIN_MAX_LEN + 1];
	char		passwd[PASSWD_MAX_LEN + 1];
	DWORD		account_id;
	BYTE		account_index;
	TPlayerTable	player_table;
} TPlayerCreatePacket;

typedef struct SPlayerDeletePacket
{
	char	login[LOGIN_MAX_LEN + 1];
	DWORD	player_id;
	BYTE	account_index;
	//char	name[CHARACTER_NAME_MAX_LEN + 1];
	char	private_code[8];
} TPlayerDeletePacket;

typedef struct SLogoutPacket
{
	char	login[LOGIN_MAX_LEN + 1];
	char	passwd[PASSWD_MAX_LEN + 1];
} TLogoutPacket;

typedef struct SPlayerCountPacket
{
	DWORD	dwCount;
} TPlayerCountPacket;

#ifdef ENABLE_MOVE_CHANNEL
typedef struct SMoveChannel
{
	DWORD	dwAID;
	BYTE	bChannel;
	long	lDestX;
	long	lDestY;
	long	lMapIndex;
} TMoveChannel;

typedef struct SRespondMoveChannel
{
	BYTE	bChannel;
	long	lDestX;
	long	lDestY;
	long	lDestMapIndex;
	BYTE	bPremium;
} TRespondMoveChannel;
#endif

typedef struct SSafeboxTable
{
	DWORD	dwID;
	BYTE	bSize;
	YANG	dwGold;
	WORD	wItemCount;
} TSafeboxTable;

typedef struct SSafeboxChangeSizePacket
{
	DWORD	dwID;
	BYTE	bSize;
} TSafeboxChangeSizePacket;

typedef struct SSafeboxLoadPacket
{
	DWORD	dwID;
	char	szLogin[LOGIN_MAX_LEN + 1];
	char	szPassword[SAFEBOX_PASSWORD_MAX_LEN + 1];
} TSafeboxLoadPacket;

typedef struct SSafeboxChangePasswordPacket
{
	DWORD	dwID;
	char	szOldPassword[SAFEBOX_PASSWORD_MAX_LEN + 1];
	char	szNewPassword[SAFEBOX_PASSWORD_MAX_LEN + 1];
} TSafeboxChangePasswordPacket;

typedef struct SSafeboxChangePasswordPacketAnswer
{
	BYTE	flag;
} TSafeboxChangePasswordPacketAnswer;

typedef struct SEmpireSelectPacket
{
	DWORD	dwAccountID;
	BYTE	bEmpire;
} TEmpireSelectPacket;

typedef struct SPacketGDMaintenance
{
	BYTE bHeader;
	BYTE bState;
} TPacketGDMaintenance;

typedef struct SPacketGDSetup
{
	char	szPublicIP[16];	// Public IP which listen to users
	BYTE	bChannel;
	WORD	wListenPort;
	WORD	wP2PPort;
	long	alMaps[MAP_ALLOW_LIMIT];
	DWORD	dwLoginCount;
	BYTE	bAuthServer;
	BYTE	bPremiumChannel;
} TPacketGDSetup;

typedef struct SPacketDGMapLocations
{
	BYTE	bCount;
} TPacketDGMapLocations;

typedef struct SMapLocation
{
	long	alMaps[MAP_ALLOW_LIMIT];
	char	szHost[MAX_HOST_LENGTH + 1];
	WORD	wPort;
#ifdef ENABLE_MOVE_CHANNEL
	BYTE	channel;
#endif
} TMapLocation;

typedef struct SPacketDGP2P
{
	char	szHost[MAX_HOST_LENGTH + 1];
	WORD	wPort;
	BYTE	bChannel;
} TPacketDGP2P;

typedef struct SPacketGDDirectEnter
{
	char	login[LOGIN_MAX_LEN + 1];
	char	passwd[PASSWD_MAX_LEN + 1];
	BYTE	index;
} TPacketGDDirectEnter;

typedef struct SPacketDGDirectEnter
{
	TAccountTable accountTable;
	TPlayerTable playerTable;
} TPacketDGDirectEnter;

typedef struct SPacketGuildSkillUpdate
{
	DWORD guild_id;
	int amount;
	BYTE skill_levels[12];
	BYTE skill_point;
	BYTE save;
} TPacketGuildSkillUpdate;

typedef struct SPacketGuildExpUpdate
{
	DWORD guild_id;
	int amount;
} TPacketGuildExpUpdate;

typedef struct SPacketGuildChangeMemberData
{
	DWORD guild_id;
	DWORD pid;
	DWORD offer;
	BYTE level;
	BYTE grade;
} TPacketGuildChangeMemberData;

typedef struct SPacketDGLoginAlready
{
	char	szLogin[LOGIN_MAX_LEN + 1];
} TPacketDGLoginAlready;

typedef struct TPacketAffectElement
{
	DWORD	dwType;
	BYTE	bApplyOn;
	long	lApplyValue;
	DWORD	dwFlag;
	long	lDuration;
	long	lSPCost;
	DWORD	dwItemVnum;
} TPacketAffectElement;

typedef struct SPacketGDAddAffect
{
	DWORD			dwPID;
	TPacketAffectElement	elem;
} TPacketGDAddAffect;

typedef struct SPacketGDRemoveAffect
{
	DWORD	dwPID;
	DWORD	dwType;
	BYTE	bApplyOn;
} TPacketGDRemoveAffect;

typedef struct SPacketPartyCreate
{
	DWORD	dwLeaderPID;
} TPacketPartyCreate;

typedef struct SPacketPartyDelete
{
	DWORD	dwLeaderPID;
} TPacketPartyDelete;

typedef struct SPacketPartyAdd
{
	DWORD	dwLeaderPID;
	DWORD	dwPID;
	BYTE	bState;
} TPacketPartyAdd;

typedef struct SPacketPartyRemove
{
	DWORD	dwLeaderPID;
	DWORD	dwPID;
} TPacketPartyRemove;

typedef struct SPacketPartyStateChange
{
	DWORD	dwLeaderPID;
	DWORD	dwPID;
	BYTE	bRole;
	BYTE	bFlag;
} TPacketPartyStateChange;

typedef struct SPacketPartySetMemberLevel
{
	DWORD	dwLeaderPID;
	DWORD	dwPID;
	BYTE	bLevel;
} TPacketPartySetMemberLevel;

typedef struct SPacketGDBoot
{
    DWORD	dwItemIDRange[2];
	char	szIP[16];
} TPacketGDBoot;

typedef struct SPacketGuild
{
	DWORD	dwGuild;
	DWORD	dwInfo;
} TPacketGuild;

typedef struct SPacketGDGuildAddMember
{
	DWORD	dwPID;
	DWORD	dwGuild;
	BYTE	bGrade;
} TPacketGDGuildAddMember;

typedef struct SPacketDGGuildMember
{
	DWORD	dwPID;
	DWORD	dwGuild;
	BYTE	bGrade;
	BYTE	isGeneral;
	BYTE	bJob;
	BYTE	bLevel;
	DWORD	dwOffer;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketDGGuildMember;

typedef struct SPacketGuildWar
{
	BYTE	bType;
	BYTE	bWar;
	DWORD	dwGuildFrom;
	DWORD	dwGuildTo;
	YANG	lWarPrice;
	long	lInitialScore;
} TPacketGuildWar;

typedef struct SPacketGuildWarScore
{
	DWORD dwGuildGainPoint;
	DWORD dwGuildOpponent;
	long lScore;
	long lBetScore;
} TPacketGuildWarScore;

typedef struct SRefineMaterial
{
	DWORD vnum;
	ITEM_COUNT count;
} TRefineMaterial;

typedef struct SRefineTable
{
	//DWORD src_vnum;
	//DWORD result_vnum;
	DWORD id;
	BYTE material_count;
	int cost;
	int prob;
	TRefineMaterial materials[REFINE_MATERIAL_MAX_NUM];
} TRefineTable;

typedef struct SOfflineShopLimit
{
	BYTE channel;
	int mapIndex;
	int shopLimit;
	int shopPremiumLimit;
} TOfflineShopLimit;

typedef struct SCraftingItem
{
	DWORD vnum;
	DWORD itemVnum;
	ITEM_COUNT count;
	int price;
	int chance;
	TRefineMaterial materials[CRAFTING_MATERIAL_MAX_NUM];
	DWORD reqProgress;
	BYTE reqLevel;
	DWORD recipeVnum;
} TCraftingItem;

struct TItemReward
{
	DWORD vnum;
	ITEM_COUNT count;
	bool isSpecial;
};

struct TQuestRewardItem
{
	char questName[QUEST_REWARD_MAX_QUEST_NAME_LEN +1];
	DWORD exp;
	YANG gold;
	int alignment;
	TItemReward items[QUEST_REWARD_MAX_ITEMS];
	TItemReward warriorItems[QUEST_REWARD_MAX_ITEMS];
	TItemReward suraItems[QUEST_REWARD_MAX_ITEMS];
	TItemReward assassinItems[QUEST_REWARD_MAX_ITEMS];
	TItemReward shamanItems[QUEST_REWARD_MAX_ITEMS];
};


typedef struct SSpecialShopLimit
{
	BYTE type;
	int value;
} TSpecialShopLimit;

typedef struct SSpecialShopItem
{
	DWORD vnum;
	DWORD itemVnum;
	ITEM_COUNT count;
	int	rarePct;
	BYTE priceType;
	int price;
	TRefineMaterial items[SPECIAL_SHOP_MAX_ITEM_COUNT];
	int randomItemCount;
	int randomItemGroup;
	TSpecialShopLimit limits[SPECIAL_SHOP_MAX_LIMIT];
} TSpecialShopItem;

typedef struct SSpecialShopItemClient
{
	DWORD vnum;
	DWORD itemVnum;
	ITEM_COUNT count;
	int	rarePct;
	BYTE priceType;
	int price;
	TRefineMaterial items[SPECIAL_SHOP_MAX_ITEM_COUNT];
	TSpecialShopLimit limits[SPECIAL_SHOP_MAX_LIMIT];
} TSpecialShopItemClient;

struct TSpecialShopMetaData
{
	DWORD seed;
	DWORD lastUpdate;
	int	arg1;
};

typedef struct SSpecialShopTable
{
	DWORD vnum;
	DWORD items[SHOP_HOST_ITEM_MAX_NUM];
} TSpecialShopTable;

typedef struct SNameReservationTable
{
	DWORD dwOwner;
	char name[CHARACTER_NAME_MAX_LEN + 1];
} TNameReservationTable;

typedef struct SBanwordTable
{
	char szWord[BANWORD_MAX_LEN + 1];
} TBanwordTable;

typedef struct SPacketGDChangeName
{
	DWORD pid;
	char name[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGDChangeName;

typedef struct SPacketDGChangeName
{
	DWORD pid;
	char name[CHARACTER_NAME_MAX_LEN + 1];
} TPacketDGChangeName;

typedef struct SPacketGuildLadder
{
	DWORD dwGuild;
	long lLadderPoint;
	long lWin;
	long lDraw;
	long lLoss;
} TPacketGuildLadder;

typedef struct SPacketGuildLadderPoint
{
	DWORD dwGuild;
	long lChange;
} TPacketGuildLadderPoint;

typedef struct SPacketGuildUseSkill
{
	DWORD dwGuild;
	DWORD dwSkillVnum;
	DWORD dwCooltime;
} TPacketGuildUseSkill;

typedef struct SPacketGuildSkillUsableChange
{
	DWORD dwGuild;
	DWORD dwSkillVnum;
	BYTE bUsable;
} TPacketGuildSkillUsableChange;

typedef struct SPacketGDLoginKey
{
	DWORD dwAccountID;
	DWORD dwLoginKey;
} TPacketGDLoginKey;

typedef struct SPacketGDAuthLogin
{
	DWORD	dwID;
	DWORD	dwLoginKey;
	char	szLogin[LOGIN_MAX_LEN + 1];
	char	szSocialID[SOCIAL_ID_MAX_LEN + 1];
	DWORD	adwClientKey[4];
	int		iPremiumTimes[PREMIUM_MAX_NUM];
	int		iPremium;
	char	szHwid[HWID_MAX_LEN + 1];
} TPacketGDAuthLogin;

typedef struct SPacketGDLoginByKey
{
	char	szLogin[LOGIN_MAX_LEN + 1];
	DWORD	dwLoginKey;
	DWORD	adwClientKey[4];
	char	szIP[MAX_HOST_LENGTH + 1];
} TPacketGDLoginByKey;

typedef struct SPacketGiveGuildPriv
{
	BYTE type;
	int value;
	DWORD guild_id;
	time_t duration_sec;
} TPacketGiveGuildPriv;
typedef struct SPacketGiveEmpirePriv
{
	BYTE type;
	int value;
	BYTE empire;
	time_t duration_sec;
} TPacketGiveEmpirePriv;
typedef struct SPacketGiveCharacterPriv
{
	BYTE type;
	int value;
	DWORD pid;
} TPacketGiveCharacterPriv;
typedef struct SPacketRemoveGuildPriv
{
	BYTE type;
	DWORD guild_id;
} TPacketRemoveGuildPriv;
typedef struct SPacketRemoveEmpirePriv
{
	BYTE type;
	BYTE empire;
} TPacketRemoveEmpirePriv;

typedef struct SPacketDGChangeCharacterPriv
{
	BYTE type;
	int value;
	DWORD pid;
	BYTE bLog;
} TPacketDGChangeCharacterPriv;

typedef struct SPacketDGChangeGuildPriv
{
	BYTE type;
	int value;
	DWORD guild_id;
	BYTE bLog;
	time_t end_time_sec;
} TPacketDGChangeGuildPriv;

typedef struct SPacketDGChangeEmpirePriv
{
	BYTE type;
	int value;
	BYTE empire;
	BYTE bLog;
	time_t end_time_sec;
} TPacketDGChangeEmpirePriv;

typedef struct SPacketMoneyLog
{
	BYTE type;
	DWORD vnum;
	YANG gold;
} TPacketMoneyLog;

typedef struct SPacketGDGuildMoney
{
	DWORD dwGuild;
	YANG iGold;
} TPacketGDGuildMoney;

typedef struct SPacketDGGuildMoneyChange
{
	DWORD dwGuild;
	YANG iTotalGold;
} TPacketDGGuildMoneyChange;

typedef struct SPacketDGGuildMoneyWithdraw
{
	DWORD dwGuild;
	YANG iChangeGold;
} TPacketDGGuildMoneyWithdraw;

typedef struct SPacketGDGuildMoneyWithdrawGiveReply
{
	DWORD dwGuild;
	YANG iChangeGold;
	BYTE bGiveSuccess;
} TPacketGDGuildMoneyWithdrawGiveReply;

typedef struct SPacketSetEventFlag
{
	char	szFlagName[EVENT_FLAG_NAME_MAX_LEN + 1];
	long	lValue;
} TPacketSetEventFlag;

typedef struct SPacketLoginOnSetup
{
	DWORD   dwID;
	char    szLogin[LOGIN_MAX_LEN + 1];
	char    szSocialID[SOCIAL_ID_MAX_LEN + 1];
	char    szHost[MAX_HOST_LENGTH + 1];
	DWORD   dwLoginKey;
	DWORD   adwClientKey[4];
} TPacketLoginOnSetup;

typedef struct SPacketGDCreateObject
{
	DWORD	dwVnum;
	DWORD	dwLandID;
	INT		lMapIndex;
	INT	 	x, y;
	float	xRot;
	float	yRot;
	float	zRot;
} TPacketGDCreateObject;

typedef struct SGuildReserve
{
	DWORD       dwID;
	DWORD       dwGuildFrom;
	DWORD       dwGuildTo;
	DWORD       dwTime;
	BYTE        bType;
	YANG        lWarPrice;
	long        lInitialScore;
	bool        bStarted;
	YANG	dwBetFrom;
	YANG	dwBetTo;
	long	lPowerFrom;
	long	lPowerTo;
	long	lHandicap;
} TGuildWarReserve;

typedef struct SPacketGDGuildWarBet
{
	DWORD	dwWarID;
	char	szLogin[LOGIN_MAX_LEN + 1];
	YANG	dwGold;
	DWORD	dwGuild;
} TPacketGDGuildWarBet;

// Marriage

typedef struct SPacketMarriageAdd
{
	DWORD dwPID1;
	DWORD dwPID2;
	time_t tMarryTime;
	char szName1[CHARACTER_NAME_MAX_LEN + 1];
	char szName2[CHARACTER_NAME_MAX_LEN + 1];
} TPacketMarriageAdd;

typedef struct SPacketMarriageUpdate
{
	DWORD dwPID1;
	DWORD dwPID2;
	INT  iLovePoint;
	BYTE  byMarried;
} TPacketMarriageUpdate;

typedef struct SPacketMarriageRemove
{
	DWORD dwPID1;
	DWORD dwPID2;
} TPacketMarriageRemove;

typedef struct SPacketWeddingRequest
{
	DWORD dwPID1;
	DWORD dwPID2;
} TPacketWeddingRequest;

typedef struct SPacketWeddingReady
{
	DWORD dwPID1;
	DWORD dwPID2;
	DWORD dwMapIndex;
} TPacketWeddingReady;

typedef struct SPacketWeddingStart
{
	DWORD dwPID1;
	DWORD dwPID2;
} TPacketWeddingStart;

typedef struct SPacketWeddingEnd
{
	DWORD dwPID1;
	DWORD dwPID2;
} TPacketWeddingEnd;

typedef struct SPacketMyshopPricelistHeader
{
	DWORD	dwOwnerID;
	BYTE	byCount;
} TPacketMyshopPricelistHeader;

typedef struct SItemPriceInfo
{
	DWORD	dwVnum;
	YANG	dwPrice;
} TItemPriceInfo;

typedef struct SItemPriceListTable
{
	DWORD	dwOwnerID;
	BYTE	byCount;

	TItemPriceInfo	aPriceInfo[SHOP_PRICELIST_MAX_NUM];
} TItemPriceListTable;

typedef struct SPacketBlockChat
{
	char szName[CHARACTER_NAME_MAX_LEN + 1];
	long lDuration;
} TPacketBlockChat;

//ADMIN_MANAGER
typedef struct TAdminInfo
{
	int m_ID;
	char m_szAccount[32];
	char m_szName[32];
	char m_szContactIP[16];
	char m_szServerIP[16];
	int m_Authority;
} tAdminInfo;
//END_ADMIN_MANAGER

//BOOT_LOCALIZATION
struct tLocale
{
	char szValue[32];
	char szKey[32];
};
//BOOT_LOCALIZATION

//RELOAD_ADMIN
typedef struct SPacketReloadAdmin
{
	char szIP[16];
} TPacketReloadAdmin;
//END_RELOAD_ADMIN

typedef struct TMonarchInfo
{
	DWORD pid[4];
	YANG money[4];
	char name[4][32];
	char date[4][32];
} MonarchInfo;

typedef struct TMonarchElectionInfo
{
	DWORD pid;
	DWORD selectedpid;
	char date[32];
} MonarchElectionInfo;

typedef struct tMonarchCandidacy
{
	DWORD pid;
	char name[32];
	char date[32];
} MonarchCandidacy;

typedef struct tChangeMonarchLord
{
	BYTE bEmpire;
	DWORD dwPID;
} TPacketChangeMonarchLord;

typedef struct tChangeMonarchLordACK
{
	BYTE bEmpire;
	DWORD dwPID;
	char szName[32];
	char szDate[32];
} TPacketChangeMonarchLordACK;

typedef struct tChangeGuildMaster
{
	DWORD dwGuildID;
	DWORD idFrom;
	DWORD idTo;
} TPacketChangeGuildMaster;

typedef struct tItemIDRange
{
	DWORD dwMin;
	DWORD dwMax;
	DWORD dwUsableItemIDMin;
} TItemIDRangeTable;

typedef struct tUpdateHorseName
{
	DWORD dwPlayerID;
	char szHorseName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketUpdateHorseName;

typedef struct SSetPremiumPass
{
	DWORD	dwAid;
	DWORD	dwPremiumTime;
} TPacketSetPremiumPass;

typedef struct tDC
{
	char	login[LOGIN_MAX_LEN + 1];
} TPacketDC;

typedef struct tNeedLoginLogInfo
{
	DWORD dwPlayerID;
} TPacketNeedLoginLogInfo;

typedef struct tItemAwardInformer
{
	char	login[LOGIN_MAX_LEN + 1];
	char	command[20];
	unsigned int vnum;
} TPacketItemAwardInfromer;

typedef struct tDeleteAwardID
{
	DWORD dwID;
} TPacketDeleteAwardID;

typedef struct SChannelStatus
{
	WORD nPort; // backward change for client @fixme024
	BYTE bStatus;
} TChannelStatus;
#ifdef ENABLE_IKASHOP_RENEWAL
//common
struct TPacketGDNewIkarusShop 
{
	BYTE bSubHeader;
};

struct TPacketDGNewIkarusShop 
{
	BYTE bSubHeader;
};


namespace ikashop
{
	//patch 08-03-2020
	enum class ExpirationType {
		EXPIRE_NONE,
		EXPIRE_REAL_TIME,
		EXPIRE_REAL_TIME_FIRST_USE,
	};

	struct TPriceInfo
	{
		long long yang = 0;
#ifdef ENABLE_CHEQUE_SYSTEM
		int cheque = 0;
#endif
		bool operator < (const TPriceInfo& rItem) const
		{
			return GetTotalYangAmount() < rItem.GetTotalYangAmount();
		}

		long long GetTotalYangAmount() const{
			long long total = yang;
#ifdef ENABLE_CHEQUE_SYSTEM
			total += (long long) YANG_PER_CHEQUE * (long long) cheque;
#endif
			return total;
		}
	};

	struct TShopPlayerItem : public TPlayerItem
	{
		ExpirationType expiration;
	};

	struct TShopItem : public TShopPlayerItem
	{
		TPriceInfo price;
	};

#ifdef ENABLE_IKASHOP_ENTITIES
	struct TShopSpawn
	{
		int x;
		int y;
		int map;
		BYTE channel;
	};
#endif

	struct TOfferInfo 
	{
		DWORD offerid;
		DWORD ownerid;
		DWORD itemid;
		DWORD buyerid;
		TPriceInfo price;
		bool notified;
		bool accepted;
		char buyername[CHARACTER_NAME_MAX_LEN+1];
		int64_t datetime;
	};

	//AUCTION
	struct TAuctionInfo : public TShopItem {
		char ownername[CHARACTER_NAME_MAX_LEN + 1];
		DWORD duration;
	};


	struct TAuctionOfferInfo {
		TPriceInfo price;
		DWORD ownerid;
		DWORD buyerid;
		char buyername[CHARACTER_NAME_MAX_LEN + 1];
		bool seenflag;
		int64_t datetime;
	};

	struct TValutesInfo
	{
		long long yang = 0;

		void operator +=(const TValutesInfo& r){
			yang += r.yang;
		}

		void operator -=(const TValutesInfo& r){
			yang -= r.yang;
		}
	};

	struct TShopInfo 
	{
		DWORD ownerid;
		DWORD duration;
		char name[OFFLINE_SHOP_NAME_MAX_LEN + 1];
		char ownerName[PLAYER_NAME_MAX_LEN + 1];
		DWORD count;
#ifdef ENABLE_IKASHOP_ENTITIES
		TShopSpawn spawn;
#endif
		BYTE unlockCount;
		bool isPremium;
	};

#ifdef EXTEND_IKASHOP_PRO
	enum class ENotificationType : uint8_t
	{
		SELLER_SOLD_ITEM,
		SELLER_NEW_OFFER_ON_ITEM,
		SELLER_SHOP_EXPIRED,
		SELLER_ITEM_EXPIRED,
		SELLER_AUCTION_NEW_OFFER,
		SELLER_AUCTION_EXPIRED,
		SELLER_AUCTION_SOLD,
		SELLER_OFFER_CANCELLED,
		BUYER_OFFER_DENIED,
		BUYER_OFFER_ACCEPTED,
		BUYER_OFFER_EXPIRED,
		BUYER_AUCTION_BUY,
		BUYER_AUCTION_RAISE,
		BUYER_AUCTION_LOST,
	};

	struct TNotificationInfo
	{
		DWORD id;
		ENotificationType type;
		DWORD what;
		char who[OFFLINE_SHOP_NAME_MAX_LEN + 1];
		char format[OFFLINESHOP_NOTIFICATION_FORMAT_LEN];
		int64_t datetime;
		bool seenflag;
	};
#endif


	// ### GAME TO DB ###

	enum eNewIkarusShopSubHeaderGD
	{
		SUBHEADER_GD_BUY_ITEM = 0,
		SUBHEADER_GD_BUY_LOCK_ITEM,
		SUBHEADER_GD_EDIT_ITEM,
		SUBHEADER_GD_REMOVE_ITEM,
		SUBHEADER_GD_ADD_ITEM,

		SUBHEADER_GD_SHOP_FORCE_HARD_CLOSE,
		SUBHEADER_GD_SHOP_FORCE_SOFT_CLOSE,
		SUBHEADER_GD_SHOP_CREATE_NEW,
		SUBHEADER_GD_SHOP_CHANGE_NAME,


		SUBHEADER_GD_OFFER_CREATE,
		SUBHEADER_GD_OFFER_NOTIFIED,
		SUBHEADER_GD_OFFER_ACCEPT,
		SUBHEADER_GD_OFFER_CANCEL,

		SUBHEADER_GD_SAFEBOX_GET_ITEM,
		SUBHEADER_GD_SAFEBOX_GET_VALUTES,
		SUBHEADER_GD_SAFEBOX_ADD_ITEM,
		SUBHEADER_GD_SAFEBOX_GET_TIME_FEEDBACK,

		//AUCTION
		SUBHEADER_GD_AUCTION_CREATE,
		SUBHEADER_GD_AUCTION_ADD_OFFER,
		SUBHEADER_GD_AUCTION_CANCEL,
		SUBHEADER_GD_AUCTION_OFFER_SEEN,

#ifdef EXTEND_IKASHOP_PRO
		SUBHEADER_GD_NOTIFICATION_SEEN,
		SUBHEADER_GD_SHOP_RESTORE_DURATION,
#ifdef ENABLE_IKASHOP_ENTITIES
		SUBHEADER_GD_MOVE_SHOP_ENTITY,
#endif
		SUBHEADER_GD_OWNER_ONLINE,
		SUBHEADER_GD_SHOP_LOCKED_SLOT_STATE,
		SUBHEADER_GD_LOAD_EXPIRED_SHOP,
#endif
	};

	struct TSubPacketGDBuyItem {
		DWORD ownerid;
		DWORD itemid;
		DWORD guestid;
		bool success;
		int tax;
	};

	struct TSubPacketGDLockBuyItem {
		DWORD ownerid;
		DWORD itemid;
		DWORD guestid;
		long long seenprice;
	};

	struct TSubPacketGDEditItem {
		DWORD ownerid;
		DWORD itemid;
		TPriceInfo price;
	};

	struct TSubPacketGDRemoveItem {
		DWORD ownerid;
		DWORD itemid;
	};

	struct TSubPacketGDAddItem {
		DWORD ownerid;
		TShopItem sitem;
	};

	struct TSubPacketGDShopForceClose {
		DWORD ownerid;
		bool gm;
	};

	struct TSubPacketGDShopCreateNew {
		TShopInfo shop;
		TShopItem items[SHOP_PLAYER_HOST_ITEM_MAX_NUM];
	};

	struct TSubPacketGDShopChangeName {
		DWORD ownerid;
		char name[OFFLINE_SHOP_NAME_MAX_LEN + 1];
	};

	struct TSubPacketGDOwnerOnline {
		DWORD ownerid;
		bool isOnline;
	};

	struct TSubPacketGDOfferCreate {
		DWORD ownerid;
		DWORD itemid;
		DWORD vnum;
		DWORD count;
		TOfferInfo offer;
	};

	struct TSubPacketGDOfferCancel {
		DWORD offerid;
		DWORD ownerid;
		DWORD requester;
	};

	struct TSubPacketGDOfferNotified {
		DWORD ownerid;
		DWORD offerid;
	};

	struct TSubPacketGDOfferAccept {
		DWORD ownerid;
		DWORD offerid;
	};

	struct TSubPacketGDSafeboxGetItem {
		DWORD ownerid;
		DWORD itemid;
	};

	struct TSubPacketGDSafeboxAddItem {
		DWORD ownerid;
		DWORD itemid;
		TShopPlayerItem item;
	};

	struct TSubPacketGDSafeboxGetItemFeedback {
		DWORD ownerid;
		DWORD itemid;
		bool result;
	};

	struct TSubPacketGDSafeboxGetValutes {
		DWORD ownerid;
		TValutesInfo valute;
	};

	struct TSubPacketGDLoadExpiredShop {
		DWORD owner;
	};

	//AUCTION
	struct TSubPacketGDAuctionCreate {
		TAuctionInfo auction;
	};

	struct TSubPacketGDAuctionAddOffer {
		TAuctionOfferInfo offer;
	};

	struct TSubPacketGDAuctionOfferSeen {
		DWORD ownerid;
		DWORD buyerid;
		long long price;
	};

#ifdef EXTEND_IKASHOP_PRO
	struct TSubPacketGDNotificationSeen
	{
		DWORD owner;
		DWORD id;
	};
#ifdef ENABLE_IKASHOP_ENTITIES
	struct TSubPacketGDMoveShopEntity
	{
		DWORD owner;
		TShopSpawn spawn;
	};
#endif
#endif

	struct TSubPacketGDLockedSlotState {
		DWORD ownerid;
		BYTE unlockCount;
		bool isPremium;
	};




	// ### DB TO GAME

	enum eSubHeaderDGNewIkarusShop
	{
		SUBHEADER_DG_BUY_ITEM,
		SUBHEADER_DG_LOCKED_BUY_ITEM,
		SUBHEADER_DG_EDIT_ITEM,
		SUBHEADER_DG_REMOVE_ITEM,
		SUBHEADER_DG_ADD_ITEM,

		SUBHEADER_DG_SHOP_FORCE_CLOSE,
		SUBHEADER_DG_SHOP_CREATE_NEW,
		SUBHEADER_DG_SHOP_CHANGE_NAME,
		SUBHEADER_DG_SHOP_EXPIRED,


		SUBHEADER_DG_OFFER_CREATE,
		SUBHEADER_DG_OFFER_NOTIFIED,
		SUBHEADER_DG_OFFER_ACCEPT,
		SUBHEADER_DG_OFFER_CANCEL,

		SUBHEADER_DG_LOAD_TABLES,

		SUBHEADER_DG_SAFEBOX_ADD_ITEM,
		SUBHEADER_DG_SAFEBOX_ADD_VALUTES,
		SUBHEADER_DG_SAFEBOX_LOAD,
		SUBHEADER_DG_SAFEBOX_EXPIRED_ITEM,
		SUBHEADER_DG_SAFEBOX_GET_ITEM_CONFIRM,

		//AUCTION
		SUBHEADER_DG_AUCTION_CREATE,
		SUBHEADER_DG_AUCTION_ADD_OFFER,
		SUBHEADER_DG_AUCTION_EXPIRED,
		SUBHEADER_DG_AUCTION_OFFER_SEEN,

#ifdef EXTEND_IKASHOP_PRO
		SUBHEADER_DG_NOTIFICATION_LOAD,
		SUBHEADER_DG_NOTIFICATION_FORWARD,
		SUBHEADER_DG_SHOP_RESTORE_DURATION,
#ifdef ENABLE_IKASHOP_ENTITIES
		SUBHEADER_DG_MOVE_SHOP_ENTITY,
#endif
#endif
		SUBHEADER_DG_SHOP_OWNER_ONLINE,
		SUBHEADER_DG_SHOP_LOCKED_SLOT_STATE,

		SUBHEADER_DG_LOAD_EXPIRED_SHOP,
	};

	struct TSubPacketDGBuyItem {
		DWORD ownerid;
		DWORD itemid;
		DWORD buyerid;
		bool requester;
	};

	struct TSubPacketDGLockedBuyItem {
		DWORD ownerid;
		DWORD itemid;
		DWORD buyerid;
	};

	struct TSubPacketDGEditItem {
		DWORD ownerid;
		DWORD itemid;
		TPriceInfo price;
	};

	struct TSubPacketDGRemoveItem {
		DWORD ownerid;
		DWORD itemid;
		bool requester = false;
	};

	struct TSubPacketDGAddItem {
		DWORD ownerid;
		DWORD itemid;
		TShopItem item;
	};

	struct TSubPacketDGShopForceClose {
		DWORD ownerid;
	};

	struct TSubPacketDGShopCreateNew {
		TShopInfo shop;
		TShopItem items[SHOP_PLAYER_HOST_ITEM_MAX_NUM];
	};

	struct TSubPacketDGShopChangeName {
		DWORD ownerid;
		char name[OFFLINE_SHOP_NAME_MAX_LEN + 1];
	};

	struct TSubPacketDGOfferCreate {
		DWORD ownerid;
		DWORD itemid;
		TOfferInfo offer;
	};

	struct TSubPacketDGOfferCancel {
		DWORD offerid;
		DWORD ownerid;
		bool removing;
	};

	struct TSubPacketDGOfferNotified {
		DWORD ownerid;
		DWORD offerid;
	};

	struct TSubPacketDGOfferAccept {
		DWORD ownerid;
		DWORD offerid;
	};

	struct TSubPacketDGLoadTables {
		DWORD shopcount;
		DWORD offercount;
		DWORD auctioncount;
		DWORD auctionoffercount;
	};

	struct TSubPacketDGShopExpired {
		DWORD ownerid;
	};

	struct TSubPacketDGSafeboxAddItem {
		DWORD ownerid;
		DWORD itemid;
		TShopPlayerItem item;
	};

	struct TSubPacketDGSafeboxAddValutes {
		DWORD ownerid;
		TValutesInfo valute;
	};

	struct TSubPacketDGSafeboxLoad {
		DWORD ownerid;
		TValutesInfo valute;
		DWORD itemcount;
	};

	struct TSubPacketDGSafeboxExpiredItem {
		DWORD ownerid;
		DWORD itemid;
	};

	struct TSubPacketDGSafeboxGetItemConfirm {
		DWORD ownerid;
		DWORD itemid;
	};

	//AUCTION
	struct TSubPacketDGAuctionCreate {
		TAuctionInfo auction;
	};

	struct TSubPacketDGAuctionAddOffer {
		TAuctionOfferInfo offer;
	};

	struct TSubPacketDGAuctionExpired {
		DWORD ownerid;
	};

	struct TSubPacketDGAuctionOfferSeen {
		DWORD ownerid;
		DWORD buyerid;
		long long price;
	};

#ifdef EXTEND_IKASHOP_PRO
	struct TSubPacketDGNotificationLoad 
	{
		DWORD ownerid;
		DWORD count;
	};

	struct TSubPacketDGNotificationForward : public TNotificationInfo
	{
		DWORD ownerid;
	};
#ifdef ENABLE_IKASHOP_ENTITIES
	struct TSubPacketDGMoveShopEntity
	{
		DWORD owner;
		TShopSpawn spawn;
	};
#endif
#endif

	struct TSubPacketDGOwnerOnline
	{
		DWORD	owner;
		bool	isOnline;
	};

	struct TSubPacketDGLockedSlotState
	{
		DWORD	owner;
		BYTE	unlockCount;
		bool	isPremium;
	};

	struct TSubPacketDGLoadExpiredShop
	{
		DWORD	owner;
		DWORD	itemCount;
	};
}

#endif

struct TPacketGDSpecialShopSeed
{
	DWORD shopVnum;
	TSpecialShopMetaData meta;
};

struct TPacketDGSpecialShopSeed
{
	DWORD shopVnum;
	TSpecialShopMetaData meta;
};

struct TPacketGDUsage
{
	DWORD invokerPid;
};

struct TPacketDGUsage
{
	DWORD invokerPid;
	DWORD usageCur;
	DWORD usageAvg;
	DWORD usageMax;
};

#pragma pack()
#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by Metin2hub.com
