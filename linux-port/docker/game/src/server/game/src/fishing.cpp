//#define __FISHING_MAIN__
#include "stdafx.h"
#include "constants.h"
#include "fishing.h"
#include "locale_service.h"

#ifndef __FISHING_MAIN__
#include "item_manager.h"

#include "config.h"
#include "packet.h"

#include "sectree_manager.h"
#include "char.h"
#include "char_manager.h"

#include "log.h"

#include "questmanager.h"
#include "buffer_manager.h"
#include "desc_client.h"
#include "locale_service.h"

#include "affect.h"
#include "unique_item.h"
#endif

namespace fishing
{
	enum
	{
		MAX_FISH = 37,
		NUM_USE_RESULT_COUNT = 10, // 1 : DEAD 2 : BONE 3 ~ 12 : rest
		FISH_BONE_VNUM = 27799,
		SHELLFISH_VNUM = 27987,
		EARTHWORM_VNUM = 27801,
		WATER_STONE_VNUM_BEGIN = 28030,
		WATER_STONE_VNUM_END = 28043,
		FISH_NAME_MAX_LEN = 64,
		MAX_PROB = 4,
	};

	enum
	{
		USED_NONE,
		USED_SHELLFISH,
		USED_WATER_STONE,
		USED_TREASURE_MAP,
		USED_EARTHWARM,
		MAX_USED_FISH
	};

	enum EFishingQuestState
	{
		FISHING_QUEST_STATE_FAIL,
		FISHING_QUEST_STATE_SUCCESS,
		FISHING_QUEST_STATE_PREDICT
	};

	enum EFishingGameState
	{
		FISHING_GAME_STATE_NONE,
		FISHING_GAME_STATE_PRE_START,
		FISHING_GAME_STATE_IN_PROGRESS,
		FISHING_GAME_STATE_PAST_EVENT,
		FISHING_GAME_STATE_REACT,
	};

	struct SFishInfo
	{
		char name[FISH_NAME_MAX_LEN];

		DWORD vnum;
		DWORD dead_vnum;
		DWORD grill_vnum;

		int prob[MAX_PROB];
		int difficulty;

		int time_type;
		int length_range[3]; // MIN MAX EXTRA_MAX : 99% MIN~MAX, 1% MAX~EXTRA_MAX

		int used_table[NUM_USE_RESULT_COUNT];
		// 6000 2000 1000 500 300 100 50 30 10 5 4 1
	};

	bool operator < (const SFishInfo& lhs, const SFishInfo& rhs)
	{
		return lhs.vnum < rhs.vnum;
	}

	int g_prob_sum[MAX_PROB];
	int g_prob_accumulate[MAX_PROB][MAX_FISH];

	SFishInfo fish_info[MAX_FISH] = { { "\0", }, };
	/*
	   {
	   { "²Î",		00000, 00000, 00000, {  750, 1750, 2750 },   10, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_NORMAL, { 0,           },
	   {0, } },
	   { "±Ý¹ÝÁö",	50002, 00000, 00000, {   50,   50,    0 },  200, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_NORMAL, { 0,           },
	   {0, } },
	   { "ÇÇ¶ó¹Ì",	27802, 00000, 00000, { 2100, 1500,   50 },   10, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_EASY,   {500, 550, 600},
	   {0, } },
	   { "ºØ¾î",	27803, 27833, 27863, { 2100, 1500,  100 },   13, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_EASY,   {1000,2500,2800},
	   {USED_NONE, USED_SHELLFISH, USED_NONE, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "½î°¡¸®",	27804, 27834, 27864, { 1100, 1300,  150 },   16, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_NORMAL, {2000,3500,3800},
	   {USED_NONE, USED_SHELLFISH, USED_NONE, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "¿ùÃ´ºØ¾î",	27805, 27835, 27865, { 1100, 1100,  200 },   20, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_SLOW, {3030,3500,4300},
	   {USED_NONE, USED_SHELLFISH, USED_NONE, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "À×¾î",	27806, 27836, 27866, { 1100,  500,  300 },   26, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_NORMAL, {4000,6000,10000},
	   {USED_NONE, USED_SHELLFISH, USED_NONE, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "¿¬¾î",	27807, 27837, 27867, { 1100,  450,  400 },   33, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_NORMAL,{6000,8000,10000},
	   {USED_NONE, USED_SHELLFISH, USED_NONE, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "Çâ¾î",	27808, 27838, 27868, {  200,  400,  500 },   42, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_NORMAL,{1500,3000,3800},
	   {USED_NONE, USED_SHELLFISH, USED_NONE, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "¼Û¾î",	27809, 27839, 27869, {  200,  300,  700 },   54, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_NORMAL,{5000,7000,8000},
	   {USED_NONE, USED_SHELLFISH, USED_NONE, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "¹Î¹°Àå¾î",	27810, 27840, 27870, {    0,  270, 1000 },   70, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_NORMAL,{4000,5000,6000},
	   {USED_SHELLFISH, USED_NONE, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_NONE, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "¹«Áö°³¼Û¾î",	27811, 27841, 27871, {    0,  200, 1000 },   91, FISHING_LIMIT_APPEAR,	{  0,   0,   0}, FISHING_TIME_NORMAL,{5000,7000,8000},
	   {USED_SHELLFISH, USED_NONE, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_NONE, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "°­¼Û¾î",	27812, 27842, 27872, {    0,  160, 1000 },  118, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_QUICK,{4000,6000,7000},
	   {USED_SHELLFISH, USED_NONE, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_NONE, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "·¯µå",	27813, 27843, 27873, {    0,  130,  700 },  153, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_NORMAL,{4000,6000,10000},
	   {USED_SHELLFISH, USED_NONE, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_NONE, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "ÆÛÄ¡",	27814, 27844, 27874, {    0,  100,  400 },  198, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_NORMAL,{3000,4000,5000},
	   {USED_SHELLFISH, USED_NONE, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_NONE, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "ÅÙÄ¡",	27815, 27845, 27875, {    0,   50,  300 },  257, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_NORMAL,{3500,5500,8000},
	   {USED_SHELLFISH, USED_NONE, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_NONE, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "¸Þ±â",	27816, 27846, 27876, {    0,   30,  100 },  334, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_NORMAL,{3000,5000,10000},
	   {USED_SHELLFISH, USED_NONE, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_NONE, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "¹Ì²Ù¶óÁö",	27817, 27847, 27877, {    0,   10,   64 },  434, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_QUICK,{1800,2200,3000},
	   {USED_SHELLFISH, USED_NONE, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_NONE, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "¹é·Ã",	27818, 27848, 27878, {    0,    0,   15 },  564, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_NORMAL,{5000,8000,10000},
	   {USED_SHELLFISH, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_NONE, USED_NONE, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "Àº¾î",	27819, 27849, 27879, {    0,    0,    9 },  733, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_NORMAL,{1500,3000,3800},
	   {USED_SHELLFISH, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_NONE, USED_NONE, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "ºù¾î",	27820, 27850, 27880, {    0,    0,    6 },  952, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_QUICK,{1500,3000,3800},
	   {USED_SHELLFISH, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_NONE, USED_NONE, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "½¬¸®",	27821, 27851, 27881, {    0,    0,    3 }, 1237, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_NORMAL,{1000,1500,2000},
	   {USED_SHELLFISH, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_NONE, USED_NONE, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "ºñ´ÃÀ×¾î",	27822, 27852, 27882, {    0,    0,    2 }, 1608, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_SLOW,{4000,6000,10000},
	   {USED_SHELLFISH, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_NONE, USED_NONE, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "È²±ÝºØ¾î",	27823, 27853, 27883, {    0,    0,    1 }, 2090, FISHING_LIMIT_NONE,	{  0,   0,   0}, FISHING_TIME_SLOW,{1000,3000,3500},
	   {USED_SHELLFISH, USED_NONE, USED_WATER_STONE, USED_TREASURE_MAP, USED_NONE, USED_NONE, USED_EARTHWARM, USED_NONE,USED_NONE,  USED_NONE } },
	   { "Å»»ö¾à",     70201, 00000, 00000, { 5,    5,  0 },   60, FISHING_LIMIT_NONE,    {  0,   0,   0}, FISHING_TIME_NORMAL, {0,           },
	   {0,	}},
	   { "¿°»ö¾à(Èò»ö)",  70202, 00000, 00000, { 15,  15,  0 },   60, FISHING_LIMIT_NONE,    {  0,   0,   0}, FISHING_TIME_NORMAL, {0,           },
	   {0,	}},
	   { "¿°»ö¾à(±Ý»ö)",  70203, 00000, 00000, { 15,  15,  0 },   60, FISHING_LIMIT_NONE,    {  0,   0,   0}, FISHING_TIME_NORMAL, {0,           },
	   {0,	}},
	   { "¿°»ö¾à(»¡°£»ö)",70204, 00000, 00000, { 15,  15,  0 },   60, FISHING_LIMIT_NONE,    {  0,   0,   0}, FISHING_TIME_NORMAL, {0,           },
	   {0,	}},
	   { "¿°»ö¾à(°¥»ö)",  70205, 00000, 00000, { 15,  15,  0 },   60, FISHING_LIMIT_NONE,    {  0,   0,   0}, FISHING_TIME_NORMAL, {0,           },
	   {0,	}},
	   { "¿°»ö¾à(°ËÀº»ö)",70206, 00000, 00000, { 15,  15,  0 },   60, FISHING_LIMIT_NONE,    {  0,   0,   0}, FISHING_TIME_NORMAL, {0,           },
	   {0,	}},
	   { "ÀºµÐÀÚÀÇ ¸ÁÅä", 70048, 00000, 00000, {  8,   8,  0 },   60, FISHING_LIMIT_NONE,    {  0,   0,   0}, FISHING_TIME_NORMAL, {0,           },
	   {0,	}},
	   { "Çà¿îÀÇ ¹ÝÁö",   70049, 00000, 00000, {  8,   8,  0 },   60, FISHING_LIMIT_NONE,    {  0,   0,   0}, FISHING_TIME_NORMAL, {0,           },
	   {0,	}},
	   { "¼±¿ÕÀÇ ÁõÇ¥",   70050, 00000, 00000, {  8,   8,  0 },   60, FISHING_LIMIT_NONE,    {  0,   0,   0}, FISHING_TIME_NORMAL, {0,           },
	   {0,	}},
	   { "¼±¿ÕÀÇ Àå°©",   70051, 00000, 00000, {  8,   8,  0 },   60, FISHING_LIMIT_NONE,    {  0,   0,   0}, FISHING_TIME_NORMAL, {0,           },
	   {0,	}},
	   { "±Ýµ¢¾î¸®",	   80008, 00000, 00000, { 20,  20,  0 },  250, FISHING_LIMIT_NONE,    {  0,   0,   0}, FISHING_TIME_SLOW,    {0,           },
	   {0, } },
	{ "Àº¿­¼è",	   50009, 00000, 00000, {300, 300, 0, },   70, FISHING_LIMIT_NONE,    { 0, 0, 0}, FISHING_TIME_NORMAL, {0,	}, {0, } },

	{ "±Ý¿­¼è",	   50008, 00000, 00000, {110, 110, 0, },  100, FISHING_LIMIT_NONE,    { 0, 0, 0}, FISHING_TIME_NORMAL, {0,	}, {0, } },
};
	*/

	int GetFishingChance(LPCHARACTER ch, LPITEM rod)
	{
		if (test_server && quest::CQuestManager::instance().GetEventFlag("fishing_chance"))
		{
			return quest::CQuestManager::instance().GetEventFlag("fishing_chance");
		}

		int chance = 20;
		if (ch->FindAffect(AFFECT_FISH_MIND_PILL))
			chance += 2;

		if (ch->GetPremiumRemainSeconds(PREMIUM_FISH_MIND) > 0)
			chance += 10;

		return chance + rod->GetValue(5);
	}

#ifndef __FISHING_MAIN__

	void FishingReact(LPCHARACTER ch)
	{
		TPacketGCFishing p;
		p.header = HEADER_GC_FISHING;
		p.subheader = FISHING_SUBHEADER_GC_REACT;
		p.info = ch->GetVID();
		ch->PacketAround(&p, sizeof(p));
	}

	void FishingSuccess(LPCHARACTER ch)
	{
		TPacketGCFishing p;
		p.header = HEADER_GC_FISHING;
		p.subheader = FISHING_SUBHEADER_GC_SUCCESS;
		p.info = ch->GetVID();
		ch->PacketAround(&p, sizeof(p));
	}

	void FishingFail(LPCHARACTER ch)
	{
		TPacketGCFishing p;
		p.header = HEADER_GC_FISHING;
		p.subheader = FISHING_SUBHEADER_GC_FAIL;
		p.info = ch->GetVID();
		ch->PacketAround(&p, sizeof(p));
	}

	void FishingPractice(LPCHARACTER ch, bool is_simulation)
	{
		LPITEM rod = ch->GetWear(WEAR_WEAPON);
		if (rod && rod->GetType() == ITEM_ROD)
		{
			// ÃÖ´ë ¼ö·Ãµµ°¡ ¾Æ´Ñ °æ¿ì ³¬½Ã´ë ¼ö·Ã
			if (rod->GetRefinedVnum() > 0 && rod->GetSocket(0) < rod->GetValue(2) && number(1, rod->GetValue(1)) == 1)
			{
				rod->SetSocket(0, rod->GetSocket(0) + 1, true, "SOCKET_FISHING");
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your fishing points have increased! (%d/%d)"), rod->GetSocket(0), rod->GetValue(2));
				if (rod->GetSocket(0) == rod->GetValue(2))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have reached the maximum number of fishing points."));
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Go to the Fisherman and get your Fishing Pole upgraded!"));
				}
			}
		}
		if (!is_simulation)
			rod->SetSocket(2, 0, true, "SOCKET_FISHING");
	}

	int GetFishLength(int fish_id)
	{
		if (number(0, 99))
		{
			// 99% : normal size
			return (int)(fish_info[fish_id].length_range[0] +
				(fish_info[fish_id].length_range[1] - fish_info[fish_id].length_range[0])
				* (number(0, 2000) + number(0, 2000) + number(0, 2000) + number(0, 2000) + number(0, 2000)) / 10000);
		}
		else
		{
			// 1% : extra LARGE size
			return (int)(fish_info[fish_id].length_range[1] +
				(fish_info[fish_id].length_range[2] - fish_info[fish_id].length_range[1])
				* 2 * asin(number(0, 10000) / 10000.) / M_PI);
		}
	}

	void SaveEndFishingTime(LPCHARACTER ch)
	{
		if (ch->HasPlayerData())
			ch->playerData->m_iEndFishingTime = get_dword_time();
	}

	void ProcessFishingGameStop(LPCHARACTER ch, LPITEM rodItem, bool isWin)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "FishingGameStop %d", isWin ? 1 : 0);
		ch->m_pkFishingEvent = NULL;
		ch->m_biFishGameState = FISHING_GAME_STATE_NONE;
		SaveEndFishingTime(ch);
		if (rodItem)
		{
			FishingPractice(ch);
			rodItem->SetSocket(2, 0, true, "SOCKET_FISHING");
		}
	}

	void ProcessFishingGameStart(LPCHARACTER ch, DWORD bar_height, DWORD start_delay)
	{
		ch->m_biFishGameState = FISHING_GAME_STATE_PRE_START;
		ch->ChatPacket(CHAT_TYPE_COMMAND, "FishingGameStart %d %d %d", bar_height, ch->m_iFish_position, start_delay);
	}

	void ProcessFishingGameUpdate(LPCHARACTER ch, DWORD top_bar_position)
	{
		TPacketGCFishingGame p;
		p.header = HEADER_GC_FISHING_GAME;
		p.subheader = FISHING_GAME_SUBHEADER_GC_GAME_UPDATE;
		p.size = sizeof(TPacketGCFishingGame) + sizeof(TPacketGCFishingGamePlayerUpdate) + sizeof(TPacketGCFishingGameUpdate);

		TPacketGCFishingGamePlayerUpdate playerPack;
		playerPack.player_pos = ch->m_iFish_position;

		TPacketGCFishingGameUpdate gamePack;
		gamePack.top_bar_pos = top_bar_position;

		LPDESC d = ch->GetDesc();
		if (d)
		{
			d->BufferedPacket(&p, sizeof(TPacketGCFishingGame));
			d->BufferedPacket(&playerPack, sizeof(TPacketGCFishingGamePlayerUpdate));
			d->Packet(&gamePack, sizeof(TPacketGCFishingGameUpdate));
		}
	}

	void ProcessFishingGameSuccess(LPCHARACTER ch)
	{
		ch->AddPlayerStat(PLAYER_STATS_FISHING_FLAG);
		quest::CQuestManager::instance().Fishing(ch->GetPlayerID(), FISHING_QUEST_STATE_SUCCESS);
		FishingSuccess(ch);
		ch->m_iFishingChanceBonus = 0;
		//if (item)
		//{
		//	item->SetSocket(0, GetFishLength(fish_id), true, "SOCKET_FISHING");
		//	if (test_server)
		//	{
		//		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The length of the captured fish is %.2fcm."), item->GetSocket(0) / 100.f);
		//	}
		//}
	}

	LPITEM CheckRod(LPCHARACTER ch) {
		LPITEM rod = ch->GetWear(WEAR_WEAPON);
		if (!(rod && rod->GetType() == ITEM_ROD) || ch->IsRiding())
		{
			return NULL;
		}
		return rod;
	}

	EVENTFUNC(pre_fishing_event)
	{
		pre_fishing_event_info* info = dynamic_cast<pre_fishing_event_info*>(event->info);

		if (info == NULL)
		{
			sys_err("fishing_event> <Factor> Null pointer");
			return 0;
		}

		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(info->pid);

		if (!ch)
			return 0;

		LPITEM rod = CheckRod(ch);
		if (!rod)
		{
			ch->m_pkPreFishingEvent = NULL;
			return 0;
		}

		if (info->step == 0)
		{
			quest::CQuestManager::instance().Fishing(ch->GetPlayerID(), FISHING_QUEST_STATE_PREDICT);
			ch->m_bPlayerFishReactTime = get_dword_time();
			FishingReact(ch);
		}
		if (info->step == 4)
		{
			FishingFail(ch);
			SaveEndFishingTime(ch);
			rod->SetSocket(2, 0, true, "SOCKET_FISHING");
			ch->m_pkPreFishingEvent = NULL;
			return 0;
		}
		++info->step;
		return PASSES_PER_SEC(1);
	}

	EVENTFUNC(fishing_event)
	{
		fishing_event_info* info = dynamic_cast<fishing_event_info*>(event->info);

		if (info == NULL)
		{
			sys_err("fishing_event> <Factor> Null pointer");
			return 0;
		}

		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(info->pid);

		if (!ch)
			return 0;

		LPITEM rod = CheckRod(ch);
		if (!rod)
		{
			ProcessFishingGameStop(ch, NULL, false);
			return 0;
		}

		if (info->step == FISHING_GAME_STATE_PRE_START)
		{
			info->cooldown--;
			ch->ChatPacket(CHAT_TYPE_COMMAND, "FishingGameCooldown %d", info->cooldown);
			if (info->cooldown > 0)
				return PASSES_PER_SEC(1);
			else
			{
				ch->m_biFishGameState = FISHING_GAME_STATE_IN_PROGRESS;
				++info->step;
			}
		}

		else if (info->step == FISHING_GAME_STATE_IN_PROGRESS) // step 2 - in progress
		{
			int rnd = number(1, 10);
			BYTE eventType = FISHING_GAME_EVENT_NONE;
			BYTE eventDuration = 0;

			if (rnd == 1)
			{
				ch->ChatDebug("fishing game event stop");
				eventType = FISHING_GAME_EVENT_STOP;
				eventDuration = 3;

			}
			else if (rnd <= 3 && (info->bar_position + info->bar_height) > 50)
			{
				ch->ChatDebug("fishing game event reverse");
				eventType = FISHING_GAME_EVENT_REVERSE;
				eventDuration = 3;
			}
			else if (rnd == 4 && (info->bar_position > 15))
			{
				ch->ChatDebug("fishing game event speedup");
				eventType = FISHING_GAME_EVENT_SPEEDUP;
				eventDuration = 3;
			}

			if (eventType != FISHING_GAME_EVENT_NONE)
			{
				info->event_type = eventType;
				info->event_length = info->update_count + eventDuration;
				info->step++;

				ch->ChatPacket(CHAT_TYPE_COMMAND, "FishingGameEvent %u %u", eventType, eventDuration + 1);
			}
		}
		// step FISHING_GAME_STATE_PAST_EVENT


		int speed = info->bar_speed / (info->update_count == 0 ? 2 : 1);

		if (info->event_type == FISHING_GAME_EVENT_STOP)
		{
			speed = 0;
		}
		else if (info->event_type == FISHING_GAME_EVENT_REVERSE)
		{
			speed = -2;
		}
		else if (info->event_type == FISHING_GAME_EVENT_SPEEDUP)
		{
			//speed = (speed * 150) / 100;
			speed = MIN(speed * 2, 13);
		}

		BYTE bottom_bar_position = info->bar_position;
		BYTE top_bar_position = bottom_bar_position + info->bar_height;
		ch->IncrementPlayerFishGamePosition(-2);
		ch->ChatDebug("top: %u bottom: %u fish: %u updateCount: %d", top_bar_position, bottom_bar_position, ch->m_iFish_position, info->update_count);

		// fail
		int top_tolerance = 2;
		if (ch->m_iFish_position < bottom_bar_position || ch->m_iFish_position >(top_bar_position + top_tolerance))
		{
			ch->ChatDebug("przejebales");
			FishingFail(ch);
			ProcessFishingGameStop(ch, rod, false);
			return 0;
		}

		// success
		if (top_bar_position >= 100) {
			ProcessFishingGameSuccess(ch);
			ProcessFishingGameStop(ch, rod, true);
			return 0;
		}
		info->update_count++;

		if (info->update_count > info->event_length) {
			info->event_type = FISHING_GAME_EVENT_NONE;
		}


		info->bar_position = MINMAX(0, info->bar_position + speed, 100);
		bottom_bar_position = info->bar_position;
		top_bar_position = bottom_bar_position + info->bar_height;
		ProcessFishingGameUpdate(ch, top_bar_position);
		return (passes_per_sec / 2);
	}

	LPEVENT CreatePreFishingEvent(LPCHARACTER ch)
	{
		pre_fishing_event_info* info = AllocEventInfo<pre_fishing_event_info>();
		info->pid = ch->GetPlayerID();
		info->step = 0;

		int time;
		if (test_server && quest::CQuestManager::instance().GetEventFlag("fishing_delay"))
		{
			time = 4;
		}
		else
		{
			time = 17;
		}

		TPacketGCFishing p;
		p.header = HEADER_GC_FISHING;
		p.subheader = FISHING_SUBHEADER_GC_START;
		p.info = ch->GetVID();
		p.dir = (BYTE)(ch->GetRotation() / 5);
		ch->PacketAround(&p, sizeof(TPacketGCFishing));

		return event_create(pre_fishing_event, info, PASSES_PER_SEC(time));
	}

	LPEVENT CreateFishingEvent(LPCHARACTER ch)
	{
		fishing_event_info* info = AllocEventInfo<fishing_event_info>();
		info->pid = ch->GetPlayerID();
		info->step = FISHING_GAME_STATE_PRE_START;
		info->bar_height = number(25, 40);
		info->bar_speed = number(5, 9);

		ch->m_iFish_position = info->bar_height * number(50, 75) / 100;

		ProcessFishingGameStart(ch, info->bar_height, 5);
		info->cooldown = 5;
		return event_create(fishing_event, info, PASSES_PER_SEC(1));
	}

	int GetFishingLevel(LPCHARACTER ch)
	{
		LPITEM rod = ch->GetWear(WEAR_WEAPON);

		if (!rod || rod->GetType() != ITEM_ROD)
			return 0;

		return rod->GetSocket(2) + rod->GetValue(0);
	}

	void Take(LPCHARACTER ch)
	{
		if (ch && ch->m_pkPreFishingEvent)
		{
			DWORD deltaTime = get_dword_time() - ch->m_bPlayerFishReactTime;
			ch->ChatDebug("React action delta: %d", deltaTime);
			LPITEM rod = CheckRod(ch);
			if (deltaTime >= 1700 && deltaTime <= 3500) {
				int chance = GetFishingChance(ch, rod);
				ch->m_iFishingChanceBonus = MIN(ch->m_iFishingChanceBonus, MAX(25 - chance / 2, 0));
				ch->ChatDebug("Fishing chance %d (bonus %d)", chance, ch->m_iFishingChanceBonus);
				if (rod && number(1, 100) <= chance + ch->m_iFishingChanceBonus)
				{
					event_cancel(&ch->m_pkPreFishingEvent);
					ch->m_pkFishingEvent = CreateFishingEvent(ch);
					return;
				}
				else {
					FishingFail(ch);
					ch->m_iFishingChanceBonus += 2;
					//quest::CQuestManager::instance().Fishing(ch->GetPlayerID(), FISHING_QUEST_STATE_FAIL);
				}

				if (rod)
				{
					FishingPractice(ch);
					rod->SetSocket(2, 0, true, "SOCKET_FISHING");
				}
			}
			else if (deltaTime < 1700 || deltaTime > 3500 && deltaTime < 5000)
			{
				FishingFail(ch);
				if (rod)
				{
					event_cancel(&ch->m_pkPreFishingEvent);
					rod->SetSocket(2, 0, true, "SOCKET_FISHING");
				}
			}

			TPacketGCFishing p;
			p.header = HEADER_GC_FISHING;
			p.subheader = FISHING_SUBHEADER_GC_STOP;
			p.info = ch->GetVID();
			ch->PacketAround(&p, sizeof(p));

			event_cancel(&ch->m_pkFishingEvent);
			SaveEndFishingTime(ch);
			event_cancel(&ch->m_pkPreFishingEvent);
			return;
		}
	}

	void Take(fishing_event_info* info, LPCHARACTER ch)
	{
		if (ch && ch->IsPlayingFishGame() && ch->m_biFishGameState >= FISHING_GAME_STATE_IN_PROGRESS) {
			ch->IncrementPlayerFishGamePosition(8);

			TPacketGCFishingGame p;
			p.header = HEADER_GC_FISHING_GAME;
			p.subheader = FISHING_GAME_SUBHEADER_GC_PLAYER_UPDATE;
			p.size = sizeof(TPacketGCFishingGame) + sizeof(TPacketGCFishingGamePlayerUpdate);

			TPacketGCFishingGamePlayerUpdate packSub;
			packSub.player_pos = ch->m_iFish_position;
			LPDESC d = ch->GetDesc();
			if (d)
			{
				d->BufferedPacket(&p, sizeof(TPacketGCFishingGame));
				d->Packet(&packSub, sizeof(TPacketGCFishingGamePlayerUpdate));
			}
			return;
		}
	}

	void Grill(LPCHARACTER ch, LPITEM item)
	{
		/*if (item->GetVnum() < fish_info[3].vnum)
		  return;
		  int idx = item->GetVnum()-fish_info[3].vnum+3;
		  if (idx>=MAX_FISH)
		  idx = item->GetVnum()-fish_info[3].dead_vnum+3;
		  if (idx>=MAX_FISH)
		  return;*/
		int idx = -1;
		DWORD vnum = item->GetVnum();
		if (vnum >= 27803 && vnum <= 27830)
			idx = 60;
		if (vnum >= 27833 && vnum <= 27860)
			idx = 30;

		if (idx == -1)
			return;

		int count = item->GetCount();

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are roasting %s over the fire."), item->GetName());
		item->SetCount(0);
		ch->AutoGiveItem(vnum + idx, count);
	}

	bool RefinableRod(LPITEM rod)
	{
		if (rod->GetType() != ITEM_ROD)
			return false;

		if (rod->IsEquipped())
			return false;

		return (rod->GetSocket(0) == rod->GetValue(2));
	}

	int RealRefineRod(LPCHARACTER ch, LPITEM item)
	{
		if (!ch || !item)
			return 2;

		// REFINE_ROD_HACK_BUG_FIX
		if (!RefinableRod(item))
		{
			sys_err("REFINE_ROD_HACK pid(%u) item(%s:%d)", ch->GetPlayerID(), item->GetName(), item->GetID());

			LogManager::instance().RefineLog(ch->GetPlayerID(), item->GetName(), item->GetID(), -1, 1, "ROD_HACK");

			return 2;
		}
		// END_OF_REFINE_ROD_HACK_BUG_FIX

		LPITEM rod = item;

		int iAdv = rod->GetValue(0) / 10;

		if (number(1, 100) <= rod->GetValue(3))
		{
			LogManager::instance().RefineLog(ch->GetPlayerID(), rod->GetName(), rod->GetID(), iAdv, 1, "ROD");

			LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(rod->GetRefinedVnum(), 1);

			if (pkNewItem)
			{
				BYTE bCell = rod->GetCell();
				// ³¬½Ã´ë °³·® ¼º°ø
				ITEM_MANAGER::instance().RemoveItem(rod, "REMOVE (REFINE FISH_ROD)");
				pkNewItem->AddToCharacter(ch, TItemPos(INVENTORY, bCell));
				LogManager::instance().ItemLog(ch, pkNewItem, "REFINE FISH_ROD SUCCESS", pkNewItem->GetName());
				return 1;
			}

			// ³¬½Ã´ë °³·® ½ÇÆÐ
			return 2;
		}
		else
		{
			LogManager::instance().RefineLog(ch->GetPlayerID(), rod->GetName(), rod->GetID(), iAdv, 0, "ROD");

			LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(rod->GetValue(4), 1);
			if (pkNewItem)
			{
				BYTE bCell = rod->GetCell();
				// ³¬½Ã´ë °³·®¿¡ ¼º°ø
				ITEM_MANAGER::instance().RemoveItem(rod, "REMOVE (REFINE FISH_ROD)");
				pkNewItem->AddToCharacter(ch, TItemPos(INVENTORY, bCell));
				LogManager::instance().ItemLog(ch, pkNewItem, "REFINE FISH_ROD FAIL", pkNewItem->GetName());
				return 0;
			}

			// ³¬½Ã´ë °³·® ½ÇÆÐ
			return 2;
		}
	}
#endif
}

#ifdef __FISHING_MAIN__
int main(int argc, char** argv)
{
	//srandom(time(0) + getpid());
	srandomdev();
	/*
	   struct SFishInfo
	   {
	   const char* name;

	   DWORD vnum;
	   DWORD dead_vnum;
	   DWORD grill_vnum;

	   int prob[3];
	   int difficulty;

	   int limit_type;
	   int limits[3];

	   int time_type;
	   int length_range[3]; // MIN MAX EXTRA_MAX : 99% MIN~MAX, 1% MAX~EXTRA_MAX

	   int used_table[NUM_USE_RESULT_COUNT];
	// 6000 2000 1000 500 300 100 50 30 10 5 4 1
	};
	 */
	using namespace fishing;

	Initialize();

	for (int i = 0; i < MAX_FISH; ++i)
	{
		printf("%s\t%u\t%u\t%u\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
			fish_info[i].name,
			fish_info[i].vnum,
			fish_info[i].dead_vnum,
			fish_info[i].grill_vnum,
			fish_info[i].prob[0],
			fish_info[i].prob[1],
			fish_info[i].prob[2],
			fish_info[i].difficulty,
			fish_info[i].time_type,
			fish_info[i].length_range[0],
			fish_info[i].length_range[1],
			fish_info[i].length_range[2]);

		for (int j = 0; j < NUM_USE_RESULT_COUNT; ++j)
			printf("\t%d", fish_info[i].used_table[j]);

		puts("");
	}

	return 1;
}

#endif

// Files shared by GameCore.top
