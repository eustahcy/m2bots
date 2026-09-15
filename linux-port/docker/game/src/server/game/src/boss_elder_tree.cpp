#include "stdafx.h"
#include "char.h"
#include "char_manager.h"
#include "utils.h"
#include "sectree_manager.h"

extern int passes_per_sec;

#include "boss_elder_tree.h"

const int ELDER_TREE_EVENT_CYCLE = 10;
const int ELDER_TREE_BOSS_VNUM = 2307;
const int ELDER_TREE_AOE_VNUM = 7501;
const int ELDER_TREE_AOE_SMALL_VNUM = 7502;
const int ELDER_TREE_KILL_EVENT_NPC_VNUM = 20404;
const int ELDER_TREE_MOB_HEALER = 2316;


LPCHARACTER SpawnCharacterAround(DWORD vnum, long map_index, long x, long y, long radius, float t)
{
	PIXEL_POSITION pos = get_pos_on_circle(x, y, radius, t);
	LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(vnum,
		map_index,
		pos.x,
		pos.y,
		0,
		true);
	return ch;
}

struct FElderTreeSpawnAoeDistance
{
	LPCHARACTER	m_me;

	FElderTreeSpawnAoeDistance(LPCHARACTER me) : m_me(me)
	{
	}

	void operator () (LPENTITY ent)
	{
		if (!m_me)
			return;

		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER victim = (LPCHARACTER)ent;

		if (!victim->IsPC())
			return;

		if (victim->IsDead())
			return;

		if (victim->GetMapIndex() != m_me->GetMapIndex())
			return;

		int distance = m_me->DistanceTo(ent);
		if (distance < 500 || distance > 2200)
			return;

		CHARACTER_MANAGER::instance().SpawnMob(ELDER_TREE_AOE_SMALL_VNUM, victim->GetMapIndex(), victim->GetX(), victim->GetY(), victim->GetZ());
	}
};

EVENTFUNC(boss_elder_tree_post_healer_event)
{
	boss_elder_tree_healer_event_info* info = dynamic_cast<boss_elder_tree_healer_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("boss_elder_tree_post_healer_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	LPCHARACTER	boss_ch = info->boss_ch;
	if (ch == NULL) {
		return 0;
	}

	if (ch->IsDead())
	{
		ch->m_pkAIEvent = NULL;
		return 0;
	}

	if (!boss_ch || boss_ch->IsDead() || !boss_ch->GetVictim())
	{
		ch->m_pkAIEvent = NULL;
		M2_DESTROY_CHARACTER(ch);
		return 0;
	}

	if (ch->DistanceTo(boss_ch) < 2000)
	{
		ch->CreateFly(FLY_HP_BIG, boss_ch);
		boss_ch->PointChange(POINT_HP, MAX(1, (boss_ch->GetMaxHP() * 2) / 100));
	}
	return PASSES_PER_SEC(number(20, 30));
}

EVENTFUNC(boss_elder_tree_event)
{
	boss_elder_tree_event_info* info = dynamic_cast<boss_elder_tree_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("boss_elder_tree_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (ch->IsDead())
	{
		ch->m_pkAIEvent = NULL;
		return 0;
	}


	if (ch->GetVictim())
	{
		info->value1++;
		info->value2++;

		if (info->value1 >= 3) {
			CHARACTER_MANAGER::instance().SpawnMob(ELDER_TREE_AOE_VNUM, ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
			info->value1 = 0;
		}
		else if (info->value1 == 2)
		{
			LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());
			FElderTreeSpawnAoeDistance f(ch);
			pMap->for_each(f);
		}

		if (info->value2 >= 6)
		{
			info->value2 = 0;
			long boss_x = ch->GetX();
			long boss_y = ch->GetY();
			for (int i = 0; i < 10; i++)
			{
				float t = (float)i / 10;
				LPCHARACTER spawn = SpawnCharacterAround(ELDER_TREE_MOB_HEALER, ch->GetMapIndex(), boss_x, boss_y, 1200, t);
				if (spawn)
				{
					spawn->SetAggressive();
					boss_elder_tree_healer_event_info* info = AllocEventInfo<boss_elder_tree_healer_event_info>();
					info->ch = spawn;
					info->boss_ch = ch;
					spawn->m_pkAIEvent = event_create(boss_elder_tree_post_healer_event, info, PASSES_PER_SEC(1));
				}
			}
		}
	}
	else
	{
		// reset jak nie walczy
		if (ch->GetHP() < ch->GetMaxHP())
			ch->PointChange(POINT_HP, MAX(1, (ch->GetMaxHP() * 10) / 100)); // regen o 10% co cykl jak nie walczy
		info->value1 = 0;
		info->value2 = 0;
	}

	return PASSES_PER_SEC(ELDER_TREE_EVENT_CYCLE);
}

EVENTFUNC(boss_elder_tree_post_kill_event)
{
	boss_elder_tree_post_kill_info* info = dynamic_cast<boss_elder_tree_post_kill_info*>(event->info);
	if (info == NULL)
	{
		sys_err("boss_elder_tree_post_kill_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) {
		return 0;
	}

	if (ch->IsDead())
	{
		ch->m_pkAIEvent = NULL;
		return 0;
	}

	if (ch->DistanceTo(info->dest_x, info->dest_y) < 50)
	{
		if (ch->GetMaxSP() == 0)
		{
			ch->SetMaxSP(1);
			ch->MonsterChat(LC_TEXT("WORLDBOSS_TRENT2_POSTKILL_DIALOG2"));
			return PASSES_PER_SEC(5);
		}
		else if (ch->GetMaxSP() == 1)
		{
			ch->SetMaxSP(2);
			ch->MonsterChat(LC_TEXT("WORLDBOSS_TRENT2_POSTKILL_DIALOG3"));
			//time_t timeNow = static_cast<time_t>(get_dword_time());
			ch->SendMovePacket(FUNC_ATTACK, 0, ch->GetX(), ch->GetY(), 0);
			return PASSES_PER_SEC(5);
		}
		else if (ch->GetMaxSP() == 2)
		{
			ch->SetMaxSP(3);
			ch->MonsterChat(LC_TEXT("WORLDBOSS_TRENT2_POSTKILL_DIALOG4"));
			return PASSES_PER_SEC(4);
		}
		else
		{
			ch->m_pkAIEvent = NULL;
			M2_DESTROY_CHARACTER(ch);
			return 0;
		}
	}
	else
	{
		return PASSES_PER_SEC(2); // czekamy az dojdzie
	}
	return 0;
}

void BossElderTree_Spawn(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (ch->GetRaceNum() != ELDER_TREE_BOSS_VNUM)
		return;

	boss_elder_tree_event_info* info = AllocEventInfo<boss_elder_tree_event_info>();
	info->ch = ch;
	info->value1 = 0;
	info->value2 = 0;
	ch->m_pkAIEvent = event_create(boss_elder_tree_event, info, PASSES_PER_SEC(ELDER_TREE_EVENT_CYCLE));
}

void BossElderTree_Dead(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (ch->GetRaceNum() != ELDER_TREE_BOSS_VNUM)
		return;

	LPCHARACTER new_ch = NULL;
	long boss_x = ch->GetX();
	long boss_y = ch->GetY();
	for (int i = 0; i < 10; i++)
	{
		float t = (float)i / 10;
		new_ch = SpawnCharacterAround(ELDER_TREE_KILL_EVENT_NPC_VNUM, ch->GetMapIndex(), boss_x, boss_y, 1000, t);
		if (new_ch)
			break;
	}

	if (!new_ch)
		return;

	new_ch->MonsterChat(LC_TEXT("WORLDBOSS_TRENT2_POSTKILL_DIALOG1"));

	new_ch->Goto(boss_x, boss_y);
	new_ch->SetRotationToXY(boss_x, boss_y);
	new_ch->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

	boss_elder_tree_post_kill_info* info = AllocEventInfo<boss_elder_tree_post_kill_info>();
	info->ch = new_ch;
	info->dest_x = boss_x;
	info->dest_y = boss_y;
	new_ch->m_pkAIEvent = event_create(boss_elder_tree_post_kill_event, info, PASSES_PER_SEC(5));
}


// Files shared by GameCore.top
