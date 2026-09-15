#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "desc.h"
#include "char.h"
#include "char_manager.h"
#include "battle.h"
#include "item.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "vector.h"
#include "packet.h"
#include "pvp.h"
#include "profiler.h"
#include "guild.h"
#include "affect.h"
#include "unique_item.h"
#include "lua_incl.h"
#include "arena.h"
#include "castle.h"
#include "sectree.h"
#include "ani.h"
#include "locale_service.h"
#include "../../common/CommonDefines.h"

int battle_hit(LPCHARACTER ch, LPCHARACTER victim, int & iRetDam);

bool is_distance_skill(BYTE skillId)
{
	if (skillId < 1)
		return false;

	switch (skillId)
	{
	case SKILL_SWORD_STRIKE:
	case SKILL_POISON_CLOUD:
	case SKILL_ROLLING_DAGGER:
	case SKILL_REPETITIVE_SHOT:
	case SKILL_ARROW_SHOWER:
	case SKILL_FIRE_ARROW:
	case SKILL_POISON_ARROW:
	case SKILL_DISPEL:
	case SKILL_DARK_STRIKE:
	case SKILL_SPIRIT_ORB:
	case SKILL_DARK_ORB:
	case SKILL_FLYING_TALISMAN:
	case SKILL_SHOOTING_DRAGON:
	case SKILL_LIGHTNING_THROW:
	case SKILL_LIGHTNING_CLAW:
	case SKILL_HORSE_WILDATTACK_RANGE:
		return true;
	}
	return false;
}

bool battle_distance_valid_by_xy(long x, long y, long tx, long ty)
{
	long distance = DISTANCE_APPROX(x - tx, y - ty);

	if (distance > 170)
		return false;

	return true;
}

bool battle_distance_valid(LPCHARACTER ch, LPCHARACTER victim)
{
	return battle_distance_valid_by_xy(ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY());
}

bool timed_event_cancel(LPCHARACTER ch)
{
	if (ch->m_pkTimedEvent)
	{
		event_cancel(&ch->m_pkTimedEvent);
		return true;
	}

	return false;
}

#ifdef NEW_ICEDAMAGE_SYSTEM
bool battle_is_icedamage(LPCHARACTER pAttacker, LPCHARACTER pVictim)
{
	if (pAttacker && pAttacker->IsPC())
	{
		DWORD race = pAttacker->GetRaceNum();
		const DWORD tmp_dwNDRFlag = pVictim->GetNoDamageRaceFlag();
		if (tmp_dwNDRFlag &&
			(race < MAIN_RACE_MAX_NUM) &&
			(IS_SET(tmp_dwNDRFlag, 1<<race))
		)
		{
			return false;
		}
		const std::set<DWORD> & tmp_setNDAFlag = pVictim->GetNoDamageAffectFlag();
		if (tmp_setNDAFlag.size())
		{
			for (std::set<DWORD>::iterator it = tmp_setNDAFlag.begin(); it != tmp_setNDAFlag.end(); ++it)
			{
				if (!pAttacker->IsAffectFlag(*it))
				{
					return false;
				}
			}
		}
	}
	return true;
}
#endif

bool battle_is_attackable(LPCHARACTER ch, LPCHARACTER victim)
{
	if (victim->IsDead())
		return false;

	if (victim->IsAffectFlag(AFF_REVIVE_INVISIBLE))
		return false;

	if (ch->IsObserverMode() || victim->IsObserverMode())
		return false;

	if (victim->IsNPC() && victim->m_pkMobInst)
	{
		const auto delta = get_dword_time() - victim->m_pkMobInst->m_dwLastLostAggroOnDistanceTime;
		if (delta < 5000)
			return false;
	}

	if (!ch->IsGuardNPC())
	{
		SECTREE	*sectree = NULL;

		sectree	= ch->GetSectree();
		if (sectree && sectree->IsAttr(ch->GetX(), ch->GetY(), ATTR_BANPK))
			return false;

		sectree = victim->GetSectree();
		if (sectree && sectree->IsAttr(victim->GetX(), victim->GetY(), ATTR_BANPK))
			return false;
	}
#ifdef NEW_ICEDAMAGE_SYSTEM
	if (!battle_is_icedamage(ch, victim))
		return false;
#endif
	if (ch->IsStun() || ch->IsDead())
		return false;

	if (ch->IsPC() && victim->IsPC())
	{
		CGuild* g1 = ch->GetGuild();
		CGuild* g2 = victim->GetGuild();

		if (g1 && g2)
		{
			if (g1->UnderWar(g2->GetID()))
				return true;
		}
	}

	if (IS_CASTLE_MAP(ch->GetMapIndex()) && false==castle_can_attack(ch, victim))
			return false;

	if (CArenaManager::instance().CanAttack(ch, victim) == true)
		return true;

	return CPVPManager::instance().CanAttack(ch, victim);
}

int battle_melee_attack(LPCHARACTER ch, LPCHARACTER victim)
{
	if (test_server&&ch->IsPC())
		sys_log(0, "battle_melee_attack : [%s] attack to [%s]", ch->GetName(), victim->GetName());

	if (!victim || ch == victim)
		return BATTLE_NONE;

	if (test_server&&ch->IsPC())
		sys_log(0, "battle_melee_attack : [%s] attack to [%s]", ch->GetName(), victim->GetName());

	if (!battle_is_attackable(ch, victim))
		return BATTLE_NONE;

	if (test_server&&ch->IsPC())
		sys_log(0, "battle_melee_attack : [%s] attack to [%s]", ch->GetName(), victim->GetName());

	int distance = DISTANCE_APPROX(ch->GetX() - victim->GetX(), ch->GetY() - victim->GetY());

	if (!victim->IsBuilding())
	{
		int max = 300;

		if (false == ch->IsPC())
		{
			max = (int) (ch->GetMobAttackRange() * 1.15f);
		}
		else
		{
			max = 405;

			if (false == victim->IsPC() && BATTLE_TYPE_MELEE == victim->GetMobBattleType())
			{
				max = MAX(405, (int)(victim->GetMobAttackRange() * 1.15f));
			}
		}

		if (distance > max)
		{
			if (test_server)
				sys_log(0, "VICTIM_FAR: %s distance: %d max: %d", ch->GetName(), distance, max);

			return BATTLE_NONE;
		}
	}

	if (timed_event_cancel(ch))
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Action cancelled. You have entered a battle."));

	if (timed_event_cancel(victim))
		victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Action cancelled. You have entered a battle."));

	ch->SetPosition(POS_FIGHTING);
	ch->SetVictim(victim);

	const PIXEL_POSITION & vpos = victim->GetXYZ();
	ch->SetRotationToXY(vpos.x, vpos.y);

	int dam;
	int ret = battle_hit(ch, victim, dam);
	return (ret);
}

int battle_range_attack(LPCHARACTER ch, LPCHARACTER victim)
{
	if (!ch || !victim)
		return BATTLE_NONE;

	if (timed_event_cancel(ch))
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Action cancelled. You have entered a battle."));

	if (timed_event_cancel(victim))
		victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Action cancelled. You have entered a battle."));

	return 1;
}

void battle_end_ex(LPCHARACTER ch)
{
	if (ch->IsPosition(POS_FIGHTING))
		ch->SetPosition(POS_STANDING);
}

void battle_end(LPCHARACTER ch)
{
	battle_end_ex(ch);
}

// AG = Attack Grade
// AL = Attack Limit
int CalcBattleDamage(int iDam, int iAttackerLev, int iVictimLev)
{
	if (iDam < 3)
		iDam = number(1, 5);

	//return CALCULATE_DAMAGE_LVDELTA(iAttackerLev, iVictimLev, iDam);
	return iDam;
}

int CalcMagicDamageWithValue(int iDam, LPCHARACTER pkAttacker, LPCHARACTER pkVictim)
{
	return CalcBattleDamage(iDam, pkAttacker->GetLevel(), pkVictim->GetLevel());
}

int CalcMagicDamage(LPCHARACTER pkAttacker, LPCHARACTER pkVictim)
{
	int iDam = 0;

	if (pkAttacker->IsNPC())
	{
		iDam = CalcMeleeDamage(pkAttacker, pkVictim, pkAttacker->PenetrateHit(pkVictim), false);
	}

	iDam += pkAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS);

	return CalcMagicDamageWithValue(iDam, pkAttacker, pkVictim);
}

float CalcAttackRating(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, bool bIgnoreTargetRating)
{
	int iARSrc;
	int iERSrc;

	{
		int attacker_dx = pkAttacker->GetPolymorphPoint(POINT_DX);
		int attacker_lv = pkAttacker->GetLevel();

		int victim_dx = pkVictim->GetPolymorphPoint(POINT_DX);
		int victim_lv = pkVictim->GetLevel();

		iARSrc = MIN(90, (attacker_dx * 4	+ attacker_lv * 2) / 6);
		iERSrc = MIN(90, (victim_dx	  * 4	+ victim_lv   * 2) / 6);
	}

	float fAR = ((float) iARSrc + 210.0f) / 300.0f;
	if (bIgnoreTargetRating)
		return fAR;

	float fER = ((float) (iERSrc * 2 + 5) / (iERSrc + 95)) * 3.0f / 10.0f;
	return fAR - fER;
}

int SumAttResBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int attBonusID, int resistBonusID)
{
	return pkAttacker->GetPoint(attBonusID) - pkVictim->GetPoint(resistBonusID);
}

int SumBreakBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int attBonusID, int resistBonusID)
{
	if (!pkVictim->IsPC())
		return 100 - pkVictim->GetPoint(resistBonusID);

	int attBonus = attBonusID == POINT_NONE ? 0 : pkAttacker->GetPoint(attBonusID);
	int resistBonus = resistBonusID == POINT_NONE ? 0 : pkVictim->GetResistPoint(resistBonusID);

	return 100 + attBonus - resistBonus;
}

int CalcPlayerBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int iDam)
{
	if (!pkAttacker || !pkVictim)
		return iDam;

	if (pkVictim->IsPC())
	{
		int point_value = pkAttacker->GetPoint(POINT_ATTBONUS_HUMAN);
		if (pkAttacker->GetMapIndex() == 64) // threeway
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_ORC_VALLEY);

		switch (pkVictim->GetJob())
		{
		case JOB_WARRIOR:
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_WARRIOR);
			break;

		case JOB_ASSASSIN:
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_ASSASSIN);
			break;

		case JOB_SURA:
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_SURA);
			break;

		case JOB_SHAMAN:
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_SHAMAN);
			break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
			iDam += (iDam * pkAttacker->GetPoint(POINT_ATTBONUS_WOLFMAN)) / 100;
			break;
#endif
		}

		iDam += (iDam * point_value) / 100;
	}

	if (pkAttacker->IsPC())
	{
		int resist_value = pkVictim->GetResistPoint(POINT_RESIST_HUMAN);

		switch (pkAttacker->GetJob())
		{
		case JOB_WARRIOR:
			resist_value += pkVictim->GetResistPoint(POINT_RESIST_WARRIOR);
			break;

		case JOB_ASSASSIN:
			resist_value += pkVictim->GetResistPoint(POINT_RESIST_ASSASSIN);
			break;

		case JOB_SURA:
			resist_value += pkVictim->GetResistPoint(POINT_RESIST_SURA);
			break;

		case JOB_SHAMAN:
			resist_value += pkVictim->GetResistPoint(POINT_RESIST_SHAMAN);
			break;

#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
			iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_WOLFMAN)) / 100;
			break;
#endif
		}

		iDam -= (iDam * resist_value / 100);
	}

	return iDam;
}

int CalcDamBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int iDam)
{
	LPITEM pkWeapon = pkAttacker->GetWear(WEAR_WEAPON);
	if (pkWeapon)
	{
		switch (pkWeapon->GetSubType())
		{
			case WEAPON_SWORD:
				iDam = iDam * (100 - pkVictim->GetResistPoint(POINT_RESIST_SWORD, pkAttacker)) / 100;
				break;

			case WEAPON_TWO_HANDED:
				iDam = iDam * (100 - pkVictim->GetResistPoint(POINT_RESIST_TWOHAND, pkAttacker)) / 100;
				break;

			case WEAPON_DAGGER:
				iDam = iDam * (100 - pkVictim->GetResistPoint(POINT_RESIST_DAGGER, pkAttacker)) / 100;
				break;

			case WEAPON_BELL:
				iDam = iDam * (100 - pkVictim->GetResistPoint(POINT_RESIST_BELL, pkAttacker)) / 100;
				break;

			case WEAPON_FAN:
				iDam = iDam * (100 - pkVictim->GetResistPoint(POINT_RESIST_FAN, pkAttacker)) / 100;
				break;

			case WEAPON_BOW:
				iDam = iDam * (100 - pkVictim->GetResistPoint(POINT_RESIST_BOW, pkAttacker)) / 100;
				break;

			#ifdef ENABLE_WOLFMAN_CHARACTER
			case WEAPON_CLAW:
				iDam = iDam * SumBreakBonus(pkAttacker, pkVictim, POINT_ATTBONUS_CLAW, POINT_RESIST_CLAW) / 100;
				#if defined(ENABLE_WOLFMAN_CHARACTER) && defined(USE_ITEM_CLAW_AS_DAGGER)
				iDam = iDam * SumBreakBonus(pkAttacker, pkVictim, POINT_ATTBONUS_DAGGER, POINT_RESIST_DAGGER) / 100;
				#endif
				break;
			#endif
		}
	}
	return iDam;
}

int CalcAttBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int iAtk)
{
	if (pkAttacker->IsNPC() && pkVictim->IsPC())
	{
		iAtk = (iAtk * CHARACTER_MANAGER::instance().GetMobDamageRate(pkAttacker)) / 100;

		iAtk *= 115 / 100; // zwiekszamy ogolny dmg mobow o 15%
	}

	if (pkVictim->IsNPC())
	{
		int point_value = pkAttacker->GetPoint(POINT_ATTBONUS_MONSTER);

		if (pkAttacker->GetMapIndex() == 64) // threeway
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_ORC_VALLEY);

		if (pkVictim->GetMobRank() >= MOB_RANK_BOSS)
		{
			if (pkVictim->IsStone())
			{
				point_value += pkAttacker->GetPoint(POINT_ATTBONUS_STONE);
			}
			else
			{
				point_value += pkAttacker->GetPoint(POINT_ATTBONUS_BOSS);
			}
		}

		if (pkVictim->IsRaceFlag(RACE_FLAG_ANIMAL))
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_ANIMAL);
		if (pkVictim->IsRaceFlag(RACE_FLAG_UNDEAD))
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_UNDEAD);
		if (pkVictim->IsRaceFlag(RACE_FLAG_DEVIL))
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_DEVIL);
		if (pkVictim->IsRaceFlag(RACE_FLAG_HUMAN))
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_HUMAN);
		if (pkVictim->IsRaceFlag(RACE_FLAG_ORC))
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_ORC);
		if (pkVictim->IsRaceFlag(RACE_FLAG_MILGYO))
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_MILGYO);
		if (pkVictim->IsRaceFlag(RACE_FLAG_INSECT))
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_INSECT);
		if (pkVictim->IsRaceFlag(RACE_FLAG_FIRE))
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_FIRE);
		if (pkVictim->IsRaceFlag(RACE_FLAG_ICE))
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_ICE);
		if (pkVictim->IsRaceFlag(RACE_FLAG_DESERT))
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_DESERT);
		if (pkVictim->IsRaceFlag(RACE_FLAG_TREE))
			point_value += pkAttacker->GetPoint(POINT_ATTBONUS_TREE);

		iAtk += (iAtk * point_value) / 100;
	}

	iAtk = CalcPlayerBonus(pkAttacker, pkVictim, iAtk);

	if (pkAttacker->IsNPC() && pkVictim->IsPC())
	{
		iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_MONSTER_1000PCT) / 1000);

		//if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_ELEC))
		//	iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_ELEC))		/ 10000;
		//else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_FIRE))
		//	iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_FIRE))		/ 10000;
		//else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_ICE))
		//	iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_ICE))		/ 10000;
		//else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_WIND))
		//	iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_WIND))		/ 10000;
		//else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_EARTH))
		//	iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_EARTH))	/ 10000;
		//else if (pkAttacker->IsRaceFlag(RACE_FLAG_ATT_DARK))
		//	iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_DARK))		/ 10000;
	}

	iAtk += (iAtk * pkVictim->GetPoint(POINT_CURSE_PCT) / 100); // curse increases damage that a victim gets

	//if (pkAttacker->IsPC() && pkVictim->IsNPC())
	//{
	//	if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_ELEC))
	//		iAtk += (iAtk * 30 * pkAttacker->GetPoint(POINT_ENCHANT_ELECT))	/ 10000;
	//	else if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_FIRE))
	//		iAtk += (iAtk * 30 * pkAttacker->GetPoint(POINT_ENCHANT_FIRE))	/ 10000;
	//	else if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_ICE))
	//		iAtk += (iAtk * 30 * pkAttacker->GetPoint(POINT_ENCHANT_ICE))	/ 10000;
	//	else if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_WIND))
	//		iAtk += (iAtk * 30 * pkAttacker->GetPoint(POINT_ENCHANT_WIND))	/ 10000;
	//	else if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_EARTH))
	//		iAtk += (iAtk * 30 * pkAttacker->GetPoint(POINT_ENCHANT_EARTH))	/ 10000;
	//	else if (pkVictim->IsRaceFlag(RACE_FLAG_ATT_DARK))
	//		iAtk += (iAtk * 30 * pkAttacker->GetPoint(POINT_ENCHANT_DARK))	/ 10000;
	//}

	return iAtk;
}

void Item_GetDamage(LPITEM pkItem, int* pdamMin, int* pdamMax)
{
	*pdamMin = 0;
	*pdamMax = 1;

	if (!pkItem)
		return;

	switch (pkItem->GetType())
	{
		case ITEM_ROD:
		case ITEM_PICK:
		case ITEM_HERB_KNIFE:
			return;
	}

	if (pkItem->GetType() != ITEM_WEAPON)
		sys_err("Item_GetDamage - !ITEM_WEAPON vnum=%d, type=%d", pkItem->GetOriginalVnum(), pkItem->GetType());

	*pdamMin = pkItem->GetValue(3);
	*pdamMax = pkItem->GetValue(4);
}

int CalcMeleeDamage(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, bool bIgnoreDefense, bool bIgnoreTargetRating)
{
	LPITEM pWeapon = pkAttacker->GetWear(WEAR_WEAPON);
	bool bPolymorphed = pkAttacker->IsPolymorphed();

	if (pWeapon && !(bPolymorphed && !pkAttacker->IsPolyMaintainStat()))
	{
		if (pWeapon->GetType() != ITEM_WEAPON)
			return 0;

		switch (pWeapon->GetSubType())
		{
			case WEAPON_SWORD:
			case WEAPON_DAGGER:
			case WEAPON_TWO_HANDED:
			case WEAPON_BELL:
			case WEAPON_FAN:
			case WEAPON_MOUNT_SPEAR:
#ifdef ENABLE_WOLFMAN_CHARACTER
			case WEAPON_CLAW:
#endif
				break;

			case WEAPON_BOW:
				sys_err("CalcMeleeDamage should not handle bows (name: %s)", pkAttacker->GetName());
				return 0;

			default:
				return 0;
		}
	}

	int iDam = 0;
	float fAR = CalcAttackRating(pkAttacker, pkVictim, bIgnoreTargetRating);
	int iDamMin = 0, iDamMax = 0;

	// TESTSERVER_SHOW_ATTACKINFO
	int DEBUG_iDamCur = 0;
	int DEBUG_iDamBonus = 0;
	// END_OF_TESTSERVER_SHOW_ATTACKINFO

	if (bPolymorphed && !pkAttacker->IsPolyMaintainStat())
	{
		// MONKEY_ROD_ATTACK_BUG_FIX
		Item_GetDamage(pWeapon, &iDamMin, &iDamMax);
		// END_OF_MONKEY_ROD_ATTACK_BUG_FIX

		DWORD dwMobVnum = pkAttacker->GetPolymorphVnum();
		const CMob * pMob = CMobManager::instance().Get(dwMobVnum);

		if (pMob)
		{
			int iPower = pkAttacker->GetPolymorphPower();
			iDamMin += pMob->m_table.dwDamageRange[0] * iPower / 100;
			iDamMax += pMob->m_table.dwDamageRange[1] * iPower / 100;
		}
	}
	else if (pWeapon)
	{
		// MONKEY_ROD_ATTACK_BUG_FIX
		Item_GetDamage(pWeapon, &iDamMin, &iDamMax);
		// END_OF_MONKEY_ROD_ATTACK_BUG_FIX
	}
	else if (pkAttacker->IsNPC())
	{
		iDamMin = pkAttacker->GetMobDamageMin();
		iDamMax = pkAttacker->GetMobDamageMax();
	}

	iDam = number(iDamMin, iDamMax) * 2;

	// TESTSERVER_SHOW_ATTACKINFO
	DEBUG_iDamCur = iDam;
	// END_OF_TESTSERVER_SHOW_ATTACKINFO

	int iAtk = 0;

	// level must be ignored when multiply by fAR, so subtract it before calculation.
	iAtk = pkAttacker->GetPoint(POINT_ATT_GRADE) + iDam - (pkAttacker->GetLevel() * 2);
	iAtk = (int) (iAtk * fAR);
	iAtk += pkAttacker->GetLevel() * 2; // and add again

	if (pWeapon)
	{
		iAtk += pWeapon->GetValue(5) * 2;

		if (pkVictim->IsNPC())
		{
			iAtk += pkAttacker->GetPoint(POINT_ATT_GRADE_MONSTER);

			if (pWeapon->GetSubType() == WEAPON_DAGGER)
				iAtk += pkAttacker->GetPoint(POINT_DAGGER_ATT_GRADE_MONSTER);
		}

		// 2004.11.12.myevan.TESTSERVER_SHOW_ATTACKINFO
		DEBUG_iDamBonus = pWeapon->GetValue(5) * 2;
		///////////////////////////////////////////////
	}

	iAtk += pkAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS); // party attacker role bonus
	iAtk += pkAttacker->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_ATTACK_BONUS); // marriage attack bonus
	iAtk = (int) (iAtk * (100 + (pkAttacker->GetPoint(POINT_ATT_BONUS) + pkAttacker->GetPoint(POINT_MELEE_MAGIC_ATT_BONUS_PER))) / 100);

	int iDef = 0;
	if (!bIgnoreDefense)
	{
		iDef = pkVictim->GetDefence();
		iDef += pkVictim->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_DEFENSE_BONUS);
	}

	if (pkAttacker->IsNPC())
		iAtk = (int) (iAtk * pkAttacker->GetMobDamageMultiply());

	iAtk = MAX(1, CalcAttBonus(pkAttacker, pkVictim, iAtk));
	iDam = MAX(1, iAtk - iDef);

	if (test_server)
	{
		int DEBUG_iLV = pkAttacker->GetLevel()*2;
		int DEBUG_iST = int((pkAttacker->GetPoint(POINT_ATT_GRADE) - DEBUG_iLV) * fAR);
		int DEBUG_iPT = pkAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS);
		int DEBUG_iWP = 0;
		int DEBUG_iPureAtk = 0;
		int DEBUG_iPureDam = 0;
		char szRB[32] = "";
		char szGradeAtkBonus[32] = "";

		DEBUG_iWP = int(DEBUG_iDamCur * fAR);
		DEBUG_iPureAtk = DEBUG_iLV + DEBUG_iST + DEBUG_iWP+DEBUG_iDamBonus;
		DEBUG_iPureDam = iAtk - iDef;

		if (pkAttacker->IsNPC())
		{
			snprintf(szGradeAtkBonus, sizeof(szGradeAtkBonus), "=%d*%.1f", DEBUG_iPureAtk, pkAttacker->GetMobDamageMultiply());
			DEBUG_iPureAtk = int(DEBUG_iPureAtk * pkAttacker->GetMobDamageMultiply());
		}

		if (DEBUG_iDamBonus != 0)
			snprintf(szRB, sizeof(szRB), "+RB(%d)", DEBUG_iDamBonus);

		char szPT[32] = "";

		if (DEBUG_iPT != 0)
			snprintf(szPT, sizeof(szPT), ", PT=%d", DEBUG_iPT);

		char szUnknownAtk[32] = "";

		if (iAtk != DEBUG_iPureAtk)
			snprintf(szUnknownAtk, sizeof(szUnknownAtk), "+?(%d)", iAtk-DEBUG_iPureAtk);

		char szUnknownDam[32] = "";

		if (iDam != DEBUG_iPureDam)
			snprintf(szUnknownDam, sizeof(szUnknownDam), "+?(%d)", iDam-DEBUG_iPureDam);

		char szMeleeAttack[128];

		snprintf(szMeleeAttack, sizeof(szMeleeAttack),
				"%s(%d)-%s(%d)=%d%s, ATK=LV(%d)+ST(%d)+WP(%d)%s%s%s, AR=%.3g%s",
				pkAttacker->GetName(),
				iAtk,
				pkVictim->GetName(),
				iDef,
				iDam,
				szUnknownDam,
				DEBUG_iLV,
				DEBUG_iST,
				DEBUG_iWP,
				szRB,
				szUnknownAtk,
				szGradeAtkBonus,
				fAR,
				szPT);

		pkAttacker->ChatPacket(CHAT_TYPE_TALKING, "%s", szMeleeAttack);
		pkVictim->ChatPacket(CHAT_TYPE_TALKING, "%s", szMeleeAttack);
	}

	return CalcBattleDamage(iDam, pkAttacker->GetLevel(), pkVictim->GetLevel());
}

int CalcArrowDamage(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, LPITEM pkBow, LPITEM pkArrow, bool bIgnoreDefense, DWORD dwSkill, int victimCount)
{
	if (!pkBow || pkBow->GetType() != ITEM_WEAPON || pkBow->GetSubType() != WEAPON_BOW)
		return 0;

	if (!pkArrow)
		return 0;

	if (!pkVictim)
		return 0;

	if (!pkAttacker)
		return 0;

	int iDist = (int)(DISTANCE_SQRT(pkAttacker->GetX() - pkVictim->GetX(), pkAttacker->GetY() - pkVictim->GetY()));
	int minDistance = pkArrow->GetValue(4) + pkAttacker->GetPoint(POINT_BOW_DISTANCE) * 100;
	int maxDistance = pkArrow->GetValue(5) + pkAttacker->GetPoint(POINT_BOW_DISTANCE) * 100;

	int iPercent = 100;
	if (iDist > minDistance)
	{
		int arrow_damage_percent = pkArrow->GetValue(2);
		int minDamagePercent = pkVictim->IsNPC() ? MIN(arrow_damage_percent * 2, 100) : arrow_damage_percent;
		if (iDist >= maxDistance)
			iPercent = minDamagePercent;
		else
		{
			int distDelta = maxDistance - minDistance;
			int distProgress = ((iDist - minDistance) * 100) / distDelta;

			int percentDelta = 100 - minDamagePercent;
			iPercent = MAX(100 - percentDelta * distProgress / 100, minDamagePercent);
		}
	}

#ifdef ENABLE_QUIVER_SYSTEM
	if (pkArrow->GetSubType() == WEAPON_QUIVER)
		iPercent = 100;
#endif

	if (iPercent <= 0)
		return 0;
	else if (iPercent > 100)
		iPercent = 100;

	int iDam = 0;

	float fAR = CalcAttackRating(pkAttacker, pkVictim, false);
	int arrow_damage_bonus = pkArrow->GetValue(3);
	iDam = number(pkBow->GetValue(3) + arrow_damage_bonus, pkBow->GetValue(4) + arrow_damage_bonus) * 2;
	int iAtk;

	// level must be ignored when multiply by fAR, so subtract it before calculation.
	iAtk = pkAttacker->GetPoint(POINT_ATT_GRADE) + iDam - (pkAttacker->GetLevel() * 2);
	iAtk = (int) (iAtk * fAR);
	iAtk += pkAttacker->GetLevel() * 2; // and add again

	// Refine Grade
	iAtk += pkBow->GetValue(5) * 2;

	iAtk += pkAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS);
	iAtk += pkAttacker->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_ATTACK_BONUS); // marriage attack bonus
	iAtk = (int) (iAtk * (100 + (pkAttacker->GetPoint(POINT_ATT_BONUS) + pkAttacker->GetPoint(POINT_MELEE_MAGIC_ATT_BONUS_PER))) / 100);

	int iDef = 0;
	if (!bIgnoreDefense)
	{
		if (pkAttacker->IsPC())
		{
			if (dwSkill)
			{
				iAtk = MAX(iAtk - pkVictim->GetMagicDefenceRating() * iAtk / 100, 1);
			}
			else
				iDef = pkVictim->GetDefence();
		}
		else
			iDef = pkVictim->GetDefence();

		iDef += pkVictim->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_DEFENSE_BONUS);
	}

	if (pkAttacker->IsNPC())
		iAtk = (int)(iAtk * pkAttacker->GetMobDamageMultiply());


	// ogolny nerf obrazen ze zwyklych strzal dla archera
	if (dwSkill == 0 &&
		pkVictim != pkAttacker->GetTarget() && pkVictim->IsNPC() &&
		victimCount > 1)
	{
		if (pkVictim->GetMobRank() < MOB_RANK_BOSS)
		{
			int max_damage_reduction = 30;
			int min_damage_reduction = 10;
			int delta_damage_reduction = max_damage_reduction - min_damage_reduction;
			int progress = MIN(pkAttacker->GetLevel()*100 / 60, 100);

			int reduction = max_damage_reduction - delta_damage_reduction * progress / 100;

			if (victimCount == 2)
				reduction /= 2;

			iAtk = iAtk * (100 - reduction) / 100;
		}
	}

	// buff 10% w bossy, minibossy i metiny
	if (pkVictim->IsNPC() && pkVictim->GetMobRank() >= MOB_RANK_BOSS || IsMiniBoss(pkVictim->GetRaceNum()))
	{
		iAtk += iAtk * 10 / 100;
	}

	iAtk = CalcAttBonus(pkAttacker, pkVictim, iAtk);

	iDam = MAX(1, iAtk - iDef);

	int iPureDam = iDam;
	iPureDam = (iPureDam * iPercent) / 100;
	iPureDam = MAX(iPureDam, 1);



	if (test_server)
	{
		pkAttacker->ChatPacket(CHAT_TYPE_INFO, "ARROW %s -> %s, DAM %d DIST %d iPercent %d",
			pkAttacker->GetName(),
			pkVictim->GetName(),
			iPureDam,
			iDist, iPercent);
	}

	return iPureDam;
}

void NormalAttackAffect(LPCHARACTER pkAttacker, LPCHARACTER pkVictim)
{
	if (pkAttacker->GetPoint(POINT_POISON_PCT) && !pkVictim->IsAffectFlag(AFF_POISON))
	{
		if (number(1, 100) <= pkAttacker->GetPoint(POINT_POISON_PCT))
			pkVictim->AttackedByPoison(pkAttacker);
	}
#ifdef ENABLE_WOLFMAN_CHARACTER
	if (pkAttacker->GetPoint(POINT_BLEEDING_PCT) && !pkVictim->IsAffectFlag(AFF_BLEEDING))
	{
		if (number(1, 100) <= pkAttacker->GetPoint(POINT_BLEEDING_PCT))
			pkVictim->AttackedByBleeding(pkAttacker);
	}
#endif
	if (pkAttacker->GetPoint(POINT_FIRE_PCT) && !pkVictim->IsAffectFlag(AFF_FIRE))
	{
		if (number(1, 100) <= pkAttacker->GetPoint(POINT_FIRE_PCT))
		{
			int amount = pkAttacker->GetLevel() * 2 + pkAttacker->GetDefence() / 1.5f;
			if (pkAttacker->IsNPC())
			{
				amount += pkAttacker->GetPoint(POINT_IQ) * 3;
			}
			pkVictim->AttackedByFire(pkAttacker, amount, 5);
		}
	}

	int iStunDuration = 2;
	if (pkAttacker->IsPC() && !pkVictim->IsPC())
		iStunDuration = 4;

	AttackAffect(pkAttacker, pkVictim, POINT_STUN_PCT, IMMUNE_STUN,  AFFECT_STUN, POINT_NONE,        0, AFF_STUN, iStunDuration, "STUN");
	AttackAffect(pkAttacker, pkVictim, POINT_SLOW_PCT, IMMUNE_SLOW,  AFFECT_SLOW, POINT_MOV_SPEED, -30, AFF_SLOW, 20,		"SLOW");
	AttackAffect(pkAttacker, pkVictim, POINT_ROOT_PCT, IMMUNE_ROOT, AFFECT_ROOT, POINT_NONE, 0, AFF_ROOT, 5,		"ROOT");
}

int battle_hit(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int & iRetDam)
{
	if (test_server)
		sys_log(0, "battle_hit : [%s] attack to [%s] : dam :%d type :%d", pkAttacker->GetName(), pkVictim->GetName(), iRetDam);

	bool bIsPenetrate = pkAttacker ? pkAttacker->PenetrateHit(pkVictim) : false;
	int iDam = CalcMeleeDamage(pkAttacker, pkVictim, bIsPenetrate);

	if (iDam <= 0)
		return (BATTLE_DAMAGE);

	NormalAttackAffect(pkAttacker, pkVictim);

	iDam = CalcDamBonus(pkAttacker, pkVictim, iDam);

	float attMul = pkAttacker->GetAttMul();
	float tempIDam = iDam;
	iDam = attMul * tempIDam + 0.5f;

	iRetDam = iDam;

	if (pkVictim->Damage(pkAttacker, iDam, DAMAGE_TYPE_NORMAL, 0, bIsPenetrate))
		return (BATTLE_DEAD);

	return (BATTLE_DAMAGE);
}

DWORD GET_ATTACK_SPEED(LPCHARACTER ch)
{
	if (NULL == ch)
		return 1000;

	LPITEM item = ch->GetWear(WEAR_WEAPON);
	DWORD default_bonus = SPEEDHACK_LIMIT_BONUS;
	DWORD riding_bonus = 0;

	if (ch->IsRiding())
	{
		riding_bonus = 50;
	}

	DWORD ani_speed = ani_attack_speed(ch);
	DWORD real_speed = (ani_speed * 100) / (default_bonus + ch->GetPoint(POINT_ATT_SPEED) + riding_bonus);

	if (item && item->GetSubType() == WEAPON_DAGGER)
		real_speed /= 2;
#ifdef ENABLE_WOLFMAN_CHARACTER
	else if (item && item->GetSubType() == WEAPON_CLAW)
		real_speed /= 2;
#endif

	return real_speed;
}

void SET_ATTACK_TIME(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time)
{
	if (NULL == ch || NULL == victim)
		return;

	if (!ch->IsPC())
		return;

	ch->m_kAttackLog.dwVID = victim->GetVID();
	ch->m_kAttackLog.dwTime = current_time;
}

void SET_ATTACKED_TIME(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time)
{
	if (NULL == ch || NULL == victim)
		return;

	if (!ch->IsPC())
		return;

	victim->m_AttackedLog.dwPID			= ch->GetPlayerID();
	victim->m_AttackedLog.dwAttackedTime= current_time;
}

bool IS_SPEED_HACK(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time, bool isBow, DWORD victimCount, DWORD dwSkillVnum)
{
	if (!ch->HasPlayerData())
		return false;

	if (isBow)
	{
		auto lastTargetTime = ch->playerData->m_LastShootTargetTime;
		auto deltaTime = get_dword_time() - lastTargetTime;

		DWORD tolerance_multiplier = 83;

		if (ch->GetRaceNum() == MAIN_RACE_ASSASSIN_M)
		{
			tolerance_multiplier = 55;
		}

		DWORD attackSpeed = ch->GetPlayerRealAttackSpeed() * tolerance_multiplier / 100;



		if (ch->IsHorseRiding())
			attackSpeed = attackSpeed * 24 / 100;

		if (deltaTime < attackSpeed)
		{
			ch->ChatDebug("target swap faster than attackSpeed (%d < %d)", deltaTime, attackSpeed);
			return true;
		}
	}

	if (dwSkillVnum == 0)
	{
		const auto movePacketLastAttackTimeDelta = get_dword_time() - ch->GetLastAttackTime();

		DWORD tolerance = 155;

		auto movePacketTimeLimit = ch->GetPlayerRealAttackSpeed(false, 100) * tolerance / 100;
		//ch->ChatDebug("check move packet (%d > %d)", movePacketLastAttackTimeDelta, movePacketTimeLimit);
		if (movePacketLastAttackTimeDelta > movePacketTimeLimit)
		{
			ch->ChatDebug("where is move packet? (%d > %d)", movePacketLastAttackTimeDelta, movePacketTimeLimit);
			return true;
		}
	}

	DWORD lastAttackTime = ch->playerData->m_lastAttackTime;
	DWORD deltaTime = current_time - lastAttackTime;
	ch->playerData->m_lastAttackTime = current_time;
	// sprawdzamy czy gracz nie bije za duzo targetow na raz
	if (deltaTime < ATTACK_MAX_DELTA_TIME_COUNT_SINGLE_ATTACK) {
		ch->playerData->victimCount++;
		if (ch->playerData->victimCount > ATTACK_MAX_VICTIM) {
			ch->ChatDebug("SPEED_HACK: single attack had more than %d victims (%d).", ATTACK_MAX_VICTIM, ch->playerData->victimCount);
			return true;
		}
	}
	else
		ch->playerData->victimCount = 0;

	DWORD attackSpeed = GET_ATTACK_SPEED(ch);
	if (isBow && dwSkillVnum != SKILL_REPETITIVE_SHOT && victimCount == 1 &&
		deltaTime < attackSpeed)
	{
		INCREASE_SPEED_HACK_COUNT(ch);

		if (test_server)
		{

			sys_log(0, "%s attack hack! time (delta, limit)=(%u, %u) hack_count %d",
				ch->GetName(),
				current_time - lastAttackTime,
				attackSpeed,
				ch->m_speed_hack_count);

			ch->ChatPacket(CHAT_TYPE_INFO, "%s attack hack! time (delta, limit)=(%u, %u) hack_count %d",
				ch->GetName(),
				current_time - lastAttackTime,
				attackSpeed,
				ch->m_speed_hack_count);

		}

		return true;
	}

	CHARACTER::THackAttackLog attackLog = ch->GetAttackLog(victim->GetVID());
	if (attackLog.last_attack_time > 0) {

		DWORD target_attack_delta_time = current_time - attackLog.last_attack_time;
		DWORD tolerance_multiplier = 83;
		DWORD player_attack_speed = ch->GetPlayerRealAttackSpeed() * tolerance_multiplier / 100;

		int min_attack_speed = REAL_ATTACK_SPEED_MIN;
		if (dwSkillVnum == SKILL_SHOOTING_DRAGON)
			min_attack_speed = 100;

		if (target_attack_delta_time < min_attack_speed) {
			ch->ChatDebug("SPEED_HACK: target_attack_delta_time < REAL_ATTACK_SPEED_MIN (%d < %d)", target_attack_delta_time, min_attack_speed);
			return true;
		}

		DWORD combo_delta = current_time - attackLog.combo_checker_time;

		DWORD average_speed = ch->GetPlayerRealAttackSpeed(true);
		DWORD average_attack_count_per_2_sec = 2000 / average_speed;

		if (dwSkillVnum == 0 && ch->GetComboIndex() > 0)
		{
			average_attack_count_per_2_sec += 1;
		}
		else if (isBow && dwSkillVnum == SKILL_REPETITIVE_SHOT)
		{
			player_attack_speed = 100;
			average_attack_count_per_2_sec = GetMaxAttackCountForSkill(SKILL_REPETITIVE_SHOT, ch->GetSkillPower(SKILL_REPETITIVE_SHOT));
		}
		else if (dwSkillVnum == SKILL_THREE_WAY_CUT)
		{
			player_attack_speed = player_attack_speed * 50 / 100;
		}

		if (combo_delta < 2000) // 2 sec
		{
			attackLog.combo_count++;
			ch->UpdateAttackLog(victim->GetVID(), attackLog);

			if (attackLog.combo_count > average_attack_count_per_2_sec) {
				ch->ChatDebug("TOO MANY HITS DETECTED!!! (cur %d max %d)", attackLog.combo_count, average_attack_count_per_2_sec);
				return true;
			}
		}
		else {
			attackLog.combo_checker_time = current_time;
			attackLog.combo_count = 0;
			ch->UpdateAttackLog(victim->GetVID(), attackLog);
		}


		if (target_attack_delta_time < player_attack_speed)
		{
			ch->ChatDebug("SPEED_HACK: delta_time %d real_speed_attack %d", target_attack_delta_time, player_attack_speed);
			return true;
		}
	}

	return false;
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
