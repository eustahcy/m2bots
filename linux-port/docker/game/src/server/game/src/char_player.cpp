#include "stdafx.h"
#include "char.h"
#include "char_player.h"
#include "desc_client.h"

#include "config.h"
#include "db.h"
#include "utils.h"
#include "log.h"

void CHARACTER::CCharacterPlayerData::Initialize()
{
	DWORD dword_time = get_dword_time();

	analyze_protect = 0;
	analyze_protect_count = 0;

	for (auto& time : m_dwSkillUseLastTime) {
		time = 0;
	}

	m_lastPickupTime = 0;
	wallHackCount = 0;
	victimCount = 0;
	m_dwRemainingShootCount = 0;
	m_iDesyncHitCount = 0;
	m_LastShootTargetTime = 0;

	m_blockHackTime = dword_time;
	m_lastAttackTime = dword_time;
	m_lastItemEquipTime = dword_time;

	m_iStartFishingTime = m_iEndFishingTime = 0;
	sync_hack_count = 0;
	m_moveCheckerTime = 0;
	m_iMoveBonusPoint = 0; 
	m_iMoveSpeedHackPenalty = 0;
	m_iMoveSpeedHackPunishmentCount = 0;
	m_iMoveCheckTimeBonus = 0;
	m_iMoveBonusPointNextRound = 0;

	m_isMyShopManage = false;
	m_isIsItemShopBrowse = false;
	m_isCrafting = false;
	m_isRefreshItemShopOnOpen = false;
	m_isBusyAction = false;
	m_SpecialShopGuest = {};
	m_bIsPotionRecharge = false;
	m_bIsItemExchange = false;
	bMarkedHacker = false;

	startGoldSessionTimeStamp = get_global_time();

	m_mapRequestInfoResponseReceivedMap.clear();

	m_swapItemCount = 0;
	m_Captcha = {};
	m_mapSpecialFlagSave.clear();
}

void CHARACTER::CCharacterPlayerData::Destroy()
{

}

DWORD CHARACTER::CCharacterPlayerData::GetAllowedMovementDistance(LPCHARACTER ch, bool isRiding, DWORD time, bool isAverage)
{
	DWORD baseDistance;
	if (isAverage)
		baseDistance = isRiding ? MOVE_HORSE_AVERAGE_DISTANCE_PER_SECOND_AND_HALF_DELAY : MOVE_AVERAGE_DISTANCE_PER_SECOND_AND_HALF_DELAY;
	else
		baseDistance = isRiding ? MOVE_HORSE_MAX_DISTANCE_PER_SECOND_AND_HALF_DELAY : MOVE_MAX_DISTANCE_PER_SECOND_AND_HALF_DELAY;

	if (!isRiding)
	{
		int baseMotionMoveSpeed = 449;
		float motionMoveMultiplier = ch->GetMoveMotionSpeed() / baseMotionMoveSpeed;

		baseDistance *= motionMoveMultiplier;
	}


	if (time != 1500)
	{
		DWORD distancePer100ms = baseDistance / 15;
		baseDistance = (time / 100) * distancePer100ms;
	}
	int limitPoint = MAX(m_iMaxMovementSpeedDuringCheck, ch->GetLimitPoint(POINT_MOV_SPEED));

	if (limitPoint < 100)
	{
		DWORD tolerance_multiplier = 100 + MAX(100 - limitPoint, 0);
		baseDistance = baseDistance * tolerance_multiplier / 100;
	}


	ch->ChatDebug("baseDistance %d avg %d", baseDistance, baseDistance * limitPoint / 100);

	return baseDistance * limitPoint / 100;
}

void CHARACTER::CCharacterPlayerData::SetBusyAction(LPCHARACTER ch, DWORD dwDuration, BYTE bActionType, bool bSuccess)
{
	if (!ch)
		return;

	m_isBusyAction = dwDuration > 0;

	LPDESC d = ch->GetDesc();
	if (d)
	{
		TPacketGCBusyAction pack;
		pack.bHeader = HEADER_GC_BUSY_ACTION;
		pack.bActionType = bActionType;
		pack.dwDuration = dwDuration;
		pack.bSuccess = bSuccess;
		d->Packet(&pack, sizeof(TPacketGCBusyAction));
	}
}

void CHARACTER::CCharacterPlayerData::SetSpecialShopGuest(DWORD vid, DWORD shop_vnum)
{
	m_SpecialShopGuest = CCharacterPlayerData::TSpecialShopGuest{ vid, shop_vnum };
}

void CHARACTER::CCharacterPlayerData::SetSpecialFlagSave(std::string flag, long long value)
{
	auto it = m_mapSpecialFlagSave.find(flag);
	if (it == m_mapSpecialFlagSave.end()) {
		m_mapSpecialFlagSave[flag] = value;
	}
	else {
		it->second = value;
	}
}

void CHARACTER::CCharacterPlayerData::SaveSpecialFlag(DWORD pid, DWORD aid)
{
	if (m_mapSpecialFlagSave.empty())
		return;

	std::vector<TSpecialFlagTable> s_table;
	s_table.resize(m_mapSpecialFlagSave.size());

	int i = 0;

	for (auto it = m_mapSpecialFlagSave.begin(); it != m_mapSpecialFlagSave.end(); ++it)
	{
		const std::string szFlag = it->first;
		long long llValue = it->second;

		if (szFlag.length() == 0)
		{
			sys_err("CHARACTER::CCharacterPlayerData::SaveSpecialFlag : invalid special flag data pid %d aid %d", pid, aid);
			continue;
		}

		sys_log(1, "SPECIAL FLAG Save %s %lld (%d)", szFlag.c_str(), llValue, i);

		if (szFlag.length() >= SPECIAL_FLAG_MAX_LEN)
		{
			sys_err("CHARACTER::CCharacterPlayerData::SaveSpecialFlag : flag name overflow");
			continue;
		}

		TSpecialFlagTable& r = s_table[i];

		std::string accountPrefix = "acc.";
		bool isAccountFlag = szFlag.substr(0, accountPrefix.size()) == accountPrefix;

		r.dwPID = isAccountFlag ? 0 : pid;
		r.dwAID = isAccountFlag ? aid : 0;
		strlcpy(r.szFlag, szFlag.c_str(), sizeof(r.szFlag));
		r.llValue = llValue;
		i++;
	}

	if (i > 0)
	{
		sys_log(1, "CCharacterPlayerData::SetSpecialFlagSave %d", i);
		db_clientdesc->DBPacketHeader(HEADER_GD_SPECIAL_FLAG_SAVE, 0, sizeof(TSpecialFlagTable) * i);
		db_clientdesc->Packet(&s_table[0], sizeof(TSpecialFlagTable) * i);
	}

	m_mapSpecialFlagSave.clear();
}

void CHARACTER::CCharacterPlayerData::SendSpecialFlagsToClient(LPCHARACTER ch)
{
	if (!ch)
		return;
	auto map = ch->GetSpecialFlagMap();
	for (auto it = map.begin(); it != map.end(); ++it)
	{
		const std::string szFlag = it->first;
		long long llValue = it->second;

		if (llValue > 0 && IS_SPECIAL_FLAG_SEND_CLIENT(szFlag))
		{
			TPacketGCSpecialFlag pack;
			pack.header = HEADER_GC_SPECIAL_FLAG;
			strlcpy(pack.cFlag, szFlag.c_str(), sizeof(pack.cFlag));
			pack.llValue = llValue;
			ch->GetDesc()->Packet(&pack, sizeof(TPacketGCSpecialFlag));
		}
	}
}

bool CHARACTER::CCharacterPlayerData::IsSummonItemDelay(LPCHARACTER ch)
{
	if (!ch)
		return false;

	int delta_time = ch->GetQuestFlag("summon_item.delay") - get_global_time();

	ch->ChatDebug("zostalo: %d", delta_time);

	if (delta_time > 0)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("WAIT_%d_BEFORE_USING_ITEM"), delta_time);

	return delta_time > 0;
}

BYTE CHARACTER::CCharacterPlayerData::GetRequestInfoResponseState(BYTE requestType)
{
	auto it = m_mapRequestInfoResponseReceivedMap.find(requestType);
	if (it == m_mapRequestInfoResponseReceivedMap.end())
		return REQUEST_INFO_STATE_NONE;

	return it->second;
}

bool CHARACTER::CCharacterPlayerData::IsRequestInfoResponseWaiting(BYTE requestType)
{
	return GetRequestInfoResponseState(requestType) == REQUEST_INFO_STATE_WAITING;
}

bool CHARACTER::CCharacterPlayerData::IsRequestInfoResponseReceived(BYTE requestType)
{
	return GetRequestInfoResponseState(requestType) == REQUEST_INFO_STATE_RECEIVED;
}

void CHARACTER::CCharacterPlayerData::SetRequestInfoResponseState(BYTE requestType, BYTE state)
{
	m_mapRequestInfoResponseReceivedMap[requestType] = state;
}

bool CHARACTER::CCharacterPlayerData::IncreaseDesyncHitCount()
{
	if (++m_iDesyncHitCount >= 4)
	{
		m_iDesyncHitCount = 0;
		return true;
	}
	return false;
}

void CHARACTER::CCharacterPlayerData::SetSummonItemDelay(LPCHARACTER ch, int duration)
{
	if (get_dword_time() - ch->GetLastAttackTime() <= 5000)
		duration *= 2;

	ch->ChatDebug("ustawiam czas na %d", get_global_time() + duration);
	ch->SetQuestFlag("summon_item.delay", get_global_time() + duration);
}

void CHARACTER::CCharacterPlayerData::SetPlayerCaptcha(const char* code, int attempts, DWORD expireTime, LPCHARACTER invoker)
{
	ClearPlayerCaptcha();

	strncpy(m_Captcha.code, code, sizeof(m_Captcha.code));
	m_Captcha.attempts = attempts;
	m_Captcha.expireTime = expireTime;
	if (invoker)
		m_Captcha.invokerPID = invoker->GetPlayerID();
}

bool CHARACTER::CCharacterPlayerData::HasCaptchaExpired()
{
	return m_Captcha.expireTime < get_global_time();
}

BYTE CHARACTER::CCharacterPlayerData::VerifyCaptcha(const char* code)
{
	auto &captcha = GetPlayerCaptcha();

	if (HasCaptchaExpired())
		return CAPTCHA_EXPIRE;

	if (captcha.attempts <= 0)
		return CAPTCHA_NO_ATTEMPTS;

	if (strcmp(code, captcha.code))
	{
		captcha.attempts--;

		if (captcha.attempts == 0)
			return CAPTCHA_NO_ATTEMPTS;

		return CAPTCHA_WRONG_CODE;
	}

	return CAPTCHA_OK;
}


void CHARACTER::CCharacterPlayerData::MarkHacker(const char* hackName, LPCHARACTER ch)
{
	if (bMarkedHacker)
		return;

	//ch->ChatDebug("Marking as hacker (hack: %s)", hackName);
	LogManager::instance().HackLog(hackName, ch);
	bMarkedHacker = true;
}

void CHARACTER::CCharacterPlayerData::MarkHackerReleased(LPCHARACTER ch)
{
	if (!bMarkedHacker)
		return;

	//ch->ChatDebug("Marking as released.");
	LogManager::instance().HackLog("BOT_RELEASED", ch);
	bMarkedHacker = false;
}

// Files shared by GameCore.top
