#include "stdafx.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "packet.h"
#include "guild.h"
#include "vector.h"
#include "questmanager.h"
#include "item.h"
#include "horsename_manager.h"
#include "locale_service.h"
#include "arena.h"
#include "war_map.h"

#include "../../common/VnumHelper.h"

bool CHARACTER::StartRiding()
{
#ifdef ENABLE_NEWSTUFF
	if (g_NoMountAtGuildWar && GetWarMap())
	{
		RemoveAffect(AFFECT_MOUNT);
		RemoveAffect(AFFECT_MOUNT_BONUS);
		if (IsRiding())
			StopRiding();
		return false;
	}
#endif
	if (IsDead() == true)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot ride while falling."));
		return false;
	}

	if (IsAffectFlag(AFF_QUEST_BOX))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_RIDE_WHILE_CARRY_ITEM"));
		return false;
	}

	if (IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot ride a Horse while you are transformed."));
		return false;
	}

	if (m_pkFishingEvent || m_pkPreFishingEvent || HasPlayerData() && get_dword_time() - playerData->m_iEndFishingTime < 2500)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("FISHING_BLOCK_MOUNT"));
		return false;
	}

	LPITEM armor = GetWear(WEAR_BODY);

	if (armor && (armor->GetVnum() >= 11901 && armor->GetVnum() <= 11904))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot ride while you are wearing a Wedding Dress or a Tuxedo."));
		return false;
	}

	if (IsBusy())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
		return false;
	}

	// @warme005
	if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
		return false;

	DWORD dwMountVnum = m_chHorse ? m_chHorse->GetRaceNum() : GetMyHorseVnum();

	if (false == CHorseRider::StartRiding())
	{
		if (GetHorseLevel() <= 0)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have a Horse."));
		else if (GetHorseStamina() <= 0)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your Horse's endurance is too low."));

		return false;
	}

	HorseSummon(false);

	MountVnum(dwMountVnum);

	int healthGrade = GetHorseHealthGrade();
	if (IsSprint() && healthGrade <= 1)
	{
		SetSprint(false);
	}

	if (healthGrade == 0)
	{
		AddAffect(AFFECT_HORSE_PENALTY, POINT_ATT_SPEED, -20, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true, true);
		AddAffect(AFFECT_HORSE_PENALTY, POINT_MOV_SPEED, -30, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true, true);
	}
	else if (healthGrade == 1)
	{
		AddAffect(AFFECT_HORSE_PENALTY, POINT_ATT_SPEED, -10, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true, true);
		AddAffect(AFFECT_HORSE_PENALTY, POINT_MOV_SPEED, -15, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true, true);
	}

	if(test_server)
		sys_log(0, "Ride Horse : %s ", GetName());

	m_dwLastStartRiding = get_dword_time();

	return true;
}

void CHARACTER::OnStopRiding()
{
	if (HasPlayerData())
	{
		// when player dismounts just before the timer
		DWORD riding_duration = get_dword_time() - m_dwLastStartRiding;
		DWORD move_duration_since_check = get_dword_time() - playerData->m_moveCheckerTime;
		DWORD riding_duration_since_check = MIN(move_duration_since_check, riding_duration);

		int ratio = riding_duration_since_check * 100 / 1500;

		int maxAllowedHorseDistance = playerData->GetAllowedMovementDistance(this, true);
		int maxAllowedWalkDistance = playerData->GetAllowedMovementDistance(this, false);

		int maxAllowedHorseDistanceInTime = maxAllowedHorseDistance * ratio / 100;
		int maxAllowedWalkDistanceInTime = maxAllowedWalkDistance * ratio / 100;

		playerData->m_iMoveBonusPointNextRound += MAX(0, maxAllowedHorseDistanceInTime - maxAllowedWalkDistanceInTime);
		playerData->m_iMoveBonusPoint = MAX(0, playerData->m_iMoveBonusPoint - 1500);
	}
}

void CHARACTER::SetStopStaminaConsume(bool state)
{
	CHorseRider::SetStopStaminaConsume(state);
}

bool CHARACTER::StopRiding()
{
	if (IsBusy())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
		return false;
	}

	if (CHorseRider::StopRiding())
	{
		quest::CQuestManager::instance().Unmount(GetPlayerID());

		if (!IsDead() && !IsStun())
		{
			DWORD dwOldVnum = GetMountVnum();
			MountVnum(0);

			HorseSummon(true, false, dwOldVnum);
		}
		else
		{
			m_dwMountVnum = 0;
			ComputePoints();
			UpdatePacket();
		}

		OnStopRiding();

		PointChange(POINT_ST, 0);
		PointChange(POINT_DX, 0);
		PointChange(POINT_HT, 0);
		PointChange(POINT_IQ, 0);

		if (GetHorseHealthGrade() < 2)
		{
			CAffect* pkAff1 = FindAffect(AFFECT_HORSE_PENALTY, POINT_MOV_SPEED);
			CAffect* pkAff2 = FindAffect(AFFECT_HORSE_PENALTY, POINT_ATT_SPEED);
			RemoveAffect(pkAff1);
			RemoveAffect(pkAff2);
		}

		return true;
	}

	return false;
}

EVENTFUNC(horse_dead_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "horse_dead_event> <Factor> Null pointer" );
		return 0;
	}

	// <Factor>
	LPCHARACTER ch = info->ch;
	if (ch == NULL) {
		return 0;
	}
	ch->HorseSummon(false);
	return 0;
}

void CHARACTER::SetRider(LPCHARACTER ch)
{
	if (m_chRider)
		m_chRider->ClearHorseInfo();

	m_chRider = ch;

	if (m_chRider)
		m_chRider->SendHorseInfo();
}

LPCHARACTER CHARACTER::GetRider() const
{
	return m_chRider;
}

void CHARACTER::HorseSummon(bool bSummon, bool bFromFar, DWORD dwVnum, const char* pPetName)
{
	if ( bSummon )
	{
		if( m_chHorse != NULL )
			return;

		if (GetHorseLevel() <= 0)
			return;

		if (IsRiding())
			return;

		if (CWarMapManager::instance().IsWarMap(GetMapIndex()))
			return;

		#ifdef ENABLE_MOUNT_COSTUME_EX_SYSTEM
		if (GetMountVnum())
			return;
		#endif

		sys_log(0, "HorseSummon : %s lv:%d bSummon:%d fromFar:%d (vnum %d)", GetName(), GetLevel(), bSummon, bFromFar, dwVnum);

		long x = GetX();
		long y = GetY();

		if (bFromFar)
		{
			x += (number(0, 1) * 2 - 1) * number(2000, 2500);
			y += (number(0, 1) * 2 - 1) * number(2000, 2500);
		}
		else
		{
			x += number(-100, 100);
			y += number(-100, 100);
		}

		m_chHorse = CHARACTER_MANAGER::instance().SpawnMob(
				(0 == dwVnum) ? GetMyHorseVnum() : dwVnum,
				GetMapIndex(),
				x, y,
				GetZ(), false, (int)(GetRotation()+180), false);

		if (!m_chHorse)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Calling the Horse has failed."));
			return;
		}

		m_chHorse->SetLevel(GetHorseLevel());

		const char* pHorseName = CHorseNameManager::instance().GetHorseName(GetPlayerID());

		if ( pHorseName != NULL && strlen(pHorseName) != 0 )
		{
			m_chHorse->m_stName = pHorseName;
		}
		else
		{
			m_chHorse->m_stName = GetName();
			m_chHorse->m_stName += LC_TEXT("'s Horse");
		}

		if (!m_chHorse->Show(GetMapIndex(), x, y, GetZ()))
		{
			M2_DESTROY_CHARACTER(m_chHorse);
			sys_err("cannot show monster");
			m_chHorse = NULL;
			return;
		}

		m_chHorse->SetRider(this);
	}
	else
	{
		if (!m_chHorse)
			return;

		LPCHARACTER chHorse = m_chHorse;

		chHorse->SetRider(nullptr);

		if (!bFromFar)
		{
			M2_DESTROY_CHARACTER(chHorse);
		}
		else
		{
			chHorse->SetNowWalking(false);
			float fx, fy;
			chHorse->SetRotation(GetDegreeFromPositionXY(chHorse->GetX(), chHorse->GetY(), GetX(), GetY())+180);
			GetDeltaByDegree(chHorse->GetRotation(), 3500, &fx, &fy);
			chHorse->Goto((long)(chHorse->GetX()+fx), (long) (chHorse->GetY()+fy));
			chHorse->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
		}

		m_chHorse = NULL;

	}
}

DWORD CHARACTER::GetMyHorseVnum() const
{
	int delta = 0;

	if (GetGuild())
	{
		++delta;

		if (GetGuild()->GetMasterPID() == GetPlayerID())
			++delta;
	}

	return c_aHorseStat[GetHorseLevel()].iNPCRace + delta;
}

void CHARACTER::ClearHorseInfo()
{
	if (!IsHorseRiding())
	{
		ChatPacket(CHAT_TYPE_COMMAND, "hide_horse_state");

		m_bSendHorseLevel = 0;
		m_bSendHorseHealthGrade = 0;
		m_bSendHorseStaminaGrade = 0;
	}

	m_chHorse = NULL;
}

void CHARACTER::SendHorseInfo()
{
	if (m_chHorse || IsHorseRiding())
	{
		int iHealthGrade = GetHorseHealthGrade();
		int iStaminaGrade;

		if (GetHorseStamina() * 10 <= GetHorseMaxStamina())
			iStaminaGrade = 0;
		else if (GetHorseStamina() * 10 <= GetHorseMaxStamina() * 3)
			iStaminaGrade = 1;
		else if (GetHorseStamina() * 10 <= GetHorseMaxStamina() * 7)
			iStaminaGrade = 2;
		else
			iStaminaGrade = 3;

		if (m_bSendHorseLevel != GetHorseLevel() ||
				m_bSendHorseHealthGrade != iHealthGrade ||
				m_bSendHorseStaminaGrade != iStaminaGrade)
		{
			ChatPacket(CHAT_TYPE_COMMAND, "horse_state %d %d %d", GetHorseLevel(), iHealthGrade, iStaminaGrade);

			m_bSendHorseLevel = GetHorseLevel();
			m_bSendHorseHealthGrade = iHealthGrade;
			m_bSendHorseStaminaGrade = iStaminaGrade;
		}
	}
}

bool CHARACTER::CanUseHorseSkill()
{
	if(IsRiding())
	{
		if (GetHorseGrade() == 3 && GetHorseHealth() > 0)
			return true;
		else
			return false;

		if(GetMountVnum())
		{
			if (GetMountVnum() >= 20209 && GetMountVnum() <= 20212)
				return true;

			if (CMobVnumHelper::IsRamadanBlackHorse(GetMountVnum()))
				return true;
		}
		else
			return false;

	}

	return false;
}

void CHARACTER::SetHorseLevel(int iLevel)
{
	CHorseRider::SetHorseLevel(iLevel);
	SetSkillLevel(SKILL_HORSE, GetHorseLevel());
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
