#include "stdafx.h"
#include "char.h"
#include "char_manager.h"
#include "boss_giant.h"

void BossGiant_OnStartBattle(LPCHARACTER ch, LPCHARACTER victim)
{
	if (ch == NULL)
		return;

	ch->MonsterChat(LC_TEXT("GIANT_BOSS_START_BATTLE"));
	ch->SetMaxSP(0);
}

LPCHARACTER SpawnAOE(LPCHARACTER ch) {
	if (ch == NULL)
		return NULL;

	return CHARACTER_MANAGER::instance().SpawnMob(7503, ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
}

void BossGiant_OnDamage(LPCHARACTER victim, int& damage, BYTE damageType)
{
	if (victim == NULL)
		return;

	int hp = victim->GetHPPct();
	int step = victim->GetMaxSP();

	if (hp <= 75 && step == 0 ||
		hp <= 50 && step == 1 ||
		hp <= 25 && step == 2 ||
		hp <= 10 && step == 3)
	{
		victim->SetMaxSP(step + 1);
		SpawnAOE(victim);
	}
}

// Files shared by GameCore.top
