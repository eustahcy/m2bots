class CCharacterPlayerData
{
public:

	void Initialize();
	void Destroy();

	int analyze_protect;
	int analyze_protect_count;

	// CHEATWARDEN START
	DWORD GetAllowedMovementDistance(LPCHARACTER ch, bool isRiding = false, DWORD time = 1500, bool isAverage = true);

	int				wallHackCount;
	int				victimCount;
	DWORD			sync_hack_count;
	DWORD			m_blockHackTime;
	DWORD			m_lastAttackTime;
	DWORD			m_lastPickupTime;

	DWORD			m_swapItemCount;
	DWORD			m_lastItemEquipTime;

	DWORD			m_dwSkillUseLastTime[SKILL_MAX_NUM];

	PIXEL_POSITION last_checked_position;
	PIXEL_POSITION		use_skill_position;
	int			m_iSkillPushCount;
	DWORD			m_moveCheckerTime;
	int			m_iMoveBonusPoint;
	int			m_iMoveSpeedHackPenalty;
	int			m_iMoveSpeedHackPunishmentCount;
	int			m_iMoveBonusPointNextRound;
	int			m_iMoveCheckTimeBonus;
	int			m_iMaxMovementSpeedDuringCheck;

	std::map<BYTE, BYTE>	m_mapRequestInfoResponseReceivedMap;

	DWORD			m_iStartFishingTime;
	DWORD			m_iEndFishingTime;
	// END OF CHEATWARDEN

	DWORD		m_dwRemainingShootCount;

	bool	m_isBusyAction;
	void 	SetBusyAction(LPCHARACTER ch, DWORD dwDuration, BYTE bActionType = BUSY_ACTION_GENERAL, bool bSuccess = false);

	bool	m_isRefreshItemShopOnOpen;
	bool	m_isIsItemShopBrowse;
	bool	IsItemShopBrowse() const { return m_isIsItemShopBrowse; }
	void 	SetItemShopBrowse(bool b) { m_isIsItemShopBrowse = b; }

	bool	m_isCrafting;
	bool	IsCrafting() const { return m_isCrafting; }
	void 	SetCrafting(bool b) { m_isCrafting = b; }

	bool	m_isMyShopManage;
	bool	IsMyShopManage() const { return m_isMyShopManage; }
	void 	SetMyShopManage(bool b) { m_isMyShopManage = b; }

	struct TSpecialShopGuest
	{
		DWORD vid;
		DWORD shop_vnum;
	};
	
	TSpecialShopGuest	m_SpecialShopGuest;
	bool	IsSpecialShopGuest() const { return m_SpecialShopGuest.vid > 0; }
	TSpecialShopGuest	GetSpecialShopGuest() const { return m_SpecialShopGuest; }
	void 	SetSpecialShopGuest(DWORD vid, DWORD shop_vnum);

	bool m_bIsPotionRecharge;
	bool IsPotionRecharge() const { return m_bIsPotionRecharge; }
	void SetPotionRecharge(bool value) { m_bIsPotionRecharge = value; }

	bool m_bIsItemExchange;
	bool IsItemExchange() const { return m_bIsItemExchange; }
	void SetItemExchange(bool value) { m_bIsItemExchange = value; }

	std::map<const std::string, long long> m_mapSpecialFlagSave;
	void SetSpecialFlagSave(std::string flag, long long value);
	void SaveSpecialFlag(DWORD pid, DWORD aid);
	void SendSpecialFlagsToClient(LPCHARACTER ch);

	void SetSummonItemDelay(LPCHARACTER ch, int duration);
	bool IsSummonItemDelay(LPCHARACTER ch);

	BYTE GetRequestInfoResponseState(BYTE requestType);
	bool IsRequestInfoResponseWaiting(BYTE requestType);
	bool IsRequestInfoResponseReceived(BYTE requestType);
	void SetRequestInfoResponseState(BYTE requestType, BYTE state);

	int			m_iDesyncHitCount;
	bool		IncreaseDesyncHitCount();

	DWORD		m_LastShootTargetTime;


	struct TPlayerCaptcha
	{
		int attempts;
		int changeRequests;
		DWORD expireTime;
		char code[8 + 1];
		DWORD invokerPID;
	};
	TPlayerCaptcha m_Captcha;
	TPlayerCaptcha&	GetPlayerCaptcha() { return m_Captcha; }
	bool HasCaptchaExpired();
	void SetPlayerCaptcha(const char * code, int attempts, DWORD expireTime, LPCHARACTER invoker);
	void ClearPlayerCaptcha() { m_Captcha = {}; }

	BYTE VerifyCaptcha(const char* code);

	DWORD m_dwEscapeCooltime{};
	void SetEscapeCooltime(const DWORD dwTime) { m_dwEscapeCooltime = dwTime; }
	DWORD GetEscapeCooltime() const { return m_dwEscapeCooltime; }

	bool bMarkedHacker{};
	bool IsHacker() { return bMarkedHacker; }
	void MarkHacker(const char* hackName, LPCHARACTER ch);
	void MarkHackerReleased(LPCHARACTER ch);

	YANG totalGoldSession{};
	DWORD startGoldSessionTimeStamp{};
	YANG GetTotalGoldSession() { return totalGoldSession; }
	void AddTotalGoldSession(YANG val) { totalGoldSession += val; }
};
// Files shared by GameCore.top
