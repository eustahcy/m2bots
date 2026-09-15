#include "stdafx.h"

#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "affect.h"
#include "packet.h"
#include "buffer_manager.h"
#include "desc_client.h"
#include "battle.h"
#include "guild.h"
#include "utils.h"
#include "locale_service.h"
#include "lua_incl.h"
#include "arena.h"
#include "horsename_manager.h"
#include "item.h"
#include "DragonSoul.h"
#include "war_map.h"
#include "pvp.h"
#include "../../common/CommonDefines.h"

#define IS_MONKEY_CURSE_AFFECT(type) (type == AFFECT_MONKEY_CURSE_EASY || type == AFFECT_MONKEY_CURSE_NORMAL || type == AFFECT_MONKEY_CURSE_HARD)

#define IS_NO_SAVE_AFFECT(type) ((type) == AFFECT_WAR_FLAG || (type) == AFFECT_REVIVE_INVISIBLE || ((type) >= AFFECT_PREMIUM_START && (type) <= AFFECT_PREMIUM_END) || (type) == AFFECT_PREMIUM_SUBSCRIPTION || (type) == AFFECT_TEMPLE_CURSE)
#define IS_NO_CLEAR_ON_DEATH_AFFECT_POTION(type, point) ((type) >= AFFECT_POTION_START && (type) <= AFFECT_POTION_END && point != POINT_MOV_SPEED && point != POINT_ATT_SPEED)
#define IS_NO_CLEAR_ON_DEATH_AFFECT(type) ((type) == AFFECT_BLOCK_CHAT || ((type) >= 500 && (type) < 600) || (type) == AFFECT_TEMPLE_CURSE || (type) == AFFECT_EXP_BLOCK || (type) == AFFECT_COLLECT_NO_TIME || (type) == AFFECT_COLLECT_NO_FAIL_TIME || (type) == AFFECT_LEARN_NO_FAIL_TIME) || IS_MONKEY_CURSE_AFFECT(type) || (type) == AFFECT_DEVILTOWER_DAMAGE || type == AFFECT_FAST_PICKUP || type == AFFECT_SPY || type == AFFECT_HERBALIST_BONUS || type == AFFECT_NO_ITEM_DEATH_PENALTY
#define IS_TARGET_SHOW_AFFECT(type) (type == AFFECT_POISON || type == AFFECT_SLOW || type == AFFECT_STUN || type == AFFECT_FIRE || type == AFFECT_HEAL_ON_DAMAGE || type == AFFECT_REMOVE_ALL_RESIST)


void SendAffectRemovePacket(LPDESC d, DWORD pid, DWORD type, BYTE point)
{
	TPacketGCAffectRemove ptoc;
	ptoc.bHeader	= HEADER_GC_AFFECT_REMOVE;
	ptoc.dwType		= type;
	ptoc.bApplyOn	= point;
	d->Packet(&ptoc, sizeof(TPacketGCAffectRemove));

	TPacketGDRemoveAffect ptod;
	ptod.dwPID		= pid;
	ptod.dwType		= type;
	ptod.bApplyOn	= point;
	db_clientdesc->DBPacket(HEADER_GD_REMOVE_AFFECT, 0, &ptod, sizeof(ptod));
}

void SendAffectAddPacket(LPDESC d, CAffect * pkAff)
{
	TPacketGCAffectAdd ptoc;
	ptoc.bHeader		= HEADER_GC_AFFECT_ADD;
	ptoc.elem.dwType		= pkAff->dwType;
	ptoc.elem.bApplyOn		= pkAff->bApplyOn;
	ptoc.elem.lApplyValue	= pkAff->lApplyValue;
	ptoc.elem.dwFlag		= pkAff->dwFlag;
	ptoc.elem.lDuration		= pkAff->lDuration;
	ptoc.elem.lSPCost		= pkAff->lSPCost;
	ptoc.elem.dwItemVnum = pkAff->dwItemVnum;
	d->Packet(&ptoc, sizeof(TPacketGCAffectAdd));
}
////////////////////////////////////////////////////////////////////
// Affect
int CHARACTER::CountAffect(DWORD dwType) const
{
	itertype(m_list_pkAffect) it = m_list_pkAffect.begin();

	int count = 0;
	while (it != m_list_pkAffect.end())
	{
		CAffect* pkAffect = *it++;

		if (pkAffect->dwType == dwType)
			count++;
	}

	return count;
}

CAffect * CHARACTER::FindAffect(DWORD dwType, BYTE bApply) const
{
	itertype(m_list_pkAffect) it = m_list_pkAffect.begin();

	while (it != m_list_pkAffect.end())
	{
		CAffect * pkAffect = *it++;

		if (pkAffect->dwType == dwType && (bApply == POINT_NONE || bApply == pkAffect->bApplyOn))
			return pkAffect;
	}

	return NULL;
}

EVENTFUNC(affect_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "affect_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (!ch->UpdateAffect())
		return 0;
	else
		return passes_per_sec;
}

bool CHARACTER::UpdateAffect()
{
	if (GetPoint(POINT_HP_RECOVERY) > 0)
	{
		if (GetMaxHP() <= GetHP())
		{
			PointChange(POINT_HP_RECOVERY, -GetPoint(POINT_HP_RECOVERY));
		}
		else
		{
			int max_maxhp_percent_regen = 70 * (100 + GetPoint(POINT_HP_REGEN)) / 100;
			int max_regen_value = GetMaxHP() * max_maxhp_percent_regen / 1000;
			int iVal = MIN(GetPoint(POINT_HP_RECOVERY), max_regen_value);

			PointChange(POINT_HP, iVal);
			PointChange(POINT_HP_RECOVERY, -iVal);
		}
	}

	if (GetPoint(POINT_SP_RECOVERY) > 0)
	{
		if (GetMaxSP() <= GetSP())
			PointChange(POINT_SP_RECOVERY, -GetPoint(POINT_SP_RECOVERY));
		else
		{
			int max_maxsp_percent_regen = 70 * (100 + GetPoint(POINT_SP_REGEN)) / 100;
			int max_regen_value = GetMaxSP() * max_maxsp_percent_regen / 1000;
			int iVal = MIN(GetPoint(POINT_SP_RECOVERY), max_regen_value);

			PointChange(POINT_SP, iVal);
			PointChange(POINT_SP_RECOVERY, -iVal);
		}
	}

	if (GetPoint(POINT_HP_RECOVER_CONTINUE) > 0)
	{
		PointChange(POINT_HP, GetPoint(POINT_HP_RECOVER_CONTINUE));
	}

	if (GetPoint(POINT_SP_RECOVER_CONTINUE) > 0)
	{
		PointChange(POINT_SP, GetPoint(POINT_SP_RECOVER_CONTINUE));
	}

	AutoRecoveryItemProcess( AFFECT_AUTO_HP_RECOVERY );
	AutoRecoveryItemProcess( AFFECT_AUTO_SP_RECOVERY );

	if (ProcessAffect())
		if (GetPoint(POINT_HP_RECOVERY) == 0 && GetPoint(POINT_SP_RECOVERY) == 0 && GetStamina() == GetMaxStamina())
		{
			m_pkAffectEvent = NULL;
			return false;
		}

	return true;
}

void CHARACTER::StartAffectEvent()
{
	if (m_pkAffectEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();
	info->ch = this;
	m_pkAffectEvent = event_create(affect_event, info, passes_per_sec);
	sys_log(1, "StartAffectEvent %s %p %p", GetName(), this, get_pointer(m_pkAffectEvent));
}

void CHARACTER::ClearAffect(bool bSave)
{
	TAffectFlag afOld = m_afAffectFlag;
	WORD	wMovSpd = GetPoint(POINT_MOV_SPEED);
	WORD	wAttSpd = GetPoint(POINT_ATT_SPEED);

	itertype(m_list_pkAffect) it = m_list_pkAffect.begin();

	bool isClearTargetShowAffect = false;
	while (it != m_list_pkAffect.end())
	{
		CAffect * pkAff = *it;
		DWORD dwType = pkAff->dwType;
		if (bSave)
		{
			if (IS_NO_CLEAR_ON_DEATH_AFFECT(dwType) ||
				IS_NO_SAVE_AFFECT(dwType) ||
				IS_NO_CLEAR_ON_DEATH_AFFECT_POTION(dwType, pkAff->bApplyOn))
			{
				++it;
				continue;
			}

			if (IsPC())
			{
				SendAffectRemovePacket(GetDesc(), GetPlayerID(), pkAff->dwType, pkAff->bApplyOn);
			}
		}

		ComputeAffect(pkAff, false);

		it = m_list_pkAffect.erase(it);
		CAffect::Release(pkAff);

		if (IS_TARGET_SHOW_AFFECT(dwType))
		{
			isClearTargetShowAffect = true;
		}
	}

	if (afOld != m_afAffectFlag ||
			wMovSpd != GetPoint(POINT_MOV_SPEED) ||
			wAttSpd != GetPoint(POINT_ATT_SPEED))
		UpdatePacket();

	CheckMaximumPoints();

	if (isClearTargetShowAffect)
		BroadcastTargetPacket(true);

	if (m_list_pkAffect.empty())
		event_cancel(&m_pkAffectEvent);
}

int CHARACTER::ProcessAffect()
{
	bool	bDiff	= false;
	CAffect	*pkAff	= NULL;

	if (IsActivePremiumSubscription())
	{
		pkAff = FindAffect(AFFECT_PREMIUM_SUBSCRIPTION);
		if (pkAff)
		{
			int remain = m_iPremiumSubscriptionTime;
			if (remain < 0)
			{
				RemoveAffect(AFFECT_PREMIUM_SUBSCRIPTION);
				bDiff = true;
			}
			else
				pkAff->lDuration = remain + 1;
		}
	}

	for (int i = 0; i <= PREMIUM_MAX_NUM; ++i)
	{
		int aff_idx = i + AFFECT_PREMIUM_START;

		pkAff = FindAffect(aff_idx);

		if (!pkAff)
			continue;

		int remain = GetPremiumRemainSeconds(i);

		if (remain < 0)
		{
			RemoveAffect(aff_idx);
			bDiff = true;
		}
		else
			pkAff->lDuration = remain + 1;
	}

	////////// HAIR_AFFECT
	pkAff = FindAffect(AFFECT_HAIR);
	if (pkAff)
	{
		// IF HAIR_LIMIT_TIME() < CURRENT_TIME()
		if ( this->GetQuestFlag("hair.limit_time") < get_global_time())
		{
			// SET HAIR NORMAL
			this->SetPart(PART_HAIR, 0);
			// REMOVE HAIR AFFECT
			RemoveAffect(AFFECT_HAIR);
		}
		else
		{
			// INCREASE AFFECT DURATION
			++(pkAff->lDuration);
		}
	}
	////////// HAIR_AFFECT

	TAffectFlag afOld = m_afAffectFlag;
	long lMovSpd = GetPoint(POINT_MOV_SPEED);
	long lAttSpd = GetPoint(POINT_ATT_SPEED);

	itertype(m_list_pkAffect) it;

	it = m_list_pkAffect.begin();

	while (it != m_list_pkAffect.end())
	{
		pkAff = *it;

		bool bEnd = false;

		if (pkAff->dwType >= GUILD_SKILL_START && pkAff->dwType <= GUILD_SKILL_END)
		{
			if (!GetGuild() || !GetGuild()->UnderAnyWar() || !CWarMapManager::instance().IsWarMap(GetMapIndex()))
				bEnd = true;
		}

		if (pkAff->lSPCost > 0)
		{
			if (GetSP() < pkAff->lSPCost)
				bEnd = true;
			else
				PointChange(POINT_SP, -pkAff->lSPCost);
		}

		// AFFECT_DURATION_BUG_FIX

		if ( --pkAff->lDuration <= 0 )
		{
			bEnd = true;
		}
		// END_AFFECT_DURATION_BUG_FIX

		if (bEnd)
		{
			it = m_list_pkAffect.erase(it);
			ComputeAffect(pkAff, false);
			bDiff = true;
			if (IsPC())
			{
				SendAffectRemovePacket(GetDesc(), GetPlayerID(), pkAff->dwType, pkAff->bApplyOn);
			}
			else
			{
				if (IS_TARGET_SHOW_AFFECT(pkAff->dwType))
				{
					BroadcastTargetPacket(true);
				}
			}

			CAffect::Release(pkAff);

			continue;
		}

		++it;
	}

	if (bDiff)
	{
		if (afOld != m_afAffectFlag ||
				lMovSpd != GetPoint(POINT_MOV_SPEED) ||
				lAttSpd != GetPoint(POINT_ATT_SPEED))
		{
			UpdatePacket();
		}

		CheckMaximumPoints();
	}

	if (m_list_pkAffect.empty())
		return true;

	return false;
}

void CHARACTER::SaveAffect()
{
	TPacketGDAddAffect p;

	itertype(m_list_pkAffect) it = m_list_pkAffect.begin();

	while (it != m_list_pkAffect.end())
	{
		CAffect * pkAff = *it++;

		if (IS_NO_SAVE_AFFECT(pkAff->dwType))
			continue;

		sys_log(1, "AFFECT_SAVE: %u %u %d %d", pkAff->dwType, pkAff->bApplyOn, pkAff->lApplyValue, pkAff->lDuration);

		p.dwPID			= GetPlayerID();
		p.elem.dwType		= pkAff->dwType;
		p.elem.bApplyOn		= pkAff->bApplyOn;
		p.elem.lApplyValue	= pkAff->lApplyValue;
		p.elem.dwFlag		= pkAff->dwFlag;
		p.elem.lDuration	= pkAff->lDuration;
		p.elem.lSPCost		= pkAff->lSPCost;
		p.elem.dwItemVnum = pkAff->dwItemVnum;
		db_clientdesc->DBPacket(HEADER_GD_ADD_AFFECT, 0, &p, sizeof(p));
	}
}

EVENTINFO(load_affect_login_event_info)
{
	DWORD pid;
	DWORD count;
	char* data;

	load_affect_login_event_info()
	: pid( 0 )
	, count( 0 )
	, data( 0 )
	{
	}
};

EVENTFUNC(load_affect_login_event)
{
	load_affect_login_event_info* info = dynamic_cast<load_affect_login_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "load_affect_login_event_info> <Factor> Null pointer" );
		return 0;
	}

	DWORD dwPID = info->pid;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwPID);

	if (!ch)
	{
		M2_DELETE_ARRAY(info->data);
		return 0;
	}

	LPDESC d = ch->GetDesc();

	if (!d)
	{
		M2_DELETE_ARRAY(info->data);
		return 0;
	}

	if (d->IsPhase(PHASE_HANDSHAKE) ||
			d->IsPhase(PHASE_LOGIN) ||
			d->IsPhase(PHASE_SELECT) ||
			d->IsPhase(PHASE_DEAD) ||
			d->IsPhase(PHASE_LOADING))
	{
		return PASSES_PER_SEC(1);
	}
	else if (d->IsPhase(PHASE_CLOSE))
	{
		M2_DELETE_ARRAY(info->data);
		return 0;
	}
	else if (d->IsPhase(PHASE_GAME))
	{
		sys_log(1, "Affect Load by Event");
		ch->LoadAffect(info->count, (TPacketAffectElement*)info->data);
		M2_DELETE_ARRAY(info->data);
		return 0;
	}
	else
	{
		sys_err("input_db.cpp:quest_login_event INVALID PHASE pid %d", ch->GetPlayerID());
		M2_DELETE_ARRAY(info->data);
		return 0;
	}
}

void CHARACTER::LoadAffect(DWORD dwCount, TPacketAffectElement * pElements)
{
	m_bIsLoadedAffect = false;

	if (!GetDesc()->IsPhase(PHASE_GAME))
	{
		if (test_server)
			sys_log(0, "LOAD_AFFECT: Creating Event", GetName(), dwCount);

		load_affect_login_event_info* info = AllocEventInfo<load_affect_login_event_info>();

		info->pid = GetPlayerID();
		info->count = dwCount;
		info->data = M2_NEW char[sizeof(TPacketAffectElement) * dwCount];
		thecore_memcpy(info->data, pElements, sizeof(TPacketAffectElement) * dwCount);

		event_create(load_affect_login_event, info, PASSES_PER_SEC(1));

		return;
	}

	ClearAffect(true);

	if (test_server)
		sys_log(0, "LOAD_AFFECT: %s count %d", GetName(), dwCount);

	TAffectFlag afOld = m_afAffectFlag;

	long lMovSpd = GetPoint(POINT_MOV_SPEED);
	long lAttSpd = GetPoint(POINT_ATT_SPEED);

	for (DWORD i = 0; i < dwCount; ++i, ++pElements)
	{
		if (pElements->dwType == SKILL_FLAME_SPIRIT || pElements->dwType == AFFECT_SPRINT)
			continue;

		if (AFFECT_AUTO_HP_RECOVERY == pElements->dwType || AFFECT_AUTO_SP_RECOVERY == pElements->dwType)
		{
			LPITEM item = FindItemByID( pElements->dwFlag );

			if (NULL == item)
				continue;

			item->Lock(true);
		}

		if (pElements->bApplyOn >= POINT_MAX_NUM)
		{
			sys_err("invalid affect data %s ApplyOn %u ApplyValue %d",
					GetName(), pElements->bApplyOn, pElements->lApplyValue);
			continue;
		}

		if (test_server)
		{
			sys_log(0, "Load Affect : Affect %s %d %d", GetName(), pElements->dwType, pElements->bApplyOn );
		}

		CAffect* pkAff = CAffect::Acquire();
		m_list_pkAffect.emplace_back(pkAff);

		pkAff->dwType		= pElements->dwType;
		pkAff->bApplyOn		= pElements->bApplyOn;
		pkAff->lApplyValue	= pElements->lApplyValue;
		pkAff->dwFlag		= pElements->dwFlag;
		pkAff->lDuration	= pElements->lDuration;
		pkAff->lSPCost		= pElements->lSPCost;
		pkAff->dwItemVnum = pElements->dwItemVnum;

		SendAffectAddPacket(GetDesc(), pkAff);

		ComputeAffect(pkAff, true);

	}

	if ( CArenaManager::instance().IsArenaMap(GetMapIndex()) == true )
	{
		RemoveGoodAffect();
	}

	if (afOld != m_afAffectFlag || lMovSpd != GetPoint(POINT_MOV_SPEED) || lAttSpd != GetPoint(POINT_ATT_SPEED))
	{
		UpdatePacket();
	}

	StartAffectEvent();

	m_bIsLoadedAffect = true;

	ComputePoints(); // @fixme156
	DragonSoul_Initialize();

	// mt2009 edit: WHY would we want to regain players HP on load affect? USE YOUR POTIONS!
	// @fixme118 BEGIN (regain affect hp/mp)
	//if (!IsDead())
	//{
	//	PointChange(POINT_HP, GetMaxHP() - GetHP());
	//	PointChange(POINT_SP, GetMaxSP() - GetSP());
	//}
	// @fixme118 END
}

bool CHARACTER::CanAddAffect(DWORD dwType, BYTE bApplyOn, long lApplyValue, DWORD dwFlag, long lDuration)
{
	if (dwType == AFFECT_HEAVEN_PROTECTION
		&& dwFlag == AFF_HEAVEN_PROTECTION
		&& (FindAffect(AFFECT_DEBUFF_HEAVEN_PROTECTION)
			|| CPVPManager::instance().IsFighting(GetPlayerID())
			|| CWarMapManager::instance().IsWarMap(GetMapIndex())
			))
		return false;

	if (IsStone() && (dwType == AFFECT_POISON || dwType == AFFECT_SLOW || dwType == AFFECT_FIRE || dwType == AFFECT_STUN))
		return false;

	if (IsUtilityPoint(bApplyOn))
		return false;

	return true;
}

void CHARACTER::OnAddAffect(DWORD dwType, BYTE& bApplyOn, long& lApplyValue, DWORD dwFlag, long lDuration)
{

	switch (dwType) {
		case AFFECT_REMOVE_ALL_RESIST:
			EffectPacket(SE_BLOW_POISON);
			break;

		case AFFECT_BLOCK_CHAT:
			if (lDuration > 1)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your chat has been blocked by a GM."));
			}
			break;
		case AFFECT_HEAVEN_PROTECTION:
			if (dwFlag == AFF_HEAVEN_PROTECTION)
			{
				AddAffect(AFFECT_DEBUFF_HEAVEN_PROTECTION, POINT_NONE, 0, AFF_NONE, 17, 0, true);

				if (GetPoint(POINT_ABSORB_DAMAGE_MONSTER) > 0)
					lApplyValue -= GetPoint(POINT_ABSORB_DAMAGE_MONSTER);
			}
			break;
		case AFFECT_SLOW:
		{
			if (IsSprint())
			{
				SetSprint(false);
			}

			auto movSpeed = GetLimitPoint(POINT_MOV_SPEED);
			auto minScaleSpeed = 100;
			auto maxScaleSpeed = 180;
			if (movSpeed > minScaleSpeed)
			{
				auto max_progress = maxScaleSpeed - minScaleSpeed;
				auto progress = MIN(movSpeed - minScaleSpeed, max_progress);

				auto progress_pct = progress * 100 / max_progress;
				auto slow_debuff_value = 30 * progress_pct / 100;
				lApplyValue -= slow_debuff_value;
			}
		}
			break;
		case AFFECT_HEAL_ON_DAMAGE:
			EffectPacket(SE_ENT_AURA);
			break;

		case SKILL_STEALTH:
		{
			EffectPacket(SE_STEALTH_0 + GetSkillMasterType(SKILL_STEALTH));
			break;
		}
		default:
			break;
	}

	if (bApplyOn == POINT_ABSORB_DAMAGE_MONSTER || bApplyOn == POINT_ABSORB_DAMAGE)
	{
		PointChange(bApplyOn, lApplyValue);
		lApplyValue = 0;
		bApplyOn = POINT_NONE;
	}

	if (dwFlag == AFF_QUEST_BOX)
	{
		if (IsSprint())
		{
			SetSprint(false);
		}
	}
}

bool CHARACTER::AddAffect(DWORD dwType, BYTE bApplyOn, long lApplyValue, DWORD dwFlag, long lDuration, long lSPCost, bool bOverride, bool IsCube, DWORD itemVnum)
{
	if (!CanAddAffect(dwType, bApplyOn, lApplyValue, dwFlag, lDuration))
		return false;

	OnAddAffect(dwType, bApplyOn, lApplyValue, dwFlag, lDuration);

	if (lDuration == 0)
	{
		sys_err("Character::AddAffect lDuration == 0 type %d", lDuration, dwType);
		lDuration = 1;
	}

	CAffect * pkAff = NULL;

	if (IsCube)
		pkAff = FindAffect(dwType,bApplyOn);
	else
		pkAff = FindAffect(dwType);

	if (dwFlag == AFF_STUN || dwFlag == AFF_ROOT)
	{
		if (m_posDest.x != GetX() || m_posDest.y != GetY())
		{
			if (HasPlayerData())
				playerData->last_checked_position = GetXYZ();
			m_posDest.x = m_posStart.x = GetX();
			m_posDest.y = m_posStart.y = GetY();
			battle_end(this);

			SyncPacket();
		}
	}

	if (pkAff && bOverride)
	{
		ComputeAffect(pkAff, false, true);

		if (GetDesc())
			SendAffectRemovePacket(GetDesc(), GetPlayerID(), pkAff->dwType, pkAff->bApplyOn);
	}
	else
	{
		pkAff = CAffect::Acquire();
		m_list_pkAffect.emplace_back(pkAff);

	}

	sys_log(1, "AddAffect %s type %d apply %d %d flag %u duration %d", GetName(), dwType, bApplyOn, lApplyValue, dwFlag, lDuration);
	sys_log(0, "AddAffect %s type %d apply %d %d flag %u duration %d", GetName(), dwType, bApplyOn, lApplyValue, dwFlag, lDuration);

	pkAff->dwType	= dwType;
	pkAff->bApplyOn	= bApplyOn;
	pkAff->lApplyValue	= lApplyValue;
	pkAff->dwFlag	= dwFlag;
	pkAff->lDuration	= lDuration;
	pkAff->lSPCost	= lSPCost;
	pkAff->dwItemVnum = itemVnum;

	WORD wMovSpd = GetPoint(POINT_MOV_SPEED);
	WORD wAttSpd = GetPoint(POINT_ATT_SPEED);

	ComputeAffect(pkAff, true);

	if (pkAff->dwFlag || wMovSpd != GetPoint(POINT_MOV_SPEED) || wAttSpd != GetPoint(POINT_ATT_SPEED))
		UpdatePacket();

	StartAffectEvent();

	if (IsPC())
	{
		SendAffectAddPacket(GetDesc(), pkAff);

		if (IS_NO_SAVE_AFFECT(pkAff->dwType))
			return true;

		TPacketGDAddAffect p;
		p.dwPID			= GetPlayerID();
		p.elem.dwType		= pkAff->dwType;
		p.elem.bApplyOn		= pkAff->bApplyOn;
		p.elem.lApplyValue	= pkAff->lApplyValue;
		p.elem.dwFlag		= pkAff->dwFlag;
		p.elem.lDuration	= pkAff->lDuration;
		p.elem.lSPCost		= pkAff->lSPCost;
		p.elem.dwItemVnum = pkAff->dwItemVnum;
		db_clientdesc->DBPacket(HEADER_GD_ADD_AFFECT, 0, &p, sizeof(p));
	}

	if (IS_TARGET_SHOW_AFFECT(dwType))
	{
		BroadcastTargetPacket(true);
	}

	return true;
}

void CHARACTER::RefreshAffect()
{
	itertype(m_list_pkAffect) it = m_list_pkAffect.begin();

	while (it != m_list_pkAffect.end())
	{
		CAffect * pkAff = *it++;
		ComputeAffect(pkAff, true);
	}
}

void CHARACTER::ComputeAffect(CAffect * pkAff, bool bAdd, bool bOverride)
{
	if (bAdd && pkAff->dwType >= GUILD_SKILL_START && pkAff->dwType <= GUILD_SKILL_END)
	{
		if (!GetGuild())
			return;

		if (!GetGuild()->UnderAnyWar())
			return;
	}

	if (pkAff->dwFlag)
	{
		if (!bAdd)
			m_afAffectFlag.Reset(pkAff->dwFlag);
		else
			m_afAffectFlag.Set(pkAff->dwFlag);
	}

	if (bAdd)
		PointChange(pkAff->bApplyOn, pkAff->lApplyValue);
	else
		PointChange(pkAff->bApplyOn, -pkAff->lApplyValue);

	if (pkAff->dwType == SKILL_FLAME_SPIRIT)
	{
		if (bAdd)
			StartMuyeongEvent();
		else
			StopMuyeongEvent();
	}
	else if (!bAdd && !bOverride && pkAff->dwType == AFFECT_HEAVEN_PROTECTION)
	{
		PointChange(POINT_ABSORB_DAMAGE_MONSTER, -GetPoint(POINT_ABSORB_DAMAGE_MONSTER));
	}
}

bool CHARACTER::RemoveAffect(CAffect * pkAff)
{
	if (!pkAff)
		return false;

	// AFFECT_BUF_FIX
	m_list_pkAffect.remove(pkAff);
	// END_OF_AFFECT_BUF_FIX

	ComputeAffect(pkAff, false);

	if (AFFECT_REVIVE_INVISIBLE != pkAff->dwType && IsPC()) // https://metin2.dev/topic/33119-computing-monster-points/
		ComputePoints();
	else  // @fixme110
		UpdatePacket();

	if (IS_TARGET_SHOW_AFFECT(pkAff->dwType))
	{
		BroadcastTargetPacket(true);
	}

	CheckMaximumPoints();

	if (test_server)
		sys_log(0, "AFFECT_REMOVE: %s (flag %u apply: %u)", GetName(), pkAff->dwFlag, pkAff->bApplyOn);

	if (IsPC())
	{
		SendAffectRemovePacket(GetDesc(), GetPlayerID(), pkAff->dwType, pkAff->bApplyOn);
	}

	CAffect::Release(pkAff);
	return true;
}

bool CHARACTER::RemoveAffect(DWORD dwType)
{
	// CHAT_BLOCK
	if (dwType == AFFECT_BLOCK_CHAT)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your chat block has been lifted."));
	}
	// END_OF_CHAT_BLOCK

	bool flag = false;

	CAffect * pkAff;

	while ((pkAff = FindAffect(dwType)))
	{
		RemoveAffect(pkAff);
		flag = true;
	}

	return flag;
}

bool CHARACTER::IsAffectFlag(DWORD dwAff) const
{
	return m_afAffectFlag.IsSet(dwAff);
}

void CHARACTER::RemoveSkillAffect()
{
	RemoveAffect(AFFECT_SPRINT);

	RemoveAffect(SKILL_BERSERK);
	RemoveAffect(SKILL_SWORD_AURA);
	RemoveAffect(SKILL_STRONG_BODY);
	RemoveAffect(SKILL_STEALTH);
	RemoveAffect(SKILL_FEATHER_WALK);
	RemoveAffect(SKILL_ENCHANTED_BLADE);
	RemoveAffect(SKILL_TERROR);
	RemoveAffect(SKILL_ENCHANTED_ARMOR);
	RemoveAffect(SKILL_MANASHIELD);
	RemoveAffect(SKILL_BLESSING);
	RemoveAffect(SKILL_REFLECT);
	RemoveAffect(SKILL_SWIFTNESS);
	RemoveAffect(SKILL_ATTACK_UP);
	RemoveAffect(SKILL_DRAGON_HELP);
#ifdef ENABLE_WOLFMAN_CHARACTER
	RemoveAffect(SKILL_JEOKRANG);
	RemoveAffect(SKILL_CHEONGRANG);
#endif
}

void CHARACTER::RemoveGoodAffect()
{
	RemoveAffect(AFFECT_MOV_SPEED);
	RemoveAffect(AFFECT_ATT_SPEED);
	RemoveAffect(AFFECT_STR);
	RemoveAffect(AFFECT_DEX);
	RemoveAffect(AFFECT_INT);
	RemoveAffect(AFFECT_CON);
	RemoveAffect(AFFECT_CHINA_FIREWORK);
	RemoveSkillAffect();
}

bool CHARACTER::IsGoodAffect(BYTE bAffectType) const
{
	switch (bAffectType)
	{
		case (AFFECT_MOV_SPEED):
		case (AFFECT_ATT_SPEED):
		case (AFFECT_STR):
		case (AFFECT_DEX):
		case (AFFECT_INT):
		case (AFFECT_CON):
		case (AFFECT_CHINA_FIREWORK):
		case (AFFECT_SPRINT):

		case (SKILL_BERSERK):
		case (SKILL_SWORD_AURA):
		case (SKILL_STRONG_BODY):
		case (SKILL_STEALTH):
		case (SKILL_FEATHER_WALK):
		case (SKILL_ENCHANTED_BLADE):
		case (SKILL_TERROR):
		case (SKILL_ENCHANTED_ARMOR):
		case (SKILL_MANASHIELD):
		case (SKILL_BLESSING):
		case (SKILL_REFLECT):
		case (SKILL_SWIFTNESS):
		case (SKILL_ATTACK_UP):
		case (SKILL_DRAGON_HELP):
#ifdef ENABLE_WOLFMAN_CHARACTER

		case (SKILL_JEOKRANG):
		case (SKILL_CHEONGRANG):
#endif
			return true;
	}
	return false;
}

void CHARACTER::RemoveBadAffect()
{
	sys_log(0, "RemoveBadAffect %s", GetName());
	RemovePoison();
#ifdef ENABLE_WOLFMAN_CHARACTER
	RemoveBleeding();
#endif
	RemoveFire();

	RemoveAffect(AFFECT_STUN);

	RemoveAffect(AFFECT_SLOW);

	RemoveAffect(SKILL_SPIRIT_ORB);

	//RemoveAffect(SKILL_CURSE);

	//RemoveAffect(SKILL_PABUP);

	//RemoveAffect(AFFECT_FAINT);

	//RemoveAffect(AFFECT_WEB);

	//RemoveAffect(AFFECT_SLEEP);

	//RemoveAffect(AFFECT_CURSE);

	//RemoveAffect(AFFECT_PARALYZE);

	//RemoveAffect(SKILL_BUDONG);
}
void CHARACTER::RemoveGuildWarAffect()
{
	RemoveAffect(GUILD_SKILL_BLOOD);
	RemoveAffect(GUILD_SKILL_BLESS);
	RemoveAffect(GUILD_SKILL_SEONGHWI);
	RemoveAffect(GUILD_SKILL_ACCEL);
	RemoveAffect(GUILD_SKILL_BUNNO);
	RemoveAffect(GUILD_SKILL_JUMUN);
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
