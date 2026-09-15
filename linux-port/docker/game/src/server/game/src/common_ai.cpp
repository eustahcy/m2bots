#include "stdafx.h"

#include "config.h"
#include "char.h"
#include "common_ai.h"

void CommonAI_HealOnDamage(LPCHARACTER victim, int& damage, BYTE damageType)
{
	if (victim == NULL)
		return;

	CAffect* pkAff = victim->FindAffect(AFFECT_HEAL_ON_DAMAGE);
	if (pkAff)
	{
		if (victim->GetHP() < victim->GetMaxHP())
			victim->PointChange(POINT_HP, damage);
		damage = 0;
		pkAff->lDuration = 3;
		victim->BroadcastTargetPacket(true);
	}
	else
	{
		if (number(1, 100) <= 1)
		{
			victim->AddAffect(AFFECT_HEAL_ON_DAMAGE, POINT_NONE, 0, AFF_NONE, 3, 0, true);
		}
	}
}

// Files shared by GameCore.top
