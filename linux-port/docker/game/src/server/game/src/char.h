#ifndef __INC_METIN_II_CHAR_H__
#define __INC_METIN_II_CHAR_H__

#include <boost/unordered_map.hpp>
#include "../../common/stl.h"
#include "entity.h"
#include "FSM.h"
#include "horse_rider.h"
#include "vid.h"
#include "constants.h"
#include "affect.h"
#include "affect_flag.h"
#include "cube.h"
#include "mining.h"
#include "../../common/CommonDefines.h"
#include <array>
#include <utility>
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
#include <vector>
#endif
#ifdef USE_PET_SEAL_ON_LOGIN
#include <unordered_set>
#endif

#define ENABLE_ANTI_CMD_FLOOD
#define ENABLE_OPEN_SHOP_WITH_ARMOR
#define ENABLE_SKILL_COOLDOWN_CHECK

enum eMountType {MOUNT_TYPE_NONE=0, MOUNT_TYPE_NORMAL=1, MOUNT_TYPE_COMBAT=2, MOUNT_TYPE_MILITARY=3};
eMountType GetMountLevelByVnum(DWORD dwMountVnum, bool IsNew);
const DWORD GetRandomSkillVnum(BYTE bJob = JOB_MAX_NUM);

class CBuffOnAttributes;
class CPetSystem;
#ifdef ENABLE_IKASHOP_RENEWAL
namespace ikashop
{
	class CShop;
	class CShopSafebox;
	class CAuction;
}
#endif

#define INSTANT_FLAG_DEATH_PENALTY		(1 << 0)
#define INSTANT_FLAG_SHOP			(1 << 1)
#define INSTANT_FLAG_EXCHANGE			(1 << 2)
#define INSTANT_FLAG_STUN			(1 << 3)
#define INSTANT_FLAG_NO_REWARD			(1 << 4)

#define AI_FLAG_NPC				(1 << 0)
#define AI_FLAG_AGGRESSIVE			(1 << 1)
#define AI_FLAG_HELPER				(1 << 2)
#define AI_FLAG_STAYZONE			(1 << 3)

#define SET_OVER_TIME(ch, time)	(ch)->SetOverTime(time)

extern int g_nPortalLimitTime;

enum
{
	MAIN_RACE_WARRIOR_M,
	MAIN_RACE_ASSASSIN_W,
	MAIN_RACE_SURA_M,
	MAIN_RACE_SHAMAN_W,
	MAIN_RACE_WARRIOR_W,
	MAIN_RACE_ASSASSIN_M,
	MAIN_RACE_SURA_W,
	MAIN_RACE_SHAMAN_M,
#ifdef ENABLE_WOLFMAN_CHARACTER
	MAIN_RACE_WOLFMAN_M,
#endif
	MAIN_RACE_MAX_NUM,
};

enum
{
	POISON_LENGTH = 30,
#ifdef ENABLE_WOLFMAN_CHARACTER
	BLEEDING_LENGTH = 30,
#endif
	AI_CHANGE_ATTACK_POISITION_TIME_NEAR = 10000,
	AI_CHANGE_ATTACK_POISITION_TIME_FAR = 1000,
	AI_CHANGE_ATTACK_POISITION_DISTANCE = 100,
	SUMMON_MONSTER_COUNT = 3,
	STAMINA_PER_STEP = 1,
	STAMINA_PER_SPRINT_STEP = 2,
};

enum EDamageType
{
	DAMAGE_TYPE_NONE,
	DAMAGE_TYPE_NORMAL,
	DAMAGE_TYPE_NORMAL_RANGE,
	DAMAGE_TYPE_MELEE,
	DAMAGE_TYPE_RANGE,
	DAMAGE_TYPE_FIRE,
	DAMAGE_TYPE_ICE,
	DAMAGE_TYPE_ELEC,
	DAMAGE_TYPE_MAGIC,
	DAMAGE_TYPE_POISON,
	DAMAGE_TYPE_SPECIAL,
#ifdef ENABLE_WOLFMAN_CHARACTER
	DAMAGE_TYPE_BLEEDING,
#endif
	DAMAGE_TYPE_SYSTEM,
};

enum DamageFlag
{
	DAMAGE_NORMAL	= (1 << 0),
	DAMAGE_POISON	= (1 << 1),
	DAMAGE_DODGE	= (1 << 2),
	DAMAGE_BLOCK	= (1 << 3),
	DAMAGE_PENETRATE= (1 << 4),
	DAMAGE_CRITICAL = (1 << 5),
#if defined(ENABLE_WOLFMAN_CHARACTER) && !defined(USE_MOB_BLEEDING_AS_POISON)
	DAMAGE_BLEEDING	= (1 << 6),
#endif
};

enum EPKModes
{
	PK_MODE_PEACE,
	PK_MODE_REVENGE,
	PK_MODE_FREE,
	PK_MODE_PROTECT,
	PK_MODE_GUILD,
	PK_MODE_MAX_NUM
};

enum EPositions
{
	POS_DEAD,
	POS_SLEEPING,
	POS_RESTING,
	POS_SITTING,
	POS_FISHING,
	POS_FIGHTING,
	POS_MOUNTING,
	POS_STANDING
};

enum EBlockAction
{
	BLOCK_EXCHANGE		= (1 << 0),
	BLOCK_PARTY_INVITE		= (1 << 1),
	BLOCK_GUILD_INVITE		= (1 << 2),
	BLOCK_WHISPER		= (1 << 3),
	BLOCK_MESSENGER_INVITE	= (1 << 4),
	BLOCK_PARTY_REQUEST		= (1 << 5),
};

// <Factor> Dynamically evaluated CHARACTER* equivalent.
// Referring to SCharDeadEventInfo.
struct DynamicCharacterPtr {
	DynamicCharacterPtr() : is_pc(false), id(0) {}
	DynamicCharacterPtr(const DynamicCharacterPtr& o)
		: is_pc(o.is_pc), id(o.id) {}

	// Returns the LPCHARACTER found in CHARACTER_MANAGER.
	LPCHARACTER Get() const;
	// Clears the current settings.
	void Reset() {
		is_pc = false;
		id = 0;
	}

	// Basic assignment operator.
	DynamicCharacterPtr& operator=(const DynamicCharacterPtr& rhs) {
		is_pc = rhs.is_pc;
		id = rhs.id;
		return *this;
	}
	// Supports assignment with LPCHARACTER type.
	DynamicCharacterPtr& operator=(LPCHARACTER character);
	// Supports type casting to LPCHARACTER.
	operator LPCHARACTER() const {
		return Get();
	}

	bool is_pc;
	uint32_t id;
};

typedef struct character_point
{
	long			points[POINT_MAX_NUM];

	BYTE			job;
	BYTE			voice;

	BYTE			level;
	DWORD			exp;
	YANG			gold;
#ifdef ENABLE_CHEQUE_SYSTEM
	int				cheque;
#endif
	int				hp;
	int				sp;

	int				iRandomHP;
	int				iRandomSP;

	int				stamina;

	BYTE			skill_group;
	BYTE			horse_inventory=0;
} CHARACTER_POINT;

typedef struct character_point_instant
{
	long			points[POINT_MAX_NUM];

	float			fRot;

	int				iMaxHP;
	int				iMaxSP;

	long			position;

	long			instant_flag;
	DWORD			dwAIFlag;
	DWORD			dwImmuneFlag;
	DWORD			dwLastShoutPulse;
	DWORD			dwLastTradeShoutPulse;

	DWORD			parts[PART_MAX_NUM]; // @fixme502

	LPCHARACTER		pCubeNpc;
	LPCHARACTER		battle_victim;

	BYTE			gm_level;

	BYTE			bBasePart;

	int				iMaxStamina;

	BYTE			bBlockMode;

	int				iDragonSoulActiveDeck;
	LPENTITY		m_pDragonSoulRefineWindowOpener;
} CHARACTER_POINT_INSTANT;

// @fixme199 BEGIN
struct PlayerSlotT {
	std::array<LPITEM,INVENTORY_AND_EQUIP_SLOT_MAX> pItems;
	std::array<BYTE,INVENTORY_AND_EQUIP_SLOT_MAX> bItemGrid;
	std::array<LPITEM,DRAGON_SOUL_INVENTORY_MAX_NUM> pDSItems;
	std::array<WORD,DRAGON_SOUL_INVENTORY_MAX_NUM> wDSItemGrid;
	std::array<LPITEM,CUBE_MAX_NUM> pCubeItems;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	std::array<TItemPosEx,ACCE_WINDOW_MAX_MATERIALS> pAcceMaterials;
#endif
	std::array<TQuickslot,QUICKSLOT_MAX_NUM> pQuickslot;

#ifdef ENABLE_ITEM_SAFE_FLUSH
	std::unordered_set<DWORD> setTouchedItems;
#endif
};
// @fixme199 END

#define TRIGGERPARAM		LPCHARACTER ch, LPCHARACTER causer

typedef struct trigger
{
	BYTE	type;
	int		(*func) (TRIGGERPARAM);
	long	value;
} TRIGGER;

class CTrigger
{
	public:
		CTrigger() : bType(0), pFunc(NULL)
		{
		}

		BYTE	bType;
		int	(*pFunc) (TRIGGERPARAM);
};

EVENTINFO(char_event_info)
{
	DynamicCharacterPtr ch;
};

EVENTINFO(area_of_effect_event_info)
{
	DynamicCharacterPtr ch;
	int radius;
	int cycle_delay;
	int cycle_count;
	int value1;
	int value2;
	DWORD	effectType;
};

EVENTINFO(request_info_event_info)
{
	DynamicCharacterPtr ch;
	BYTE requestType;
	bool isSend;
};

EVENTINFO(captcha_event_info)
{
	DynamicCharacterPtr ch;
	DynamicCharacterPtr invokerCh;
};

typedef std::map<VID, size_t> target_map;

struct TSkillUseInfo
{
	int	    iHitCount;
	int	    iMaxHitCount;
	int	    iSplashCount;
	DWORD   dwNextSkillUsableTime;
	int	    iRange;
	bool    bUsed;
	DWORD   dwVID;
	bool    isGrandMaster;
	#ifdef ENABLE_SKILL_COOLDOWN_CHECK
	bool	cooldown{false};
	int		skillCount{0};
	#endif

	target_map TargetVIDMap;

	TSkillUseInfo()
		: iHitCount(0), iMaxHitCount(0), iSplashCount(0), dwNextSkillUsableTime(0), iRange(0), bUsed(false),
		dwVID(0), isGrandMaster(false)
   	{}

	bool    HitOnce(DWORD dwVnum = 0);

	bool    UseSkill(bool isGrandMaster, DWORD vid, DWORD dwCooltime, int splashcount = 1, int hitcount = -1, int range = -1);
	DWORD   GetMainTargetVID() const	{ return dwVID; }
	void    SetMainTargetVID(DWORD vid) { dwVID=vid; }
	void    ResetHitCount() { if (iSplashCount) { iHitCount = iMaxHitCount; iSplashCount--; } }
	#ifdef ENABLE_SKILL_COOLDOWN_CHECK
	bool	IsOnCooldown(int vnum);
	#endif
};

typedef struct packet_party_update TPacketGCPartyUpdate;
class CExchange;
class CSkillProto;
class CParty;
class CDungeon;
class CWarMap;
class CAffect;
class CGuild;
class CSafebox;
class CArena;

class CShop;
typedef class CShop * LPSHOP;

class CMob;
class CMobInstance;
typedef struct SMobSkillInfo TMobSkillInfo;

//SKILL_POWER_BY_LEVEL
extern int GetSkillPowerByLevelFromType(int job, int skillgroup, int skilllevel);
//END_SKILL_POWER_BY_LEVEL
extern int GetMaxAttackCountForSkill(DWORD dwSkillVnum, int skillPower = 0);

namespace marriage
{
	class WeddingMap;
}
enum e_overtime
{
	OT_NONE,
	OT_3HOUR,
	OT_5HOUR,
};

typedef std::map<const std::string, long long> SPECIAL_FLAG_MAP;

#define NEW_ICEDAMAGE_SYSTEM
class CHARACTER : public CEntity, public CFSM, public CHorseRider
{
	protected:
		//////////////////////////////////////////////////////////////////////////////////
		virtual void	EncodeInsertPacket(LPENTITY entity);
		virtual void	EncodeRemovePacket(LPENTITY entity);
		//////////////////////////////////////////////////////////////////////////////////

	public:
		LPCHARACTER			FindCharacterInView(const char * name, bool bFindPCOnly);
		void				UpdatePacket();

		//////////////////////////////////////////////////////////////////////////////////
	protected:
		CStateTemplate<CHARACTER>	m_stateMove;
		CStateTemplate<CHARACTER>	m_stateBattle;
		CStateTemplate<CHARACTER>	m_stateIdle;

	public:
		virtual void		StateMove();
		virtual void		StateBattle();
		virtual void		StateIdle();
		virtual void		StateFlag();
		virtual void		StateFlagBase();
		void				StateHorse();

	protected:
		void				__StoneSpawnGroup(DWORD groupVnum, DWORD count);

		// STATE_IDLE_REFACTORING
		void				__StateIdle_Monster();
		void				__StateIdle_Stone();
		void				__StateIdle_NPC();
		// END_OF_STATE_IDLE_REFACTORING

	public:
		DWORD GetAIFlag() const	{ return m_pointsInstant.dwAIFlag; }

		void				SetLooseAggroOnDistance();
		void				SetIgnoreLastAttack();

		void				SetAggressive();
		bool				IsAggressive() const;

		void				SetCoward();
		bool				IsCoward() const;
		void				CowardEscape();

		void				SetNoAttackShinsu();
		bool				IsNoAttackShinsu() const;

		void				SetNoAttackChunjo();
		bool				IsNoAttackChunjo() const;

		void				SetNoAttackJinno();
		bool				IsNoAttackJinno() const;

		void				SetAttackMob();
		bool				IsAttackMob() const;

		virtual void			BeginStateEmpty();
		virtual void			EndStateEmpty() {}

		void				RestartAtSamePos();

	protected:
		DWORD				m_dwStateDuration;
		//////////////////////////////////////////////////////////////////////////////////

	public:
		CHARACTER(bool isPlayer = false);
		virtual ~CHARACTER();

		void			Create(const char * c_pszName, DWORD vid, bool isPC);
		void			Destroy();

		void			Disconnect(const char * c_pszReason);

	protected:
		void			Initialize(bool isPlayer = false);

		//////////////////////////////////////////////////////////////////////////////////
		// Basic Points
	public:
		DWORD			GetPlayerID() const	{ return m_dwPlayerID; }

		void			SetPlayerProto(const TPlayerTable * table);
		void			CreatePlayerProto(TPlayerTable & tab);

		void			SetProto(const CMob * c_pkMob);
		DWORD			GetRaceNum() const; // @fixme501

		void			Save();		// DelayedSave
		void			SaveReal();
		void			FlushDelayedSaveItem();

		const char *	GetName() const;
		const VID &		GetVID() const		{ return m_vid;		}

		void			SetName(const std::string& name) { m_stName = name; }

		void			SetRace(BYTE race);
		bool			ChangeSex();

		DWORD			GetAID() const;
		int				GetChangeEmpireCount() const;
		void			SetChangeEmpireCount();
		int				ChangeEmpire(BYTE empire);

		BYTE			GetJob() const;
		BYTE			GetCharType() const;

		bool			IsPC() const		{ return GetDesc() ? true : false; }
		bool			IsNPC()	const		{ return m_bCharType != CHAR_TYPE_PC; }
		bool			IsMonster()	const	{ return m_bCharType == CHAR_TYPE_MONSTER; }
		bool			IsStone() const		{ return m_bCharType == CHAR_TYPE_STONE; }
		bool			IsDoor() const		{ return m_bCharType == CHAR_TYPE_DOOR; }
		bool			IsBuilding() const	{ return m_bCharType == CHAR_TYPE_BUILDING;  }
		bool			IsWarp() const		{ return m_bCharType == CHAR_TYPE_WARP; }
		bool			IsGoto() const		{ return m_bCharType == CHAR_TYPE_GOTO; }
#ifdef __PET_SYSTEM__
		bool			IsPet() const		{ return m_bCharType == CHAR_TYPE_PET; }
#else
		bool			IsPet() const { return false; }
#endif
		bool			IsAOE() const { return m_bCharType == CHAR_TYPE_AOE; }

		DWORD			GetLastShoutPulse() const	{ return m_pointsInstant.dwLastShoutPulse; }
		void			SetLastShoutPulse(DWORD pulse) { m_pointsInstant.dwLastShoutPulse = pulse; }

		DWORD			GetLastTradeShoutPulse() const { return m_pointsInstant.dwLastTradeShoutPulse; }
		void			SetLastTradeShoutPulse(DWORD pulse) { m_pointsInstant.dwLastTradeShoutPulse = pulse; }


		bool			IsLevelViewable() const;
		int				GetViewedLevel() const;

		int				GetLevel() const		{ return m_points.level;	}
		void			SetLevel(BYTE level);

		BYTE			GetGMLevel() const;
		BOOL 			IsGM() const;
		BOOL 			IsTutor() const;
		void			SetGMLevel();

		DWORD			GetExp() const		{ return m_points.exp;	}
		void			SetExp(DWORD exp)	{ m_points.exp = exp;	}
		DWORD			GetNextExp() const;
		LPCHARACTER		DistributeExp();
		void			DistributeHP(LPCHARACTER pkKiller);
		void			DistributeSP(LPCHARACTER pkKiller, int iMethod=0);

#ifdef ENABLE_KILL_EVENT_FIX
		LPCHARACTER		GetMostAttacked();
#endif

		void			SetPosition(int pos);
		bool			IsPosition(int pos) const	{ return m_pointsInstant.position == pos ? true : false; }
		int				GetPosition() const		{ return m_pointsInstant.position; }

		void			SetPart(BYTE bPartPos, DWORD wVal); // @fixme502
		DWORD			GetPart(BYTE bPartPos) const; // @fixme502
		DWORD			GetOriginalPart(BYTE bPartPos) const; // @fixme502

		void			SetHP(int hp)		{ m_points.hp = hp; }
		int				GetHP() const		{ return m_points.hp; }

		void			SetSP(int sp)		{ m_points.sp = sp; }
		int				GetSP() const		{ return m_points.sp; }

		void			SetStamina(int stamina)	{ m_points.stamina = stamina; }
		int				GetStamina() const		{ return m_points.stamina; }

		void			SetMaxHP(int iVal)	{ m_pointsInstant.iMaxHP = iVal; }
		int				GetMaxHP() const	{ return m_pointsInstant.iMaxHP; }

		void			SetMaxSP(int iVal)	{ m_pointsInstant.iMaxSP = iVal; }
		int				GetMaxSP() const	{ return m_pointsInstant.iMaxSP; }

		void			SetMaxStamina(int iVal)	{ m_pointsInstant.iMaxStamina = iVal; }
		int				GetMaxStamina() const	{ return m_pointsInstant.iMaxStamina; }

		void			SetRandomHP(int v)	{ m_points.iRandomHP = v; }
		void			SetRandomSP(int v)	{ m_points.iRandomSP = v; }

		int				GetRandomHP() const	{ return m_points.iRandomHP; }
		int				GetRandomSP() const	{ return m_points.iRandomSP; }

		int				GetHPPct() const;
		int				GetDefence() const;
		int				GetMagicDefenceRating() const;

		void			SetRealPoint(BYTE idx, int val);
		int				GetRealPoint(BYTE idx) const;

		void			SetPoint(BYTE idx, int val);
		int				GetPoint(BYTE idx) const;
		int				GetLimitPoint(BYTE idx) const;
		int				GetPolymorphPoint(BYTE idx) const;
		int				GetResistPoint(BYTE idx, LPCHARACTER pkAttacker = NULL) const;

		const TMobTable &	GetMobTable() const;
		BYTE				GetMobRank() const;
		BYTE				GetMobBattleType() const;
		BYTE				GetMobSize() const;
		DWORD				GetMobDamageMin() const;
		DWORD				GetMobDamageMax() const;
		WORD				GetMobAttackRange() const;
		DWORD				GetMobDropItemVnum() const;
		float				GetMobDamageMultiply() const;

		BYTE				GetFraction() const;
		bool				IsFraction() const;

		void				SendFractionInfoBoard(LPDESC d);

		// NEWAI
		bool			IsBerserker() const;
		bool			IsBerserk() const;
		void			SetBerserk(bool mode);

		bool			IsStoneSkinner() const;

		bool			IsGodSpeeder() const;
		bool			IsGodSpeed() const;
		void			SetGodSpeed(bool mode);

		bool			IsDeathBlower() const;
		bool			IsDeathBlow() const;

		bool			IsReviver() const;
		bool			HasReviverInParty() const;
		bool			IsRevive() const;
		void			SetRevive(bool mode);

		bool			IsIgnoreLastAttack() const;
		// NEWAI END

		bool			IsRaceFlag(DWORD dwBit) const;
		bool			IsSummonMonster() const;
		DWORD			GetSummonVnum() const;

		DWORD			GetPolymorphItemVnum() const;
		DWORD			GetMonsterDrainSPPoint() const;

		void			MainCharacterPacket();

		void			ComputePoints();
		void			ComputeBattlePoints();
		void			PointChange(BYTE type, int amount, bool bAmount = false, bool bBroadcast = false);
		void			PointsPacket();
		void			CheckMaximumPoints();
		bool			IsUtilityPoint(BYTE point);

		bool			Show(long lMapIndex, long x, long y, long z = LONG_MAX, bool bShowSpawnMotion = false);

		void			Sitdown(int is_ground);
		void			Standup();

		VECTOR			GetForwardDirection() const;

		void			SetRotation(float fRot);
		void			SetRotationToXY(long x, long y);
		float			GetRotation() const	{ return m_pointsInstant.fRot; }

		void			MotionPacketEncode(BYTE motion, LPCHARACTER victim, struct packet_motion * packet);
		void			Motion(BYTE motion, LPCHARACTER victim = NULL);

		void			ChatDebug(const char *format, ...);
		void			ChatPacket(BYTE type, const char *format, ...);
		void			MonsterChat(BYTE bMonsterChatType);
		void			MonsterChat(const char* format, ...);
		void			MonsterChatPlayer(LPCHARACTER player, const char* format, ...);

		void			ResetPoint(int iLv);

		void			SetBlockMode(BYTE bFlag);
		void			SetBlockModeForce(BYTE bFlag);
		bool			IsBlockMode(BYTE bFlag) const	{ return (m_pointsInstant.bBlockMode & bFlag)?true:false; }

		bool			IsPolymorphed() const		{ return m_dwPolymorphRace>0; }
		bool			IsPolyMaintainStat() const	{ return m_bPolyMaintainStat; }
		void			SetPolymorph(DWORD dwRaceNum, bool bMaintainStat = false);
		DWORD			GetPolymorphVnum() const	{ return m_dwPolymorphRace; }
		int				GetPolymorphPower() const;

		// FISING
		void			fishing();
		void			fishing_take();
		// END_OF_FISHING

		// MINING
		void			mining(LPCHARACTER chLoad);
		void			mining_cancel();
		void			mining_take();
		// END_OF_MINING

		void			ResetPlayTime(DWORD dwTimeRemain = 0);

		void			CreateFly(BYTE bType, LPCHARACTER pkVictim);

		void			ResetChatCounter();
		BYTE			IncreaseChatCounter();
		BYTE			GetChatCounter() const;

	protected:
		DWORD			m_dwPolymorphRace;
		bool			m_bPolyMaintainStat;
		DWORD			m_dwLoginPlayTime;
		DWORD			m_dwPlayerID;
		VID				m_vid;
		std::string		m_stName;
		BYTE			m_bCharType;

		CHARACTER_POINT		m_points;
		CHARACTER_POINT_INSTANT	m_pointsInstant;
		std::unique_ptr<PlayerSlotT> m_PlayerSlots; // @fixme199

		int				m_iMoveCount;
		DWORD			m_dwPlayStartTime;
		BYTE			m_bAddChrState;
		bool			m_bSkipSave;
		BYTE			m_bChatCounter;
		BOOL			m_bIsSprint;
		bool m_isAdditionalShootingTarget;

		// End of Basic Points

		//////////////////////////////////////////////////////////////////////////////////
		// Move & Synchronize Positions
		//////////////////////////////////////////////////////////////////////////////////
	public:
		bool			IsStateMove() const			{ return IsState((CState&)m_stateMove); }
		bool			IsStateIdle() const			{ return IsState((CState&)m_stateIdle); }
		bool			IsStateBattle() const			{ return IsState((CState&)m_stateBattle); }
		bool			IsWalking() const			{ return m_bNowWalking; }
		void			SetWalking(bool bWalkFlag)	{ m_bWalking=bWalkFlag; }
		void			SetNowWalking(bool bWalkFlag);
		void			ResetWalking()			{ SetNowWalking(m_bWalking); }
		void			SetSprint(bool state);

		bool			Goto(long x, long y);
		void			Stop();

		bool			CanMove() const;

		void			SyncPacket(LPCHARACTER who);
		void			SyncPacket();
		bool			Sync(long x, long y);
		bool			Move(long x, long y);
		void			OnMove(bool bIsAttack = false);
		DWORD			GetMotionMode() const;
		float			GetMoveMotionSpeed() const;
		float			GetMoveSpeed() const;
		void			CalculateMoveDuration();
		void			SendMovePacket(BYTE bFunc, BYTE bArg, DWORD x, DWORD y, DWORD dwDuration, DWORD dwTime=0, int iRot=-1);
		DWORD			GetCurrentMoveDuration() const	{ return m_dwMoveDuration; }
		DWORD			GetWalkStartTime() const	{ return m_dwWalkStartTime; }
		DWORD			GetLastMoveTime() const		{ return m_dwLastMoveTime; }
		DWORD			GetLastAttackTime() const	{ return m_dwLastAttackTime; }

		void			SetLastAttacked(DWORD time);

		bool			SetSyncOwner(LPCHARACTER ch, bool bRemoveFromList = true);
		bool			IsSyncOwner(LPCHARACTER ch) const;

		bool			WarpSet(long x, long y, long lRealMapIndex = 0);
		void			SetWarpLocation(long lMapIndex, long x, long y);
		void			WarpEnd();
		const PIXEL_POSITION & GetWarpPosition() const { return m_posWarp; }
		bool			WarpToPID(DWORD dwPID);

		void			SetExitLocation(long x, long y, long mapIndex);
		void			SaveExitLocation();
		void			ExitToSavedLocation();

		void			StartStaminaConsume();
		void			StopStaminaConsume();
		bool			IsStaminaConsume() const;
		bool			IsStaminaHalfConsume() const;

		void			ResetStopTime();
		DWORD			GetStopTime() const;

	protected:
		void			ClearSync();

		float			m_fSyncTime;
		LPCHARACTER		m_pkChrSyncOwner;
		CHARACTER_LIST	m_kLst_pkChrSyncOwned;

		PIXEL_POSITION	m_posDest;
		PIXEL_POSITION	m_posStart;
		PIXEL_POSITION	m_posWarp;
		long			m_lWarpMapIndex;

		PIXEL_POSITION	m_posExit;
		long			m_lExitMapIndex;

		DWORD			m_dwMoveStartTime;
		DWORD			m_dwMoveDuration;

		DWORD			m_dwLastStartRiding;

		DWORD			m_dwLastMoveTime;
		DWORD			m_dwLastAttackTime;
		DWORD			m_dwWalkStartTime;
		DWORD			m_dwStopTime;

		bool			m_bWalking;
		bool			m_bNowWalking;
		bool			m_bStaminaConsume;
		// End

	public:
		void			SyncQuickslot(BYTE bType, BYTE bOldPos, BYTE bNewPos);
		bool			GetQuickslot(BYTE pos, TQuickslot ** ppSlot);
		bool			SetQuickslot(BYTE pos, TQuickslot & rSlot);
		bool			DelQuickslot(BYTE pos);
		bool			SwapQuickslot(BYTE a, BYTE b);
		void			ChainQuickslotItem(LPITEM pItem, BYTE bType, BYTE bOldPos);
#ifdef ENABLE_IKASHOP_RENEWAL
	private:
		// watcher
		using ShopActionWeight = uint16_t;
		using ShopActionTimestamp = double;
		using ShopActionData = std::pair<ShopActionWeight, ShopActionTimestamp>;
		using ShopActionRegister = std::deque<ShopActionData>;


	public:
		using SHOP_HANDLE = std::shared_ptr<ikashop::CShop>;
		using SAFEBOX_HANDLE = std::shared_ptr<ikashop::CShopSafebox>;
		using AUCTION_HANDLE = std::shared_ptr<ikashop::CAuction>;

		SHOP_HANDLE GetIkarusShop() {return m_pkIkarusShop;}
		void SetIkarusShop(SHOP_HANDLE pkShop) {m_pkIkarusShop = pkShop;}
		
		SHOP_HANDLE	GetIkarusShopGuest() const {return m_pkIkarusShopGuest;}
		void SetIkarusShopGuest(SHOP_HANDLE pkShop);
		
		SAFEBOX_HANDLE GetIkarusShopSafebox() {return m_pkIkarusShopSafebox;}
		void SetIkarusShopSafebox(SAFEBOX_HANDLE pk);

		void SetIkarusAuction(AUCTION_HANDLE pk) {m_pkIkarusAuction = pk;}
		void SetIkarusAuctionGuest(AUCTION_HANDLE pk);

		AUCTION_HANDLE GetIkarusAuction() {return m_pkIkarusAuction;}
		AUCTION_HANDLE GetIkarusAuctionGuest() const {return m_pkIkarusAuctionGuest;}

		// can warp check
		int GetIkarusShopUseTime() const { return m_iIkarusShopUseTime; }
		void SetIkarusShopUseTime() { m_iIkarusShopUseTime = thecore_pulse(); }

		int GetIkarusShopOpenClosedTime() const { return m_iIkarusShopOpenClosedTime; }
		void SetIkarusShopOpenClosedTime() { m_iIkarusShopOpenClosedTime = thecore_pulse(); }

		// looking offerlist
		void SetLookingIkarusShopOfferList(bool state) { m_bIsLookingIkarusShopOfferList = state; }
		bool IsLookingIkarusShopOfferList() const { return m_bIsLookingIkarusShopOfferList; }
		
		// looking shop
		void SetLookingShopOwner(bool state) { m_bIsLookingShopOwner  = state;}
		bool IsLookingShopOwner() const { return m_bIsLookingShopOwner; }
		
		// looking auction
		void SetLookingAuctionOwner(bool state) { m_bIsLookingAuctionOwner = state;}
		bool IsLookingAuctionOwner() const { return m_bIsLookingAuctionOwner; }

#ifdef EXTEND_IKASHOP_PRO
		void SetLookingNotificationList(bool state) { m_bIsLookingNotificationList = state; }
		bool IsLookingNotificationList() const { return m_bIsLookingNotificationList; }
#endif

		bool CanTakeInventoryItem(LPITEM item, TItemPos* pos);

		//watcher
		bool IkarusShopFloodCheck(ShopActionWeight weight);

	private:
		//watcher
		ShopActionRegister m_IkarusShopActionRegister;

		// handles
		SHOP_HANDLE	m_pkIkarusShop;
		SHOP_HANDLE	m_pkIkarusShopGuest;
		SAFEBOX_HANDLE m_pkIkarusShopSafebox;
		AUCTION_HANDLE m_pkIkarusAuction;
		AUCTION_HANDLE m_pkIkarusAuctionGuest;

		int m_iIkarusShopUseTime = 0;
		int m_iIkarusShopOpenClosedTime = 0;
		bool m_bIsLookingIkarusShopOfferList = false;
		bool m_bIsLookingShopOwner = false;
		bool m_bIsLookingAuctionOwner = false;
#ifdef EXTEND_IKASHOP_PRO
		bool m_bIsLookingNotificationList = false;
#endif
#endif

		////////////////////////////////////////////////////////////////////////////////////////
		// Affect
	public:
		void			StartAffectEvent();
		void			ClearAffect(bool bSave=false);
		void			ComputeAffect(CAffect * pkAff, bool bAdd, bool bOverride = false);
		bool			CanAddAffect(DWORD dwType, BYTE bApplyOn, long lApplyValue, DWORD dwFlag, long lDuration);
		void			OnAddAffect(DWORD dwType, BYTE& bApplyOn, long& lApplyValue, DWORD dwFlag, long lDuration);
		bool			AddAffect(DWORD dwType, BYTE bApplyOn, long lApplyValue, DWORD dwFlag, long lDuration, long lSPCost, bool bOverride, bool IsCube = false, DWORD itemVnum = 0);
		void			RefreshAffect();
		bool			RemoveAffect(DWORD dwType);
		bool			IsAffectFlag(DWORD dwAff) const;

		bool			UpdateAffect();	// called from EVENT
		int				ProcessAffect();

		void			LoadAffect(DWORD dwCount, TPacketAffectElement * pElements);
		void			SaveAffect();

		bool			IsLoadedAffect() const	{ return m_bIsLoadedAffect; }

		bool			IsGoodAffect(BYTE bAffectType) const;

		void			RemoveSkillAffect();
		void			RemoveGoodAffect();
		void			RemoveBadAffect();
		void			RemoveGuildWarAffect();

		int				CountAffect(DWORD dwType) const;
		CAffect *		FindAffect(DWORD dwType, BYTE bApply=POINT_NONE) const;
		const std::list<CAffect *> & GetAffectContainer() const	{ return m_list_pkAffect; }
		bool			RemoveAffect(CAffect * pkAff);

	protected:
		bool			m_bIsLoadedAffect;
		TAffectFlag		m_afAffectFlag;
		std::list<CAffect *>	m_list_pkAffect;

	public:
		// PARTY_JOIN_BUG_FIX
		void			SetParty(LPPARTY pkParty);
		LPPARTY			GetParty() const	{ return m_pkParty; }

		bool			RequestToParty(LPCHARACTER leader);
		void			DenyToParty(LPCHARACTER member);
		void			AcceptToParty(LPCHARACTER member);

		void			PartyInvite(LPCHARACTER pchInvitee);

		void			PartyInviteAccept(LPCHARACTER pchInvitee);

		void			PartyInviteDeny(DWORD dwPID);

		bool			BuildUpdatePartyPacket(TPacketGCPartyUpdate & out);
		int				GetLeadershipSkillLevel() const;

		bool			CanSummon(int iLeaderShip);

		void			SetPartyRequestEvent(LPEVENT pkEvent) { m_pkPartyRequestEvent = pkEvent; }

	protected:

		void			PartyJoin(LPCHARACTER pkLeader);

		enum PartyJoinErrCode {
			PERR_NONE		= 0,
			PERR_SERVER,
			PERR_DUNGEON,
			PERR_OBSERVER,
			PERR_LVBOUNDARY,
			PERR_LOWLEVEL,
			PERR_HILEVEL,
			PERR_ALREADYJOIN,
			PERR_PARTYISFULL,
			PERR_SEPARATOR,			///< Error type separator.
			PERR_DIFFEMPIRE,
			PERR_MAX
		};

		static PartyJoinErrCode	IsPartyJoinableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest);

		static PartyJoinErrCode	IsPartyJoinableMutableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest);

		LPPARTY			m_pkParty;
		DWORD			m_dwLastDeadTime;
		LPEVENT			m_pkPartyRequestEvent;

		typedef std::map< DWORD, LPEVENT >	EventMap;
		EventMap		m_PartyInviteEventMap;

		// END_OF_PARTY_JOIN_BUG_FIX

		////////////////////////////////////////////////////////////////////////////////////////
		// Dungeon
	public:
		void			SetDungeon(LPDUNGEON pkDungeon);
		LPDUNGEON		GetDungeon() const	{ return m_pkDungeon; }
		LPDUNGEON		GetDungeonForce() const;
	protected:
		LPDUNGEON	m_pkDungeon;
		int			m_iEventAttr;

		////////////////////////////////////////////////////////////////////////////////////////
		// Guild
	public:
		void			SetGuild(CGuild * pGuild);
		CGuild*			GetGuild() const	{ return m_pGuild; }

		void			SetWarMap(CWarMap* pWarMap);
		CWarMap*		GetWarMap() const	{ return m_pWarMap; }

	protected:
		CGuild *		m_pGuild;
		DWORD			m_dwUnderGuildWarInfoMessageTime;
		CWarMap *		m_pWarMap;

		////////////////////////////////////////////////////////////////////////////////////////
		// Item related
	public:
		bool			CanHandleItem(bool bSkipRefineCheck = false, bool bSkipObserver = false, DWORD busy_exclude=BUSY_CAN_HANDLE_ITEM_EXCLUDE);

		bool			IsItemLoaded() const	{ return m_bItemLoaded; }
		void			SetItemLoaded()	{ m_bItemLoaded = true; }

		void			ClearItem();
#ifdef ENABLE_HIGHLIGHT_NEW_ITEM
		void			SetItem(TItemPos Cell, LPITEM item, bool bWereMine = false);
#else
		void			SetItem(TItemPos Cell, LPITEM item);
#endif
		LPITEM			GetItem(TItemPos Cell) const;
		LPITEM			GetInventoryItem(WORD wCell) const;
		bool			IsEmptyItemGrid(TItemPos Cell, BYTE size, int iExceptionCell = -1) const;

		BYTE* GetInventoryItemGrid() { return (m_PlayerSlots) ? m_PlayerSlots->bItemGrid.data() : nullptr; }

		void			SetWear(BYTE bCell, LPITEM item);
		LPITEM			GetWear(BYTE bCell) const;

		// MYSHOP_PRICE_LIST
		void			UseSilkBotary(void);

		void			UseSilkBotaryReal(const TPacketMyshopPricelistHeader* p);
		// END_OF_MYSHOP_PRICE_LIST

		bool			UseItemEx(LPITEM item, TItemPos DestCell);
		bool			UseItem(TItemPos Cell, TItemPos DestCell = NPOS);

		// ADD_REFINE_BUILDING
		bool			IsRefineThroughGuild() const;
		CGuild *		GetRefineGuild() const;
		int				ComputeRefineFee(int iCost, int iMultiply = 5) const;
		void			PayRefineFee(YANG iTotalMoney);
		void			SetRefineNPC(LPCHARACTER ch);
		// END_OF_ADD_REFINE_BUILDING

		bool			RefineItem(LPITEM pkItem, LPITEM pkTarget);
		bool			DropItem(TItemPos Cell, ITEM_COUNT bCount=0);
		bool			GiveRecallItem(LPITEM item);
		void			ProcessRecallItem(LPITEM item);

		//	void			PotionPacket(int iPotionType);
		void			EffectPacket(int enumEffectType);
		void			SpecificEffectPacket(const char filename[128]);

		// ADD_MONSTER_REFINE
		bool			IsRefineBonus(LPITEM scrollItem, DWORD bonusFlag);
		bool			CheckRefineBonus(LPITEM item, LPITEM scrollItem);
		bool			DoRefine(LPITEM item, bool bMoneyOnly = false, BYTE refineType = REFINE_TYPE_NORMAL);
		// END_OF_ADD_MONSTER_REFINE

		bool			DoRefineWithScroll(LPITEM item);
		bool			RefineInformation(BYTE bCell, BYTE bType, int iAdditionalCell = -1);

		void			SetRefineMode(int iAdditionalCell = -1);
		void			ClearRefineMode();

		bool			GiveItem(LPCHARACTER victim, TItemPos Cell);
		bool			CanReceiveItem(LPCHARACTER from, LPITEM item) const;
		void			ReceiveItem(LPCHARACTER from, LPITEM item);
		LPITEM			GiveItemToPlayerSpecialGroup(DWORD dwVnum, DWORD dwCount, int iRarePct, bool& bSuccess, bool bCreateItemOnly = false, bool bNotify = true);
		LPITEM			DropSpecialItemGroup(DWORD dwGroupNum, bool& bSuccess, bool bCreateItemOnly = false, bool bNotify = true, int iRarePct = -1);

		bool			MoveItem(TItemPos pos, TItemPos change_pos, ITEM_COUNT num);
		bool			PickupItem(DWORD vid);
		bool			EquipItem(LPITEM item, int iCandidateCell = -1);
		bool			UnequipItem(LPITEM item, LPITEM keyItem=NULL);

		bool			CanEquipNow(const LPITEM item, const TItemPos& srcCell = NPOS, const TItemPos& destCell = NPOS);
		bool			CanUnequipNow(const LPITEM item, const TItemPos& srcCell = NPOS, const TItemPos& destCell = NPOS, const LPITEM keyItem=nullptr);

		void			OnWeaponChange(const LPITEM newItem, const LPITEM oldItem);

		bool			SwapItem(BYTE bCell, BYTE bDestCell);
		LPITEM			AutoGiveItem(DWORD dwItemVnum, ITEM_COUNT bCount=1, int iRarePct = -1, bool bMsg = true, bool alwaysNewItem = false);
		void			AutoGiveItem(LPITEM item, bool longOwnerShip = false);

		std::pair<bool, LPITEM>	AutoStackItemProto(DWORD dwItemVnum, ITEM_COUNT & bCount);
		LPITEM			AutoStackItem(LPITEM item);

		bool			HasSlotForItem(DWORD dwItemVnum) const;
		int				GetEmptyInventoryEx(LPITEM item);

		int				GetEmptyInventory(BYTE size, BYTE startSlot=0) const;
		int				GetEmptyDragonSoulInventory(LPITEM pItem) const;
		void			CopyDragonSoulItemGrid(std::vector<WORD>& vDragonSoulItemGrid) const;

		int				CountEmptyInventory() const;

		ITEM_COUNT		CountSpecifyItem(DWORD vnum) const;
		void			RemoveSpecifyItem(DWORD vnum, ITEM_COUNT count = 1);
		LPITEM			FindSpecifyItem(DWORD vnum) const;
		LPITEM			FindItemByID(DWORD id) const;

		ITEM_COUNT		CountSpecifyTypeItem(BYTE type) const;
		void			RemoveSpecifyTypeItem(BYTE type, ITEM_COUNT count = 1);

		LPITEM			GetEquipedUniqueItem(DWORD dwItemVnum) const;
		bool			IsEquipUniqueItem(DWORD dwItemVnum) const;

		// CHECK_UNIQUE_GROUP
		bool			IsEquipUniqueGroup(DWORD dwGroupVnum) const;
		// END_OF_CHECK_UNIQUE_GROUP

		void			SendEquipment(LPCHARACTER ch);
		// End of Item

		DWORD			GetRefineNPCVID() { return m_dwRefineNPCVID; }
		int				GetRefineAdditionalCell() const { return m_iRefineAdditionalCell; }

	protected:

		void			SendMyShopPriceListCmd(DWORD dwItemVnum, YANG dwItemPrice);

		bool			m_bNoOpenedShop;

		bool			m_bItemLoaded;
		int				m_iRefineAdditionalCell;
		bool			m_bUnderRefine;
		DWORD			m_dwRefineNPCVID;

	public:
		////////////////////////////////////////////////////////////////////////////////////////
		// Money related
		YANG			GetGold() const		{ return m_points.gold;	}
		void			SetGold(YANG gold)	{ m_points.gold = gold;	}
		bool			DropGold(YANG gold);
		YANG			GetAllowedGold() const;
		void			GiveGold(YANG iAmount);
		void			ChangeGold(YANG gold, bool isUpdate = true);
		// End of Money
#ifdef ENABLE_CHEQUE_SYSTEM
		INT				GetCheque() const { return m_points.cheque; }
		void			SetCheque(int cheque) { m_points.cheque = cheque; }
		void			GiveCheque(INT iAmount);
		bool			DropCheque(INT gold);
#endif

		////////////////////////////////////////////////////////////////////////////////////////
		// Shop related
	public:
		void			SetShop(LPSHOP pkShop);
		LPSHOP			GetShop() const { return m_pkShop; }
		void			ShopPacket(BYTE bSubHeader);

		void			SetShopOwner(LPCHARACTER ch) { m_pkChrShopOwner = ch; }
		LPCHARACTER		GetShopOwner() const { return m_pkChrShopOwner;}

		void			OpenMyShop(const char * c_pszSign, TShopItemTable * pTable, BYTE bItemCount, BYTE bTimeIndex);
		LPSHOP			GetMyShop() const { return m_pkMyShop; }
		void			CloseMyShop();

		bool			CanPlaceOnShopSlot(BYTE slot);
		bool			CanPlaceItemOnShopSlot(BYTE slot, BYTE itemHeight);
		bool			CanAddItemToShop(LPITEM item, BYTE slot);

	protected:

		LPSHOP			m_pkShop;
		LPSHOP			m_pkMyShop;
		std::string		m_stShopSign;
		LPCHARACTER		m_pkChrShopOwner;
		// End of shop

		////////////////////////////////////////////////////////////////////////////////////////
		// Exchange related
	public:
		bool			ExchangeStart(LPCHARACTER victim);
		void			SetExchange(CExchange * pkExchange);
		CExchange *		GetExchange() const	{ return m_pkExchange;	}

	protected:
		CExchange *		m_pkExchange;
		// End of Exchange

		////////////////////////////////////////////////////////////////////////////////////////
		// Battle
	public:
		struct TBattleInfo
		{
			int iTotalDamage;
			int iAggro;

			TBattleInfo(int iTot, int iAggr)
				: iTotalDamage(iTot), iAggro(iAggr)
				{}
		};
		typedef std::map<VID, TBattleInfo>	TDamageMap;

		typedef struct SAttackLog
		{
			DWORD	dwVID;
			DWORD	dwTime;
		} AttackLog;

		bool				Damage(LPCHARACTER pAttacker, int dam, EDamageType type = DAMAGE_TYPE_NORMAL, DWORD dwUsedSkill = 0, bool IsPenetrate = false);
		bool				__Profile__Damage(LPCHARACTER pAttacker, int dam, EDamageType type = DAMAGE_TYPE_NORMAL);
		void				DeathPenalty(BYTE bExpLossPercent);
		void				ReviveInvisible(int iDur);

		bool				Attack(LPCHARACTER pkVictim, BYTE bType = 0, BYTE skill_vnum=0);
		bool				IsResistPush();
		bool				IsResistFall();
		bool				CanBePushed();
		void				Knockback(LPCHARACTER pkAttacker, int push_force, VECTOR direction);
		bool				PenetrateHit(LPCHARACTER pkVictim, DWORD dwType = 0);
		bool				IsAlive() const		{ return m_pointsInstant.position == POS_DEAD ? false : true; }
		bool				CanFight() const;

		bool				CanBeginFight() const;
		void				BeginFight(LPCHARACTER pkVictim);

		bool				CounterAttack(LPCHARACTER pkChr);

		bool				IsBusyAction() const;

		bool				IsStun() const;
		void				Stun();
		bool				IsDead() const;
		void				Dead(LPCHARACTER pkKiller = NULL, bool bImmediateDead=false);

		void				Reward(bool bItemDrop);
		void				RewardGold(LPCHARACTER pkAttacker);

		std::vector<DWORD>	m_vec_dwFlyTargets;
		void				OnShootSkill(DWORD dwSkillVnum);
		bool				Shoot(BYTE bType);
		void				MainFlyTarget(DWORD dwTargetVID, long x, long y);
		void				AddFlyTarget(LPCHARACTER target, BYTE type);

		int					GetShootMaxTargetCount(DWORD dwType = 0);
		bool				IsAdditionalShootingTarget() { return m_isAdditionalShootingTarget; }
		void				SetAdditionalShootingTarget(bool value) { m_isAdditionalShootingTarget = value; }

		void				ForgetMyAttacker(bool reviveEffect = true);
		void				AggregateMonster();
		void				AttractRanger();
		void				PullMonster();

		int					GetArrowAndBow(LPITEM * ppkBow, LPITEM * ppkArrow, int iArrowCount = 1);
		void				UseArrow(LPITEM pkArrow, DWORD dwArrowCount);

		void				AttackedByPoison(LPCHARACTER pkAttacker);
		void				RemovePoison();
#ifdef ENABLE_WOLFMAN_CHARACTER
		void				AttackedByBleeding(LPCHARACTER pkAttacker);
		void				RemoveBleeding();
#endif
		void				AttackedByFire(LPCHARACTER pkAttacker, int amount, int count);
		void				RemoveFire();

		void				UpdateAlignment(int iAmount);
		int					GetAlignment() const;

		int					GetRealAlignment() const;
		void				ShowAlignment(bool bShow);

		void				SetKillerMode(bool bOn);
		bool				IsKillerMode() const;
		void				UpdateKillerMode();

		BYTE				GetPKMode() const;
		void				SetPKMode(BYTE bPKMode);

		void				ItemDropPenalty(LPCHARACTER pkKiller);

		void				UpdateAggrPoint(LPCHARACTER ch, EDamageType type, int dam);

		// HACK

	public:
		void SetComboSequence(BYTE seq);
		BYTE GetComboSequence() const;

		void SetLastComboTime(DWORD time);
		DWORD GetLastComboTime() const;

		int GetValidComboInterval() const;
		void SetValidComboInterval(int interval);

		BYTE GetComboIndex() const;

		void IncreaseComboHackCount(int k = 1);
		void ResetComboHackCount();
		void SkipComboAttackByTime(int interval);
		DWORD GetSkipComboAttackByTime() const;

		bool IsSprint() const { return m_bIsSprint; }

		void ResetFlyTarget();

	protected:
		BYTE m_bComboSequence;
		DWORD m_dwLastComboTime;
		int m_iValidComboInterval;
		BYTE m_bComboIndex;
		int m_iComboHackCount;
		DWORD m_dwSkipComboAttackByTime;

	protected:
		void				UpdateAggrPointEx(LPCHARACTER ch, EDamageType type, int dam, TBattleInfo & info);
		void				ChangeVictimByAggro(int iNewAggro, LPCHARACTER pNewVictim);

		DWORD				m_dwFlyTargetID;
		TDamageMap			m_map_kDamage;
		DWORD				m_dwKillerPID;

		int					m_iAlignment;		// Lawful/Chaotic value -200000 ~ 200000
		int					m_iRealAlignment;
		int					m_iKillerModePulse;
		BYTE				m_bPKMode;

		// Aggro
		DWORD				m_dwLastVictimSetTime;
		int					m_iMaxAggro;
		BYTE				m_bAggroChangeCount{};

		DWORD				dwCustomNpcFlag;
		// End of Battle

		// Stone
	public:
		bool				IsCustomNpcFlag(DWORD flag) { return IsNPC() && IS_SET(GetCustomNpcFlag(), flag); }
		DWORD				GetCustomNpcFlag() { return dwCustomNpcFlag; }
		void				SetCustomNpcFlag(DWORD flag, bool add = true);

		void				SetStone(LPCHARACTER pkChrStone);
		void				ClearStone();
		void				DetermineDropMetinStone();
		DWORD				GetDropMetinStoneVnum() const { return m_dwDropMetinStone; }
		BYTE				GetDropMetinStonePct() const { return m_bDropMetinStonePct; }

	protected:
		LPCHARACTER			m_pkChrStone;
		CHARACTER_SET		m_set_pkChrSpawnedBy;
		DWORD				m_dwDropMetinStone;
		BYTE				m_bDropMetinStonePct;
		// End of Stone

	public:
		enum
		{
			SKILL_UP_BY_POINT,
			SKILL_UP_BY_BOOK,
			SKILL_UP_BY_TRAIN,

			// ADD_GRANDMASTER_SKILL
			SKILL_UP_BY_QUEST,
			// END_OF_ADD_GRANDMASTER_SKILL
		};

		void				SkillLevelPacket();
		void				SkillLevelUp(DWORD dwVnum, BYTE bMethod = SKILL_UP_BY_POINT);
		bool				SkillLevelDown(DWORD dwVnum);
		// ADD_GRANDMASTER_SKILL
		bool				UseSkill(DWORD dwVnum, LPCHARACTER pkVictim, bool bUseGrandMaster = true);
		bool                IsSkillCooldown(DWORD dwVnum, float fSkillPower) { return m_SkillUseInfo[dwVnum].IsOnCooldown(dwVnum); }
		void				ResetSkill();
		void				SetSkillLevel(DWORD dwVnum, BYTE bLev);
		int					GetUsedSkillMasterType(DWORD dwVnum);

		bool				IsLearnableSkill(DWORD dwSkillVnum) const;
		// END_OF_ADD_GRANDMASTER_SKILL

		bool				CheckSkillHitCount(const BYTE SkillID, const VID dwTargetVID);
		bool				CanUseSkill(DWORD dwSkillVnum) const;
		bool				IsUsableSkillMotion(DWORD dwMotionIndex) const;
		int					GetSkillLevel(DWORD dwVnum) const;
		int					GetSkillMasterType(DWORD dwVnum) const;
		int					GetSkillPower(DWORD dwVnum, BYTE bLevel = 0) const;

		time_t				GetSkillNextReadTime(DWORD dwVnum) const;
		void				SetSkillNextReadTime(DWORD dwVnum, time_t time, bool bSuccess);
		void				SkillLearnWaitMoreTimeMessage(DWORD dwVnum);

		void				ComputePassiveSkill(DWORD dwVnum);
		int					ComputeSkill(DWORD dwVnum, LPCHARACTER pkVictim, BYTE bSkillLevel = 0);
#ifdef ENABLE_SKILL_FLAG_PARTY
		int					ComputeSkillParty(DWORD dwVnum, LPCHARACTER pkVictim, BYTE bSkillLevel = 0);
#endif
		int					ComputeSkillAtPosition(DWORD dwVnum, const PIXEL_POSITION& posTarget, BYTE bSkillLevel = 0);
		void				ComputeSkillPoints();

		void				SetSkillGroup(BYTE bSkillGroup);
		BYTE				GetSkillGroup() const		{ return m_points.skill_group; }

		int					ComputeCooltime(int time);

		void				GiveRandomSkillBook();

		void				DisableCooltime();
		bool				LearnSkillByBook(DWORD dwSkillVnum, DWORD dwProb = 0);
		bool				LearnGrandMasterSkill(DWORD dwSkillVnum);

		#ifdef ENABLE_SKILL_COOLDOWN_CHECK
		bool				SkillIsOnCooldown(int vnum) { return m_SkillUseInfo[vnum].IsOnCooldown(vnum); }
		#endif
	private:
		bool				m_bDisableCooltime;
		DWORD				m_dwLastSkillTime;
		// End of Skill

		// MOB_SKILL
	public:
		bool				HasMobSkill() const;
		size_t				CountMobSkill() const;
		const TMobSkillInfo * GetMobSkill(unsigned int idx) const;
		bool				CanUseMobSkill(unsigned int idx) const;
		bool				UseMobSkill(unsigned int idx);
		void				ResetMobSkillCooltime();
	protected:
		DWORD				m_adwMobSkillCooltime[MOB_SKILL_MAX_NUM];
		// END_OF_MOB_SKILL

		// for SKILL_FLAME_SPIRIT
	public:
		void				StartMuyeongEvent();
		void				StopMuyeongEvent();

	private:
		LPEVENT				m_pkMuyeongEvent;

		// for SKILL_LIGHTNING_CLAW lighting
	public:
		int					GetChainLightningIndex() const { return m_iChainLightingIndex; }
		void				IncChainLightningIndex() { ++m_iChainLightingIndex; }
		void				AddChainLightningExcept(LPCHARACTER ch) { m_setExceptChainLighting.emplace(ch); }
		void				ResetChainLightningIndex() { m_iChainLightingIndex = 0; m_setExceptChainLighting.clear(); }
		int					GetChainLightningMaxCount() const;
		const CHARACTER_SET& GetChainLightingExcept() const { return m_setExceptChainLighting; }

	private:
		int					m_iChainLightingIndex;
		CHARACTER_SET m_setExceptChainLighting;

		// for SKILL_STEALTH
	public:
		void				SetAffectedEunhyung();
		void				ClearAffectedEunhyung() { m_dwAffectedEunhyungLevel = 0; }
		DWORD				GetAffectedEunhyung() const { return m_dwAffectedEunhyungLevel; }

		bool				IsInvisible();

	private:
		DWORD				m_dwAffectedEunhyungLevel;

		// Skill levels

	protected:
		TPlayerSkill*					m_pSkillLevels;
		boost::unordered_map<BYTE, int>		m_SkillDamageBonus;
		std::map<int, TSkillUseInfo>	m_SkillUseInfo;

		////////////////////////////////////////////////////////////////////////////////////////
		// AI related
	public:
		void			AssignTriggers(const TMobTable * table);
		LPCHARACTER		GetVictim() const;
		void			SetVictim(LPCHARACTER pkVictim);
		LPCHARACTER		GetNearestVictim(LPCHARACTER pkChr);
		LPCHARACTER		GetProtege() const;
		bool			IsStoneSpawed() const { return m_pkChrStone != nullptr; }

		bool			Follow(LPCHARACTER pkChr, float fMinimumDistance = 150.0f);
		bool			Return();
		bool			IsGuardNPC() const;
		bool			IsChangeAttackPosition(LPCHARACTER target) const;
		void			ResetChangeAttackPositionTime() { m_dwLastChangeAttackPositionTime = get_dword_time() - AI_CHANGE_ATTACK_POISITION_TIME_NEAR;}
		void			SetChangeAttackPositionTime() { m_dwLastChangeAttackPositionTime = get_dword_time();}

		bool			OnIdle();

		void			OnAttack(LPCHARACTER pkChrAttacker);
		void			OnClick(LPCHARACTER pkChrCauser);

		VID				m_kVIDVictim;

	protected:
		DWORD			m_dwLastChangeAttackPositionTime;
		CTrigger		m_triggerOnClick;
		// End of AI

		////////////////////////////////////////////////////////////////////////////////////////
		// Target
	protected:
		LPCHARACTER				m_pkChrTarget;
		CHARACTER_SET	m_set_pkChrTargetedBy;

	public:
		void				SendTargetPacket(DWORD vid, BYTE hpPercent, BYTE subheader, LPCHARACTER target);
		void				SetTarget(LPCHARACTER pkChrTarget);
		void				BroadcastTargetPacket(bool refreshAffectList=false);
		void				ClearTarget();
		void				CheckTarget();
		LPCHARACTER			GetTarget() const { return m_pkChrTarget; }

		////////////////////////////////////////////////////////////////////////////////////////
		// Safebox
	public:
		int					GetSafeboxSize() const;
		void				QuerySafeboxSize();
		void				SetSafeboxSize(int size);

		CSafebox *			GetSafebox() const;
		void				LoadSafebox(int iSize, YANG dwGold, int iItemCount, TPlayerItem * pItems);
		void				ChangeSafeboxSize(int iSize);
		void				CloseSafebox();

		void				ReqSafeboxLoad(const char* pszPassword);

		void				CancelSafeboxLoad( void ) { m_bOpeningSafebox = false; }

		void				SetMallLoadTime(int t) { m_iMallLoadTime = t; }
		int					GetMallLoadTime() const { return m_iMallLoadTime; }

		CSafebox *			GetMall() const;
		void				LoadMall(int iItemCount, TPlayerItem * pItems);
		void				CloseMall();

		void				SetSafeboxOpenPosition();
		float				GetDistanceFromSafeboxOpen() const;

	protected:
		CSafebox *			m_pkSafebox;
		int					m_iSafeboxSize;
		int					m_iSafeboxLoadTime;
		bool				m_bOpeningSafebox;

		CSafebox *			m_pkMall;
		int					m_iMallLoadTime;

		PIXEL_POSITION		m_posSafeboxOpen;

		////////////////////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////////////////////
		// Mounting
	public:
		void				EnterMount();
		void				MountVnum(DWORD vnum);
		DWORD				GetMountVnum() const { return m_dwMountVnum; }
		DWORD				GetLastMountTime() const { return m_dwMountTime; }

		bool				CanUseHorseSkill();

		// Horse
		virtual	void		SetHorseLevel(int iLevel);

		virtual	bool		StartRiding();
		virtual	void		OnStopRiding();
		virtual	bool		StopRiding();

		virtual void		SetStopStaminaConsume(bool state);

		virtual	DWORD		GetMyHorseVnum() const;

		virtual void		SendHorseInfo();
		virtual	void		ClearHorseInfo();

		void				HorseSummon(bool bSummon, bool bFromFar = false, DWORD dwVnum = 0, const char* name = 0);

		LPCHARACTER			GetHorse() const			{ return m_chHorse; }
		LPCHARACTER			GetRider() const; // rider on horse
		void				SetRider(LPCHARACTER ch);

		bool				IsRiding() const;
		bool IsHorse() { return GetRaceNum() >= 20101 && GetRaceNum() <= 20109; }

#ifdef __PET_SYSTEM__
	public:
		CPetSystem*			GetPetSystem()				{ return m_petSystem; }

	protected:
		CPetSystem*			m_petSystem;
#endif
#ifdef USE_PET_SEAL_ON_LOGIN
	public:
		void				EnterPet();
		bool				SummonPetFromItem(LPITEM item);
		void				AddLoadedPetItems(DWORD itemID) { m_loadedPetItems.emplace(itemID); }
		std::unordered_set<DWORD>& GetLoadedPetItems() { return m_loadedPetItems; }
	protected:
		std::unordered_set<DWORD> m_loadedPetItems;
#endif

	protected:
		LPCHARACTER			m_chHorse;
		LPCHARACTER			m_chRider;

		DWORD				m_dwMountVnum;
		DWORD				m_dwMountTime;

		BYTE				m_bSendHorseLevel;
		BYTE				m_bSendHorseHealthGrade;
		BYTE				m_bSendHorseStaminaGrade;

		////////////////////////////////////////////////////////////////////////////////////////
		// Detailed Log
	public:
		void				DetailLog() { m_bDetailLog = !m_bDetailLog; }
		void				ToggleMonsterLog();
		void				MonsterLog(const char* format, ...);
	private:
		bool				m_bDetailLog;
		bool				m_bMonsterLog;

		////////////////////////////////////////////////////////////////////////////////////////
		// Empire

	public:
		void 				SetEmpire(BYTE bEmpire);
		BYTE				GetEmpire() const { return m_bEmpire; }

	protected:
		BYTE				m_bEmpire;

		////////////////////////////////////////////////////////////////////////////////////////
		// Regen
	public:
		void				SetRegen(LPREGEN pkRegen);

	protected:
		PIXEL_POSITION			m_posRegen;
		float				m_fRegenAngle;
		LPREGEN				m_pkRegen;
		size_t				regen_id_; // to help dungeon regen identification
		// End of Regen

	// SPECIAL_SPAWN
	public:
		LPSPECIAL_SPAWN m_pkSpecialSpawn;

		////////////////////////////////////////////////////////////////////////////////////////
		// Resists & Proofs
	public:
		bool				CannotMoveByAffect() const;
		bool				IsImmune(DWORD dwImmuneFlag, LPCHARACTER pkAttacker = NULL);
		void				SetImmuneFlag(DWORD dw) { m_pointsInstant.dwImmuneFlag = dw; }

	protected:
		void				ApplyMobAttribute(const TMobTable* table);
		// End of Resists & Proofs

		////////////////////////////////////////////////////////////////////////////////////////
		// QUEST

	public:
		void				SetQuestNPCID(DWORD vid);
		DWORD				GetQuestNPCID() const { return m_dwQuestNPCVID; }
		LPCHARACTER			GetQuestNPC() const;

		void				SetQuestItemPtr(LPITEM item);
		void				ClearQuestItemPtr();
		LPITEM				GetQuestItemPtr() const;

#ifdef ENABLE_QUEST_DND_EVENT
		void				SetQuestDNDItemPtr(LPITEM item);
		void				ClearQuestDNDItemPtr();
		LPITEM				GetQuestDNDItemPtr() const;
#endif

		void				SetQuestBy(DWORD dwQuestVnum)	{ m_dwQuestByVnum = dwQuestVnum; }
		DWORD				GetQuestBy() const			{ return m_dwQuestByVnum; }

		int					GetQuestFlag(const std::string& flag) const;
		void				SetQuestFlag(const std::string& flag, int value);

		void				ConfirmWithMsg(const char* szMsg, int iTimeout, DWORD dwRequestPID);

		bool				IsQuestRunning() const;

	private:
		DWORD				m_dwQuestNPCVID;
		DWORD				m_dwQuestByVnum;
		DWORD				m_dwQuestItemVID{}; // @fixme304 (LPITEM -> DWORD)
#ifdef ENABLE_QUEST_DND_EVENT
		DWORD				m_dwQuestDNDItemVID{}; // @fixme304 (LPITEM -> DWORD)
#endif

		// Events
	public:
		bool				StartStateMachine(int iPulse = 1);
		void				StopStateMachine();
		void				UpdateStateMachine(DWORD dwPulse);
		void				SetNextStatePulse(int iPulseNext);

		void				UpdateCharacter(DWORD dwPulse);

	protected:
		DWORD				m_dwNextStatePulse;

		// Marriage
	public:
		LPCHARACTER			GetMarryPartner() const;
		void				SetMarryPartner(LPCHARACTER ch);
		int					GetMarriageBonus(DWORD dwItemVnum, bool bSum = true);

		void				SetWeddingMap(marriage::WeddingMap* pMap);
		marriage::WeddingMap* GetWeddingMap() const { return m_pWeddingMap; }

	private:
		marriage::WeddingMap* m_pWeddingMap;
		LPCHARACTER			m_pkChrMarried;

		// Warp Character
	public:
		void				StartWarpNPCEvent();

	public:
		void				StartSaveEvent();
		void				StartRecoveryEvent();
		void				StartCheckSpeedHackEvent();
		void				StartDestroyWhenIdleEvent();

		LPEVENT				m_pkDeadEvent;
		LPEVENT				m_pkStunEvent;
		LPEVENT				m_pkSaveEvent;
		LPEVENT				m_pkRecoveryEvent;
		LPEVENT				m_pkTimedEvent;
		LPEVENT				m_pkFishingEvent;
		LPEVENT				m_pkPreFishingEvent;
		LPEVENT				m_pkAffectEvent;
		LPEVENT				m_pkPoisonEvent;
#ifdef ENABLE_WOLFMAN_CHARACTER
		LPEVENT				m_pkBleedingEvent;
#endif
		LPEVENT				m_pkFireEvent;
		LPEVENT				m_pkWarpNPCEvent;
		//DELAYED_WARP
		//END_DELAYED_WARP

		// MINING
		LPEVENT				m_pkMiningEvent;
		// END_OF_MINING
		LPEVENT				m_pkWarpEvent;
		LPEVENT				m_pkCheckSpeedHackEvent;
		LPEVENT				m_pkDestroyWhenIdleEvent;
		LPEVENT				m_pkPetSystemUpdateEvent;

		LPEVENT				m_pkResetAttackLogEvent;
		LPEVENT				m_pkAIEvent;

		LPEVENT				m_pkRequestDiscordInfoEvent;
		LPEVENT				m_pkRequestHackInfoEvent;

		LPEVENT				m_pkCaptchaEvent;

		bool IsWarping() const { return m_pkWarpEvent || m_pkTimedEvent;  }

		bool				m_bHasPoisoned;
#ifdef ENABLE_WOLFMAN_CHARACTER
		bool				m_bHasBled;
#endif

		const CMob *		m_pkMobData;
		CMobInstance *		m_pkMobInst;

		std::map<int, LPEVENT> m_mapMobSkillEvent;

		friend struct FuncSplashDamage;
		friend struct FuncSplashAffect;
		friend class CFuncShoot;

	public:
		int				GetPremiumRemainSeconds(BYTE bType) const;
		bool			IsActivePremiumSubscription() const;
		int				GetPremiumSubscriptionRemainSeconds() const;

		bool			CanEnterPremiumChannel();

	private:
		int				m_aiPremiumTimes[PREMIUM_MAX_NUM];
		int				m_iPremiumSubscriptionTime;

		// CHANGE_ITEM_ATTRIBUTES
		static const char		msc_szLastChangeItemAttrFlag[];
		// END_OF_CHANGE_ITEM_ATTRIBUTES

		// NEW_HAIR_STYLE_ADD
	public :
		bool ItemProcess_Hair(LPITEM item, int iDestCell);
		// END_NEW_HAIR_STYLE_ADD

	public :
		void ClearSkill();
		void ClearSubSkill();

		// RESET_ONE_SKILL
		bool ResetOneSkill(DWORD dwVnum);
		// END_RESET_ONE_SKILL

	private :
		void SendDamagePacket(LPCHARACTER pAttacker, int Damage, BYTE DamageFlag);

	// ARENA
	private :
		CArena *m_pArena;
		bool m_ArenaObserver;
		int m_nPotionLimit;

	public :
		void 	SetArena(CArena* pArena) { m_pArena = pArena; }
		void	SetArenaObserverMode(bool flag) { m_ArenaObserver = flag; }

		CArena* GetArena() const { return m_pArena; }
		bool	GetArenaObserverMode() const { return m_ArenaObserver; }

		void	SetPotionLimit(int count) { m_nPotionLimit = count; }
		int		GetPotionLimit() const { return m_nPotionLimit; }
	// END_ARENA

		//PREVENT_TRADE_WINDOW
	public:
		bool	IsOpenSafebox() const { return m_isOpenSafebox ? true : false; }
		void 	SetOpenSafebox(bool b) { m_isOpenSafebox = b; }

		int		GetSafeboxLoadTime() const { return m_iSafeboxLoadTime; }
		void	SetSafeboxLoadTime() { m_iSafeboxLoadTime = thecore_pulse(); }
		//END_PREVENT_TRADE_WINDOW
	private:
		bool	m_isOpenSafebox;

	public:
		int		GetSkillPowerByLevel(int level, bool bMob = false) const;

		//PREVENT_REFINE_HACK
		int		GetRefineTime() const { return m_iRefineTime; }
		void	SetRefineTime() { m_iRefineTime = thecore_pulse(); }
		int		m_iRefineTime;
		//END_PREVENT_REFINE_HACK

		//RESTRICT_USE_SEED_OR_MOONBOTTLE
		int 	GetUseSeedOrMoonBottleTime() const { return m_iSeedTime; }
		void  	SetUseSeedOrMoonBottleTime() { m_iSeedTime = thecore_pulse(); }
		int 	m_iSeedTime;
		//END_RESTRICT_USE_SEED_OR_MOONBOTTLE

		//PREVENT_PORTAL_AFTER_EXCHANGE
		int		GetExchangeTime() const { return m_iExchangeTime; }
		void	SetExchangeTime() { m_iExchangeTime = thecore_pulse(); }
		int		m_iExchangeTime;
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

		int 	m_iMyShopTime;
		int		GetMyShopTime() const	{ return m_iMyShopTime; }
		void	SetMyShopTime() { m_iMyShopTime = thecore_pulse(); }

		bool	IsHack(bool bSendMsg = true, bool bCheckShopOwner = true, int limittime = g_nPortalLimitTime);

		// MONARCH
		BOOL	IsMonarch() const;
		// END_MONARCH
		void Say(const std::string & s);

		enum MONARCH_COOLTIME
		{
			MC_HEAL = 10,
			MC_WARP	= 60,
			MC_TRANSFER = 60,
			MC_TAX = (60 * 60 * 24 * 7),
			MC_SUMMON = (60 * 60),
		};

		enum MONARCH_INDEX
		{
			MI_HEAL = 0,
			MI_WARP,
			MI_TRANSFER,
			MI_TAX,
			MI_SUMMON,
			MI_MAX
		};

		DWORD m_dwMonarchCooltime[MI_MAX]; //todo move inside PlayerSlotT
		DWORD m_dwMonarchCooltimelimit[MI_MAX]; //todo move inside PlayerSlotT

		void  InitMC();
		DWORD GetMC(enum MONARCH_INDEX e) const;
		void SetMC(enum MONARCH_INDEX e);
		bool IsMCOK(enum MONARCH_INDEX e) const;
		DWORD GetMCL(enum MONARCH_INDEX e) const;
		DWORD GetMCLTime(enum MONARCH_INDEX e) const;

	public:
		bool ItemProcess_Polymorph(LPITEM item);

		LPITEM*	GetCubeItem() { return (m_PlayerSlots) ? m_PlayerSlots->pCubeItems.data() : nullptr; }
		bool IsCubeOpen () const	{ return (m_pointsInstant.pCubeNpc); }
		void SetCubeNpc(LPCHARACTER npc)	{ m_pointsInstant.pCubeNpc = npc; }
		bool CanDoCube() const;

	public:
		bool IsSiegeNPC() const;

	private:

		e_overtime m_eOverTime;

	public:
		bool IsOverTime(e_overtime e) const { return (e == m_eOverTime); }
		void SetOverTime(e_overtime e) { m_eOverTime = e; }

	private:
		int		m_deposit_pulse;

	public:
		void	UpdateDepositPulse(DWORD duration=5*60);
		bool	CanDeposit() const;

	private:
		void	__OpenPrivateShop();

	public:
		struct AttackedLog
		{
			DWORD 	dwPID;
			DWORD	dwAttackedTime;

			AttackedLog() : dwPID(0), dwAttackedTime(0)
			{
			}
		};

		AttackLog	m_kAttackLog;
		AttackedLog m_AttackedLog;
		int			m_speed_hack_count;

	private :
		std::string m_strNewName;

	public :
		const std::string GetNewName() const { return this->m_strNewName; }
		void SetNewName(const std::string name) { this->m_strNewName = name; }

	public :
		void GoHome();
		void GoTown();

	private :
		std::set<DWORD>	m_known_guild;

	public :
		void SendGuildName(CGuild* pGuild);
		void SendGuildName(DWORD dwGuildID);

	private :
		DWORD m_dwLogOffInterval;

	public :
		DWORD GetLogOffInterval() const { return m_dwLogOffInterval; }

	public:
		bool UnEquipSpecialRideUniqueItem ();

		bool CanWarp () const;
		bool IsBusy(DWORD exclude = 0) const;

	public:
		void AutoRecoveryItemProcess (const EAffectTypes);

	public:
		void BuffOnAttr_AddBuffsFromItem(LPITEM pItem);
		void BuffOnAttr_RemoveBuffsFromItem(LPITEM pItem);

		typedef std::map <BYTE, CBuffOnAttributes*> TMapBuffOnAttrs;
		TMapBuffOnAttrs m_map_buff_on_attrs;

	public:
		void SetArmada() { cannot_dead = true; }
		void ResetArmada() { cannot_dead = false; }
	private:
		bool cannot_dead;

#ifdef __PET_SYSTEM__
	private:
		bool m_bIsPet;
	public:
		void SetPet() { m_bIsPet = true; }
		bool IsPet() { return m_bIsPet; }
#endif
#ifdef NEW_ICEDAMAGE_SYSTEM
	private:
		DWORD m_dwNDRFlag;
		std::set<DWORD> m_setNDAFlag;
	public:
		const DWORD GetNoDamageRaceFlag();
		void SetNoDamageRaceFlag(DWORD dwRaceFlag);
		void UnsetNoDamageRaceFlag(DWORD dwRaceFlag);
		void ResetNoDamageRaceFlag();
		const std::set<DWORD> & GetNoDamageAffectFlag();
		void SetNoDamageAffectFlag(DWORD dwAffectFlag);
		void UnsetNoDamageAffectFlag(DWORD dwAffectFlag);
		void ResetNoDamageAffectFlag();
#endif

	private:
		float m_fAttMul;
		float m_fDamMul;
	public:
		float GetAttMul() { return this->m_fAttMul; }
		void SetAttMul(float newAttMul) {this->m_fAttMul = newAttMul; }
		float GetDamMul() { return this->m_fDamMul; }
		void SetDamMul(float newDamMul) {this->m_fDamMul = newDamMul; }

		bool IsValidItemPosition(TItemPos Pos) const;

	public:

		void	DragonSoul_Initialize();

		bool	DragonSoul_IsQualified() const;
		void	DragonSoul_GiveQualification();

		int		DragonSoul_GetActiveDeck() const;
		bool	DragonSoul_IsDeckActivated() const;
		bool	DragonSoul_ActivateDeck(int deck_idx);

		void	DragonSoul_DeactivateAll();

		void	DragonSoul_CleanUp();

	public:
		bool		DragonSoul_RefineWindow_Open(LPENTITY pEntity);
		bool		DragonSoul_RefineWindow_Close();
		LPENTITY	DragonSoul_RefineWindow_GetOpener() { return  m_pointsInstant.m_pDragonSoulRefineWindowOpener; }
		bool		DragonSoul_RefineWindow_CanRefine();

	private:
		unsigned int itemAward_vnum;
		char		 itemAward_cmd[20];
	public:
		unsigned int GetItemAward_vnum() { return itemAward_vnum; }
		char*		 GetItemAward_cmd() { return itemAward_cmd;	  }
		void		 SetItemAward_vnum(unsigned int vnum) { itemAward_vnum = vnum; }
		void		 SetItemAward_cmd(char* cmd) { strcpy(itemAward_cmd,cmd); }

	private:
		timeval		m_tvLastSyncTime;
		int			m_iSyncHackCount;
	public:
		void			SetLastSyncTime(const timeval &tv) { memcpy(&m_tvLastSyncTime, &tv, sizeof(timeval)); }
		const timeval&	GetLastSyncTime() { return m_tvLastSyncTime; }
		void			SetSyncHackCount(int iCount) { m_iSyncHackCount = iCount;}
		int				GetSyncHackCount() { return m_iSyncHackCount; }

	private:
		SPECIAL_FLAG_MAP m_mapSpecialFlag;

	public:
		void	OnSetSpecialFlag(std::string flag, long long value);
		void	SetAccountFlag(std::string flag, long long val, bool skipSave = false, bool sendClient = false);
		void	SetSpecialFlag(std::string flag, long long val, bool skipSave = true, bool sendClient = true);
		void	SetSpecialFlag(DWORD flag, long long val, bool skipSave = true);
		long long		GetSpecialFlag(DWORD flagId);
		long long		GetSpecialFlag(std::string flag);
		long long		GetAccountFlag(std::string flag);
		SPECIAL_FLAG_MAP	GetSpecialFlagMap() { return m_mapSpecialFlag; }

		void SetPlayerStat(DWORD dwStatType, long long val);
		void AddPlayerStat(DWORD dwStatType, long long val = 1);

		BYTE GetShopUnlockedProgress();
		bool CanOpenShop();

		int GetReputation(BYTE fraction);
		void SetReputation(BYTE fraction, int value);
		void AddReputation(BYTE fraction, int value);

	// @fixme188 BEGIN
	public:
		void ResetRewardInfo() {
			m_map_kDamage.clear();
			if (!IsPC())
				SetExp(0);
		}
		void DeadNoReward() {
			if (!IsDead())
				ResetRewardInfo();
			Dead();
		}
	// @fixme188 END

#if defined(ENABLE_CHEQUE_SYSTEM) && defined(ENABLE_WON_EXCHANGE_WINDOW)
	public:
		bool	WonExchange(const std::string & option, int value);
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	protected:
		bool	m_bAcceCombination, m_bAcceAbsorption;

	public:
		bool	IsAcceOpened(bool bCombination) {return bCombination ? m_bAcceCombination : m_bAcceAbsorption;}
		bool	IsAcceOpened() {return (m_bAcceCombination || m_bAcceAbsorption);}
		void	OpenAcce(bool bCombination);
		void	CloseAcce();
		void	ClearAcceMaterials();
		bool	CleanAcceAttr(LPITEM pkItem, LPITEM pkTarget);
		std::vector<LPITEM>	GetAcceMaterials();
		const TItemPosEx*	GetAcceMaterialsInfo();
		void	SetAcceMaterial(int pos, LPITEM ptr);
		bool	AcceIsSameGrade(long lGrade);
		YANG	GetAcceCombinePrice(long lGrade);
		void	GetAcceCombineResult(DWORD & dwItemVnum, DWORD & dwMinAbs, DWORD & dwMaxAbs);
		BYTE	CheckEmptyMaterialSlot();
		void	AddAcceMaterial(TItemPos tPos, BYTE bPos);
		void	RemoveAcceMaterial(BYTE bPos);
		BYTE	CanRefineAcceMaterials();
		void	RefineAcceMaterials();
#endif

		int DistanceTo(LPENTITY target);
		int DistanceTo(long x, long y);

		bool IsInRadius(long x, long y, long radius);
		bool IsInOwnM1City();
		bool IsInOwnM2City();

#ifdef ENABLE_MOVE_CHANNEL
	public:
		void CreateMoveChannelEvent(BYTE bDestChannel);
		bool ChangeChannel(const TRespondMoveChannel * respond);
#endif

		void CreateLeaveGuildWarEvent();

#ifdef ENABLE_GUILD_TOKEN_AUTH
	public:
		bool IsGuildMaster() const;
		uint64_t GetGuildToken() const;
		void SendGuildToken();
#endif

#include "char_player.h"
	public:
		CCharacterPlayerData* playerData;
		bool HasPlayerData() const { return IsPC() && playerData != nullptr; }

		void RefreshItemShopData();

		 //ANTYCHEAT
	public:

		struct THackAttackLog {
			DWORD last_attack_time;
			PIXEL_POSITION last_attack_position;
			DWORD combo_checker_time;
			DWORD knockback_victim_time;
			BYTE combo_count;
		};

		PIXEL_POSITION position_before_update;

		typedef std::map<DWORD, THackAttackLog>	TAttackLogMap;
		TAttackLogMap			m_mapAttackLog;

		void			UpdateAttackLog(DWORD vid, DWORD time);
		void			UpdateAttackLog(DWORD vid, THackAttackLog log);
		THackAttackLog& GetAttackLog(DWORD vid);
		void			ClearAttackLog(DWORD vid);
		void			StartResetAttackLogEvent();
		void			IncrementSyncHackCount();
		bool			IsHackBlock();

		const DWORD			GetPlayerRealAttackSpeed(bool getAverage = false, int attackSpeed=-1) const;
		const DWORD			PlayerMoveGetDelayTime(const long to_x, const long to_y) const;

		int	GetValueBasedOnPointValue(int baseValue, DWORD point_type, bool isFloat = false);

		DWORD m_bPlayerFishReactTime;
		int m_iFish_position;
		int m_biFishGameState;
		int m_iFishingChanceBonus;
		bool IsPlayingFishGame() { return m_biFishGameState > 0 && m_pkFishingEvent; }
		void IncrementPlayerFishGamePosition(int add_value) { m_iFish_position = MINMAX(0, m_iFish_position + add_value, 100); }

		bool m_bEntityViewNotSendable;
		bool IsEntityViewSendable() { return !m_bEntityViewNotSendable; }
		void SetEntityViewSendable(bool state) { m_bEntityViewNotSendable = !state; }

		void StartRequestInfoEvent(BYTE requestType);

		void ExpireCaptcha();
		bool IsCaptcha();
		void CaptchaRequestChange();
		void StartCaptcha(CAPTCHA_IMAGE captcha_image, const char* captcha_code, BYTE maxAttempts, DWORD duration, LPCHARACTER invoker);
		void StopCaptcha(bool kill_event);
		void SendCaptchaInfo(CAPTCHA_IMAGE captcha_image, const char* captcha_code, BYTE maxAttempts, DWORD duration, bool isInvoker, LPCHARACTER target=nullptr);
		void SendCaptchaWrongAttempt();
		void SendCaptchaGameMasterFeedback(DWORD pid, BYTE type, const char * code, int elapsedTime=0); // only Game Master

		int analyze_protect{ 0 };
		int analyze_protect_count{ 0 };

		bool bMarkedToBeWarped{ false };

		void SetHorseInventory(BYTE value);
		bool CanUseHorseInventory() const;
		BYTE GetHorseInventoryUnlock() const;
		BYTE GetInventoryMaxCount() const;
		bool IsHorseInventory(TItemPos cell) const;

		BYTE	GetLastMoveFunc() { return m_bLastMoveFunc; }
		void	SetLastMoveFunc(BYTE bFunc) { m_bLastMoveFunc = bFunc; }
		BYTE	m_bLastMoveFunc{};
};
ESex GET_SEX(LPCHARACTER ch);

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
