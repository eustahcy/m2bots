#ifndef __INC_METIN_II_GAME_BATTLE_H__
#define __INC_METIN_II_GAME_BATTLE_H__

#include "char.h"

enum EBattleTypes
{
	BATTLE_NONE,
	BATTLE_DAMAGE,
	BATTLE_DEFENSE,
	BATTLE_DEAD
};

extern int	SumAttResBonus(int attbonus, int resbonus);
extern int	SumBreakBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int attBonusID, int resistBonusID);
extern int	CalcPlayerBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int iDam);
extern int	CalcDamBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int iDam);
extern int	CalcAttBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int iAtk);
extern int	CalcBattleDamage(int iDam, int iAttackerLev, int iVictimLev);
extern int	CalcMeleeDamage(LPCHARACTER pAttacker, LPCHARACTER pVictim, bool bIgnoreDefense = false, bool bIgnoreTargetRating = false);
extern int	CalcMagicDamage(LPCHARACTER pAttacker, LPCHARACTER pVictim);
extern int	CalcArrowDamage(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, LPITEM pkBow, LPITEM pkArrow, bool bIgnoreDefense = false, DWORD dwSkill = 0, int victimCount=1);
extern float	CalcAttackRating(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, bool bIgnoreTargetRating = false);

#ifdef NEW_ICEDAMAGE_SYSTEM
extern bool battle_is_icedamage(LPCHARACTER pAttacker, LPCHARACTER pVictim);
#endif
extern bool	battle_is_attackable(LPCHARACTER ch, LPCHARACTER victim);
extern int	battle_melee_attack(LPCHARACTER ch, LPCHARACTER victim);
extern int	battle_range_attack(LPCHARACTER ch, LPCHARACTER victim);
extern void	battle_end(LPCHARACTER ch);

extern bool	battle_distance_valid_by_xy(long x, long y, long tx, long ty);
extern bool	battle_distance_valid(LPCHARACTER ch, LPCHARACTER victim);
extern int	battle_count_attackers(LPCHARACTER ch);

extern void	NormalAttackAffect(LPCHARACTER pkAttacker, LPCHARACTER pkVictim);

extern bool is_distance_skill(BYTE skillId);

inline void AttackAffect(LPCHARACTER pkAttacker,
		LPCHARACTER pkVictim,
		BYTE att_point,
		DWORD immune_flag,
		DWORD affect_idx,
		BYTE affect_point,
		long affect_amount,
		DWORD affect_flag,
		int time,
		const char* name)
{

	bool isImmune = pkVictim->IsImmune(immune_flag, pkAttacker);
	int pct_value = pkAttacker ? pkAttacker->GetPoint(att_point) : 100;

	if (pct_value && !pkVictim->IsAffectFlag(affect_flag))
	{
		if (pkAttacker)
			pkAttacker->ChatDebug("AttackAffect success: %d%%", pct_value);

		if (number(1, 100) <= pct_value && !isImmune)
		{
			pkVictim->AddAffect(affect_idx, affect_point, affect_amount, affect_flag, time, 0, true);

			if (test_server)
			{
				pkVictim->ChatPacket(CHAT_TYPE_PARTY, "%s %s(%ld%%) SUCCESS", pkAttacker->GetName(), name, pct_value);
			}
		}
		else if (test_server)
		{
			pkVictim->ChatPacket(CHAT_TYPE_PARTY, "%s %s(%ld%%) FAIL", pkAttacker->GetName(), name, pct_value);

		}
	}
}

inline void SkillAttackAffect(LPCHARACTER pkAttacker,
		LPCHARACTER pkVictim,
		int success_pct,
		DWORD immune_flag,
		DWORD affect_idx,
		BYTE affect_point,
		long affect_amount,
		DWORD affect_flag,
		int time,
		const char* name)
{
	bool isImmune = pkVictim->IsImmune(immune_flag, pkAttacker);
	if (name == "GM_STUN" || name == "GM_SLOW")
		isImmune = false;

	if (success_pct && !pkVictim->IsAffectFlag(affect_flag))
	{
		if (pkAttacker)
			pkAttacker->ChatDebug("SkillAttackAffect success: %d%%", success_pct/10);
		if (number(1, 1000) <= success_pct && !isImmune)
		{
			pkVictim->ChatDebug("SkillAttackAffect success affect %d point %d amount %d flag %d", affect_idx, affect_point, affect_amount, affect_flag);
			pkVictim->AddAffect(affect_idx, affect_point, affect_amount, affect_flag, time, 0, true);

			// SKILL_ATTACK_NO_LOG_TARGET_NAME_FIX
			if (test_server)
				pkVictim->ChatPacket(CHAT_TYPE_PARTY,
					"%s(%d%%) -> %s SUCCESS", name, success_pct, name);
			// END_OF_SKILL_ATTACK_LOG_NO_TARGET_NAME_FIX
		}
		else if (test_server)
		{
			// SKILL_ATTACK_NO_LOG_TARGET_NAME_FIX
			pkVictim->ChatPacket(CHAT_TYPE_PARTY, "%s(%d%%) -> %s FAIL", name, success_pct, name);
			// END_OF_SKILL_ATTACK_LOG_NO_TARGET_NAME_FIX
		}
	}

	if (pkVictim && pkVictim->IsBusyAction())
	{
		pkVictim->playerData->SetBusyAction(pkVictim, 0); // interrupt on stun
	}
}

#define GET_SPEED_HACK_COUNT(ch)		((ch)->m_speed_hack_count)
#define INCREASE_SPEED_HACK_COUNT(ch)	(++GET_SPEED_HACK_COUNT(ch))
DWORD	GET_ATTACK_SPEED(LPCHARACTER ch);
void	SET_ATTACK_TIME(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time);
void	SET_ATTACKED_TIME(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time);
bool	IS_SPEED_HACK(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time, bool isBow = false, DWORD victimCount = 1, DWORD dwSkillVnum = 0);

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
