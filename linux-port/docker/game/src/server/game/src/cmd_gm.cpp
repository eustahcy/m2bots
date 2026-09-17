#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#ifdef ENABLE_IKASHOP_RENEWAL
#include "ikarus_shop.h"
#include "ikarus_shop_manager.h"
#endif
#include "sectree_manager.h"
#include "mob_manager.h"
#include "packet.h"
#include "cmd.h"
#include "regen.h"
#include "guild.h"
#include "guild_manager.h"
#include "p2p.h"
#include "buffer_manager.h"
#include "fishing.h"
#include "mining.h"
#include "questmanager.h"
#include "vector.h"
#include "affect.h"
#include "db.h"
#include "priv_manager.h"
#include "building.h"
#include "battle.h"
#include "arena.h"
#include "start_position.h"
#include "party.h"
#include "monarch.h"
#include "castle.h"
#include "xmas_event.h"
#include "log.h"
#include "threeway_war.h"
#include "unique_item.h"
#include "DragonSoul.h"
#include "special_spawn.h"
#include "SpecialSpawnManager.h"
#include "special_shop_manager.h"
#include "desc.h"
#include "playerbot_manager.h"
#include "../../common/CommonDefines.h"
#include "BanManager.h"
#include "safebox.h"

extern bool DropEvent_RefineBox_SetValue(const std::string& name, int value);

// ADD_COMMAND_SLOW_STUN
enum
{
	COMMANDAFFECT_STUN,
	COMMANDAFFECT_SLOW,
};

void Command_ApplyAffect(LPCHARACTER ch, const char* argument, const char* affectName, int cmdAffect)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	sys_log(0, arg1);

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: %s <name>", affectName);
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);
	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s is not in same map", arg1);
		return;
	}

	switch (cmdAffect)
	{
		case COMMANDAFFECT_STUN:
			SkillAttackAffect(ch, tch, 1000, IMMUNE_STUN, AFFECT_STUN, POINT_NONE, 0, AFF_STUN, 30, "GM_STUN");
			break;
		case COMMANDAFFECT_SLOW:
			SkillAttackAffect(ch, tch, 1000, IMMUNE_SLOW, AFFECT_SLOW, POINT_MOV_SPEED, -30, AFF_SLOW, 30, "GM_SLOW");
			break;
	}

	sys_log(0, "%s %s", arg1, affectName);

	ch->ChatPacket(CHAT_TYPE_INFO, "%s %s", arg1, affectName);
}
// END_OF_ADD_COMMAND_SLOW_STUN

ACMD(do_playerbot_spawn)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: bot_spawn <player_id> <empire: 1-3>");
		return;
	}

	DWORD dwPlayerID = 0;
	int iEmpire = 0;
	str_to_number(dwPlayerID, arg1);
	str_to_number(iEmpire, arg2);

	if (dwPlayerID == 0 || iEmpire <= 0 || iEmpire >= EMPIRE_MAX_NUM)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Invalid player id or empire (use 1, 2 or 3).");
		return;
	}

	if (!CPlayerBotManager::instance().Spawn(dwPlayerID, static_cast<BYTE>(iEmpire)))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Cannot spawn playerbot %u (already active or invalid).", dwPlayerID);
		return;
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "Playerbot %u load requested. Active/pending: %u",
			dwPlayerID, static_cast<unsigned int>(CPlayerBotManager::instance().GetCount()));
}

ACMD(do_playerbot_despawn)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: bot_despawn <player_id>");
		return;
	}

	DWORD dwPlayerID = 0;
	str_to_number(dwPlayerID, arg1);

	if (!CPlayerBotManager::instance().Despawn(dwPlayerID))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Playerbot %u is not active.", dwPlayerID);
		return;
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "Playerbot %u despawned. Active/pending: %u",
			dwPlayerID, static_cast<unsigned int>(CPlayerBotManager::instance().GetCount()));
}

ACMD(do_playerbot_spawn_many)
{
	char arg1[256], arg2[256], arg3[256];
	one_argument(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));

	DWORD dwFirstPlayerID = 0;
	int iCount = 0;
	int iEmpire = 0;
	str_to_number(dwFirstPlayerID, arg1);
	str_to_number(iCount, arg2);
	str_to_number(iEmpire, arg3);

	if (dwFirstPlayerID == 0 || iCount <= 0 || iCount > 500 || iEmpire <= 0 || iEmpire >= EMPIRE_MAX_NUM)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: bot_spawn_many <first_player_id> <count: 1-500> <empire: 1-3>");
		return;
	}

	int iStarted = 0;
	for (int i = 0; i < iCount; ++i)
		if (CPlayerBotManager::instance().Spawn(dwFirstPlayerID + i, static_cast<BYTE>(iEmpire)))
			++iStarted;

	ch->ChatPacket(CHAT_TYPE_INFO, "Playerbot range %u-%u: requested %d, started %d, active/pending %u.",
			dwFirstPlayerID, dwFirstPlayerID + iCount - 1, iCount, iStarted,
			static_cast<unsigned int>(CPlayerBotManager::instance().GetCount()));
}

ACMD(do_playerbot_despawn_many)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	DWORD dwFirstPlayerID = 0;
	int iCount = 0;
	str_to_number(dwFirstPlayerID, arg1);
	str_to_number(iCount, arg2);

	if (dwFirstPlayerID == 0 || iCount <= 0 || iCount > 500)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: bot_despawn_many <first_player_id> <count: 1-500>");
		return;
	}

	int iStopped = 0;
	for (int i = 0; i < iCount; ++i)
		if (CPlayerBotManager::instance().Despawn(dwFirstPlayerID + i))
			++iStopped;

	ch->ChatPacket(CHAT_TYPE_INFO, "Playerbot range %u-%u: requested %d, stopped %d, active/pending %u.",
			dwFirstPlayerID, dwFirstPlayerID + iCount - 1, iCount, iStopped,
			static_cast<unsigned int>(CPlayerBotManager::instance().GetCount()));
}

ACMD(do_playerbot_rank)
{
	struct TBotRankEntry
	{
		DWORD pid;
		std::string name;
		BYTE level;
		long x;
		long y;
		bool inPT;

		bool operator < (const TBotRankEntry& other) const
		{
			return level > other.level;
		}
	};

	std::vector<TBotRankEntry> ranks;
	for (DWORD pid = 1; pid <= 4000; ++pid)
	{
		LPCHARACTER bot = CHARACTER_MANAGER::instance().FindByPID(pid);
		if (bot && CPlayerBotManager::instance().IsManaged(pid))
		{
			TBotRankEntry e;
			e.pid = pid;
			e.name = bot->GetName();
			e.level = bot->GetLevel();
			e.x = bot->GetX();
			e.y = bot->GetY();
			e.inPT = (bot->GetParty() != NULL);
			ranks.push_back(e);
		}
	}

	std::sort(ranks.begin(), ranks.end());

	ch->ChatPacket(CHAT_TYPE_INFO, "=== TOP 10 ACTIVE PLAYERBOTS (Total Active: %u) ===", static_cast<unsigned int>(ranks.size()));
	for (size_t i = 0; i < std::min((size_t)10, ranks.size()); ++i)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "#%u %s (Lv %u) - Pos: (%ld, %ld) %s",
				static_cast<unsigned int>(i + 1), ranks[i].name.c_str(), ranks[i].level, ranks[i].x, ranks[i].y, ranks[i].inPT ? "[PT]" : "[Solo]");
	}
}

ACMD(do_stun)
{
	Command_ApplyAffect(ch, argument, "stun", COMMANDAFFECT_STUN);
}

ACMD(do_slow)
{
	Command_ApplyAffect(ch, argument, "slow", COMMANDAFFECT_SLOW);
}

ACMD(do_transfer)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: transfer <name>");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);
	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			if (g_bChannel < 99 && pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "Target(%s) is in %d channel (my channel %d)", arg1, pkCCI->bChannel, g_bChannel);
				return;
			}

			// Playerbot: a bot on another core stays there - it cannot stand
			// on a map its own core does not host, and a WarpSet only takes it
			// off its sectree.
			if (CPlayerBotManager::instance().IsRegisteredBotPID(pkCCI->dwPID))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "Bot %s jest na innym rdzeniu (mapa %ld) i nie przejdzie na mape tego rdzenia.", arg1, pkCCI->lMapIndex);
				return;
			}

			TPacketGGTransfer pgg;

			pgg.bHeader = HEADER_GG_TRANSFER;
			strlcpy(pgg.szName, arg1, sizeof(pgg.szName));
			pgg.lX = ch->GetX();
			pgg.lY = ch->GetY();

			P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGTransfer));
			ch->ChatPacket(CHAT_TYPE_INFO, "Transfer requested.");
		}
		else
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no character(%s) by that name", arg1);

		return;
	}

	if (ch == tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Transfer me?!?");
		return;
	}

	//tch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
	// Playerbot: a bot has no client to reconnect, so it changes map the
	// way its own AI changes every other one.
	if (tch->GetDesc() && tch->GetDesc()->IsBot())
	{
		CPlayerBotManager::instance().TransferBot(tch, ch);
		return;
	}
	tch->WarpSet(ch->GetX(), ch->GetY(), ch->GetMapIndex());
}

// LUA_ADD_GOTO_INFO
struct GotoInfo
{
	std::string 	st_name;

	BYTE 	empire;
	int 	mapIndex;
	DWORD 	x, y;

	GotoInfo()
	{
		st_name 	= "";
		empire 		= 0;
		mapIndex 	= 0;

		x = 0;
		y = 0;
	}
	GotoInfo(const GotoInfo& c_src)
	{
		__copy__(c_src);
	}
	void operator = (const GotoInfo& c_src)
	{
		__copy__(c_src);
	}
	void __copy__(const GotoInfo& c_src)
	{
		st_name 	= c_src.st_name;
		empire 		= c_src.empire;
		mapIndex 	= c_src.mapIndex;

		x = c_src.x;
		y = c_src.y;
	}
};

static std::vector<GotoInfo> gs_vec_gotoInfo;

void CHARACTER_AddGotoInfo(const std::string& c_st_name, BYTE empire, int mapIndex, DWORD x, DWORD y)
{
	GotoInfo newGotoInfo;
	newGotoInfo.st_name = c_st_name;
	newGotoInfo.empire = empire;
	newGotoInfo.mapIndex = mapIndex;
	newGotoInfo.x = x;
	newGotoInfo.y = y;
	gs_vec_gotoInfo.emplace_back(newGotoInfo);

	sys_log(0, "AddGotoInfo(name=%s, empire=%d, mapIndex=%d, pos=(%d, %d))", c_st_name.c_str(), empire, mapIndex, x, y);
}

bool FindInString(const char * c_pszFind, const char * c_pszIn)
{
	const char * c = c_pszIn;
	const char * p;

	p = strchr(c, '|');

	if (!p)
		return (0 == strncasecmp(c_pszFind, c_pszIn, strlen(c_pszFind)));
	else
	{
		char sz[64 + 1];

		do
		{
			strlcpy(sz, c, MIN(sizeof(sz), (p - c) + 1));

			if (!strncasecmp(c_pszFind, sz, strlen(c_pszFind)))
				return true;

			c = p + 1;
		} while ((p = strchr(c, '|')));

		strlcpy(sz, c, sizeof(sz));

		if (!strncasecmp(c_pszFind, sz, strlen(c_pszFind)))
			return true;
	}

	return false;
}

bool CHARACTER_GoToName(LPCHARACTER ch, BYTE empire, int mapIndex, const char* gotoName)
{
	std::vector<GotoInfo>::iterator i;
	for (i = gs_vec_gotoInfo.begin(); i != gs_vec_gotoInfo.end(); ++i)
	{
		const GotoInfo& c_eachGotoInfo = *i;

		if (mapIndex != 0)
		{
			if (mapIndex != c_eachGotoInfo.mapIndex)
				continue;
		}
		else if (!FindInString(gotoName, c_eachGotoInfo.st_name.c_str()))
			continue;

		if (c_eachGotoInfo.empire == 0 || c_eachGotoInfo.empire == empire)
		{
			int x = c_eachGotoInfo.x * 100;
			int y = c_eachGotoInfo.y * 100;

			ch->ChatPacket(CHAT_TYPE_INFO, "You warp to ( %d, %d )", x, y);
			ch->WarpSet(x, y);
			ch->Stop();
			return true;
		}
	}
	return false;
}

// END_OF_LUA_ADD_GOTO_INFO

ACMD(do_goto)
{
	char arg1[256], arg2[256];
	int x = 0, y = 0, z = 0;

	bool isTutor = ch->GetGMLevel() == GM_LOW_WIZARD;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 && !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: goto <x meter> <y meter>");
		return;
	}

	if (isnhdigit(*arg1) && isnhdigit(*arg2))
	{
		if (isTutor)
			return;

		str_to_number(x, arg1);
		str_to_number(y, arg2);

		PIXEL_POSITION p;

		if (SECTREE_MANAGER::instance().GetMapBasePosition(ch->GetX(), ch->GetY(), p))
		{
			x += p.x / 100;
			y += p.y / 100;
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "You goto ( %d, %d )", x, y);
	}
	else
	{
		int mapIndex = 0;
		BYTE empire = 0;


		if (*arg1 == '#')
			str_to_number(mapIndex,  (arg1 + 1));

		if (*arg2 && isnhdigit(*arg2))
		{
			if (isTutor)
				return;

			str_to_number(empire, arg2);
			empire = MINMAX(1, empire, 3);
		}
		else
			empire = ch->GetEmpire();

		if (isTutor)
		{
			if (!(strcmp(arg1, "a1") == 0 ||
				strcmp(arg1, "b1") == 0 ||
				strcmp(arg1, "c1") == 0))
				return;
		}

		if (CHARACTER_GoToName(ch, empire, mapIndex, arg1))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Cannot find map command syntax: /goto <mapname> [empire]");
			return;
		}

		return;

		/*
		   int iMapIndex = 0;
		   for (int i = 0; aWarpInfo[i].c_pszName != NULL; ++i)
		   {
		   if (iMapIndex != 0)
		   {
		   if (iMapIndex != aWarpInfo[i].iMapIndex)
		   continue;
		   }
		   else if (!FindInString(arg1, aWarpInfo[i].c_pszName))
		   continue;

		   if (aWarpInfo[i].bEmpire == 0 || aWarpInfo[i].bEmpire == bEmpire)
		   {
		   x = aWarpInfo[i].x * 100;
		   y = aWarpInfo[i].y * 100;

		   ch->ChatPacket(CHAT_TYPE_INFO, "You warp to ( %d, %d )", x, y);
		   ch->WarpSet(x, y);
		   ch->Stop();
		   return;
		   }
		   }
		 */

	}

	x *= 100;
	y *= 100;

	ch->Show(ch->GetMapIndex(), x, y, z);
	ch->Stop();
}

ACMD(do_warp)
{
	char arg1[256], arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: warp <character name> | <x meter> <y meter>");
		return;
	}

	int x = 0, y = 0;
#ifdef ENABLE_CMD_WARP_IN_DUNGEON
	int mapIndex = 0;
#endif

	if (isnhdigit(*arg1) && isnhdigit(*arg2))
	{
		str_to_number(x, arg1);
		str_to_number(y, arg2);
	}
	else
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

		if (NULL == tch)
		{
			const CCI* pkCCI = P2P_MANAGER::instance().Find(arg1);

			if (NULL != pkCCI)
			{
				if (ch->GetGMLevel() == GM_LOW_WIZARD)
				{
					if (!(pkCCI->lMapIndex == 1 || pkCCI->lMapIndex == 21 || pkCCI->lMapIndex == 41))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, "Wskazana postac nie jest w M1.");
						return;
					}
				}

				ch->WarpToPID( pkCCI->dwPID );
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "There is no one(%s) by that name", arg1);
			}

			return;
		}
		else
		{
			x = tch->GetX() / 100;
			y = tch->GetY() / 100;
#ifdef ENABLE_CMD_WARP_IN_DUNGEON
			mapIndex = tch->GetMapIndex();
#endif

			if (ch->GetGMLevel() == GM_LOW_WIZARD)
			{
				if (!(mapIndex == 1 || mapIndex == 21 || mapIndex == 41))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "Wskazana postac nie jest w M1.");
					return;
				}
			}
		}
	}

	x *= 100;
	y *= 100;

#ifdef ENABLE_CMD_WARP_IN_DUNGEON
	ch->ChatPacket(CHAT_TYPE_INFO, "You warp to ( %d, %d, %d )", x, y, mapIndex);
	ch->WarpSet(x, y, mapIndex);
#else
	ch->ChatPacket(CHAT_TYPE_INFO, "You warp to ( %d, %d )", x, y);
	ch->WarpSet(x, y);
#endif
	ch->Stop();
}

#ifdef ENABLE_NEWSTUFF
ACMD(do_rewarp)
{
	ch->ChatPacket(CHAT_TYPE_INFO, "You warp to ( %d, %d )", ch->GetX(), ch->GetY());
	ch->WarpSet(ch->GetX(), ch->GetY());
	ch->Stop();
}
#endif

ACMD(do_item)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: item <item vnum>");
		return;
	}

	DWORD dwVnum;

	if (isnhdigit(*arg1))
		str_to_number(dwVnum, arg1);
	else
	{
		if (!ITEM_MANAGER::instance().GetVnum(arg1, dwVnum))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "#%u item not exist by that vnum(%s).", dwVnum, arg1);
			return;
		}
	}

	int iCount = 1;

	if (*arg2)
	{
		str_to_number(iCount, arg2);

		int MAX_STACK = ITEM_MAX_COUNT;
		TItemTable* pItemTable = ITEM_MANAGER::instance().GetTable(dwVnum);
		if (pItemTable)
		{
			MAX_STACK = pItemTable->dwMaxStack;
		}

		iCount = MINMAX(1, iCount, MAX_STACK);
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwVnum, iCount, 0, true);

	if (item)
	{
		int iEmptyPos = ch->GetEmptyInventoryEx(item);
		if (iEmptyPos != -1)
		{
			item->AddToCharacter(ch, TItemPos(item->GetWindowInventoryEx(), iEmptyPos));
			LogManager::instance().ItemLog(ch, item, "GM", item->GetName());
		}
		else
		{
			M2_DESTROY_ITEM(item);
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Not enough inventory space."));
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "#%u item not exist by that vnum(%s).", dwVnum, arg1);
	}
}

ACMD(do_group_random)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: grrandom <group vnum>");
		return;
	}

	DWORD dwVnum = 0;
	str_to_number(dwVnum, arg1);
	CHARACTER_MANAGER::instance().SpawnGroupGroup(dwVnum, ch->GetMapIndex(), ch->GetX() - 500, ch->GetY() - 500, ch->GetX() + 500, ch->GetY() + 500);
}

ACMD(do_group)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: group <group vnum>");
		return;
	}

	DWORD dwVnum = 0;
	str_to_number(dwVnum, arg1);

	if (test_server)
		sys_log(0, "COMMAND GROUP SPAWN %u at %u %u %u", dwVnum, ch->GetMapIndex(), ch->GetX(), ch->GetY());

	CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, ch->GetMapIndex(), ch->GetX() - 500, ch->GetY() - 500, ch->GetX() + 500, ch->GetY() + 500);
}

ACMD(do_mob_coward)
{
	char	arg1[256], arg2[256];
	DWORD	vnum = 0;
	LPCHARACTER	tch;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mc <vnum>");
		return;
	}

	const CMob * pkMob;

	if (isdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	if (vnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such mob(%s) by that vnum", arg1);
		return;
	}

	int iCount = 0;

	if (*arg2)
		str_to_number(iCount, arg2);
	else
		iCount = 1;

	iCount = MIN(20, iCount);

	while (iCount--)
	{
		tch = CHARACTER_MANAGER::instance().SpawnMobRange(vnum,
				ch->GetMapIndex(),
				ch->GetX() - number(200, 750),
				ch->GetY() - number(200, 750),
				ch->GetX() + number(200, 750),
				ch->GetY() + number(200, 750),
				true,
				pkMob->m_table.bType == CHAR_TYPE_STONE);
		if (tch)
			tch->SetCoward();
	}
}

ACMD(do_mob_map)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: mm <vnum>");
		return;
	}

	DWORD vnum = 0;
	str_to_number(vnum, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRandomPosition(vnum, ch->GetMapIndex());

	if (tch)
		ch->ChatPacket(CHAT_TYPE_INFO, "%s spawned in %dx%d", tch->GetName(), tch->GetX(), tch->GetY());
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Spawn failed.");
}

ACMD(do_mob_aggresive)
{
	char	arg1[256], arg2[256];
	DWORD	vnum = 0;
	LPCHARACTER	tch;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mob <mob vnum>");
		return;
	}

	const CMob * pkMob;

	if (isdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	if (vnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such mob(%s) by that vnum", arg1);
		return;
	}

	int iCount = 0;

	if (*arg2)
		str_to_number(iCount, arg2);
	else
		iCount = 1;

	iCount = MIN(20, iCount);

	while (iCount--)
	{
		tch = CHARACTER_MANAGER::instance().SpawnMobRange(vnum,
				ch->GetMapIndex(),
				ch->GetX() - number(200, 750),
				ch->GetY() - number(200, 750),
				ch->GetX() + number(200, 750),
				ch->GetY() + number(200, 750),
				true,
				pkMob->m_table.bType == CHAR_TYPE_STONE);
		if (tch)
			tch->SetAggressive();
	}
}

ACMD(do_mob)
{
	char	arg1[256], arg2[256];
	DWORD	vnum = 0;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mob <mob vnum>");
		return;
	}

	const CMob* pkMob = NULL;

	if (isnhdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	if (vnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such mob(%s) by that vnum", arg1);
		return;
	}

	int iCount = 0;

	if (*arg2)
		str_to_number(iCount, arg2);
	else
		iCount = 1;

	if (test_server)
		iCount = MIN(40, iCount);
	else
		iCount = MIN(20, iCount);

	while (iCount--)
	{
		CHARACTER_MANAGER::instance().SpawnMobRange(vnum,
				ch->GetMapIndex(),
				ch->GetX() - number(200, 750),
				ch->GetY() - number(200, 750),
				ch->GetX() + number(200, 750),
				ch->GetY() + number(200, 750),
				true,
				pkMob->m_table.bType == CHAR_TYPE_STONE);
	}
}

ACMD(do_mob_ld)
{
	char	arg1[256], arg2[256], arg3[256], arg4[256];
	DWORD	vnum = 0;

	two_arguments(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mob <mob vnum>");
		return;
	}

	const CMob* pkMob = NULL;

	if (isnhdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	if (vnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such mob(%s) by that vnum", arg1);
		return;
	}

	int dir = 1;
	long x=0,y=0;

	if (*arg2)
		str_to_number(x, arg2);
	if (*arg3)
		str_to_number(y, arg3);
	if (*arg4)
		str_to_number(dir, arg4);

	CHARACTER_MANAGER::instance().SpawnMob(vnum,
		ch->GetMapIndex(),
		x*100,
		y*100,
		ch->GetZ(),
		pkMob->m_table.bType == CHAR_TYPE_STONE,
		dir);
}

struct FuncPurge
{
	LPCHARACTER m_pkGM;
	bool	m_bAll;

	FuncPurge(LPCHARACTER ch) : m_pkGM(ch), m_bAll(false)
	{
	}

	void operator () (LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER) ent;

		int iDist = DISTANCE_APPROX(pkChr->GetX() - m_pkGM->GetX(), pkChr->GetY() - m_pkGM->GetY());

		if (!m_bAll && iDist >= 1000)
			return;

		sys_log(0, "PURGE: %s %d", pkChr->GetName(), iDist);

		if (pkChr->IsNPC() && !pkChr->IsPet() && pkChr->GetRider() == NULL)
		{
			M2_DESTROY_CHARACTER(pkChr);
		}
	}
};

ACMD(do_purge)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	FuncPurge func(ch);

	if (*arg1 && !strcmp(arg1, "all"))
		func.m_bAll = true;

	LPSECTREE sectree = ch->GetSectree();
	if (sectree) // #431
		sectree->ForEachAround(func);
	else
		sys_err("PURGE_ERROR.NULL_SECTREE(mapIndex=%d, pos=(%d, %d)", ch->GetMapIndex(), ch->GetX(), ch->GetY());
}

#define ENABLE_CMD_IPURGE_EX
ACMD(do_item_purge)
{
	ch->ComputePoints(); //@fixme300
#ifdef ENABLE_CMD_IPURGE_EX
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: ipurge <window>");
		ch->ChatPacket(CHAT_TYPE_INFO, "List of the available windows:");
		ch->ChatPacket(CHAT_TYPE_INFO, " all");
		ch->ChatPacket(CHAT_TYPE_INFO, " inventory or inv");
		ch->ChatPacket(CHAT_TYPE_INFO, " equipment or equip");
		ch->ChatPacket(CHAT_TYPE_INFO, " dragonsoul or ds");
		ch->ChatPacket(CHAT_TYPE_INFO, " belt");
		return;
	}

	int i{};
	LPITEM item{};
	std::string strArg(arg1);
	if (!strArg.compare(0, 3, "all"))
	{
		for (i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
		{
			if ((item = ch->GetInventoryItem(i)))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
				ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, 255);
			}
		}
		for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
		{
			if ((item = ch->GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i ))))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			}
		}
	}
	else if (!strArg.compare(0, 3, "inv"))
	{
		for (i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			if ((item = ch->GetInventoryItem(i)))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
				ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, 255);
			}
		}
	}
	else if (!strArg.compare(0, 5, "equip"))
	{
		for (i = 0; i < WEAR_MAX_NUM; ++i)
		{
			if ((item = ch->GetInventoryItem(INVENTORY_MAX_NUM + i)))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
				ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, INVENTORY_MAX_NUM + i, 255);
			}
		}
	}
	else if (!strArg.compare(0, 6, "dragon") || !strArg.compare(0, 2, "ds"))
	{
		for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
		{
			if ((item = ch->GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i ))))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			}
		}
	}
	else if (!strArg.compare(0, 4, "belt"))
	{
		for (i = 0; i < BELT_INVENTORY_SLOT_COUNT; ++i)
		{
			if ((item = ch->GetInventoryItem(BELT_INVENTORY_SLOT_START + i)))
			{
				ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
				ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, BELT_INVENTORY_SLOT_START + i, 255);
			}
		}
	}
#else
	int i{};
	LPITEM item{};
	for (i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if ((item = ch->GetInventoryItem(i)))
		{
			ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, 255);
		}
	}
	for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = ch->GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i ))))
		{
			ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
		}
	}
#endif
	ch->ComputePoints(); //@fixme300
}

ACMD(do_state)
{
	char arg1[256];
	LPCHARACTER tch;

	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		if (arg1[0] == '#')
		{
			tch = CHARACTER_MANAGER::instance().Find(strtoul(arg1+1, NULL, 10));
		}
		else
		{
			LPDESC d = DESC_MANAGER::instance().FindByCharacterName(arg1);

			if (!d)
				tch = NULL;
			else
				tch = d->GetCharacter();
		}
	}
	else
		tch = ch;

	if (!tch)
		return;

	char buf[256];

	snprintf(buf, sizeof(buf), "%s's State: ", tch->GetName());

	if (tch->IsPosition(POS_FIGHTING))
		strlcat(buf, "Battle", sizeof(buf));
	else if (tch->IsPosition(POS_DEAD))
		strlcat(buf, "Dead", sizeof(buf));
	else
		strlcat(buf, "Standing", sizeof(buf));

	if (ch->GetShop())
		strlcat(buf, ", Shop", sizeof(buf));

	if (ch->GetExchange())
		strlcat(buf, ", Exchange", sizeof(buf));

	ch->ChatPacket(CHAT_TYPE_INFO, "%s", buf);

	int len = snprintf(buf, sizeof(buf), "Coordinate %ldx%ld (%ldx%ld)",
			tch->GetX(), tch->GetY(), tch->GetX() / 100, tch->GetY() / 100);

	len = snprintf(buf, sizeof(buf), "Hostname %s Channel %u (port %u)", g_stHostname.c_str(), g_bChannel, mother_port);

	if (len < 0 || len >= (int) sizeof(buf))
		len = sizeof(buf) - 1;

	LPSECTREE pSec = SECTREE_MANAGER::instance().Get(tch->GetMapIndex(), tch->GetX(), tch->GetY());
	if (pSec)
	{
		TMapSetting& map_setting = SECTREE_MANAGER::instance().GetMap(tch->GetMapIndex())->m_setting;
		snprintf(buf + len, sizeof(buf) - len, " MapIndex %ld Attribute %08X Local Position (%ld x %ld)",
			tch->GetMapIndex(), pSec->GetAttribute(tch->GetX(), tch->GetY()), (tch->GetX() - map_setting.iBaseX)/100, (tch->GetY() - map_setting.iBaseY)/100);
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "%s", buf);

	ch->ChatPacket(CHAT_TYPE_INFO, "LEV %d", tch->GetLevel());
	ch->ChatPacket(CHAT_TYPE_INFO, "HP %d/%d", tch->GetHP(), tch->GetMaxHP());
	ch->ChatPacket(CHAT_TYPE_INFO, "SP %d/%d", tch->GetSP(), tch->GetMaxSP());
	ch->ChatPacket(CHAT_TYPE_INFO, "ATT %d MAGIC_ATT %d SPD %d CRIT %d%% PENE %d%% ATT_BONUS %d%%",
			tch->GetPoint(POINT_ATT_GRADE),
			tch->GetPoint(POINT_MAGIC_ATT_GRADE),
			tch->GetPoint(POINT_ATT_SPEED),
			tch->GetPoint(POINT_CRITICAL_PCT),
			tch->GetPoint(POINT_PENETRATE_PCT),
			tch->GetPoint(POINT_ATT_BONUS));
	ch->ChatPacket(CHAT_TYPE_INFO, "DEF %d MAGIC_DEF %d BLOCK %d%% DODGE %d%% DEF_BONUS %d%%",
			tch->GetPoint(POINT_DEF_GRADE),
			tch->GetPoint(POINT_MAGIC_DEF_GRADE),
			tch->GetPoint(POINT_BLOCK),
			tch->GetPoint(POINT_DODGE),
			tch->GetPoint(POINT_DEF_BONUS));
	#ifdef ENABLE_MOUNT_COSTUME_EX_SYSTEM
	ch->ChatPacket(CHAT_TYPE_INFO, "MOUNT %d", tch->GetPoint(POINT_MOUNT));
	#endif

	ch->ChatPacket(CHAT_TYPE_INFO, "RESISTANCES:");
	ch->ChatPacket(CHAT_TYPE_INFO, "   WARR:%3d%% ASAS:%3d%% SURA:%3d%% SHAM:%3d%%"
			#ifdef ENABLE_WOLFMAN_CHARACTER
			" WOLF:%3d%%"
			#endif
			" HUMAN:%3d%%"
			,
			tch->GetPoint(POINT_RESIST_WARRIOR),
			tch->GetPoint(POINT_RESIST_ASSASSIN),
			tch->GetPoint(POINT_RESIST_SURA),
			tch->GetPoint(POINT_RESIST_SHAMAN),
			#ifdef ENABLE_WOLFMAN_CHARACTER
			tch->GetPoint(POINT_RESIST_WOLFMAN),
			#endif
			tch->GetPoint(POINT_RESIST_HUMAN)
	);
	ch->ChatPacket(CHAT_TYPE_INFO, "   SWORD:%3d%% THSWORD:%3d%% DAGGER:%3d%% BELL:%3d%% FAN:%3d%% BOW:%3d%%"
			#ifdef ENABLE_WOLFMAN_CHARACTER
			" CLAW:%3d%%"
			#endif
			,
			tch->GetPoint(POINT_RESIST_SWORD),
			tch->GetPoint(POINT_RESIST_TWOHAND),
			tch->GetPoint(POINT_RESIST_DAGGER),
			tch->GetPoint(POINT_RESIST_BELL),
			tch->GetPoint(POINT_RESIST_FAN),
			tch->GetPoint(POINT_RESIST_BOW)
			#ifdef ENABLE_WOLFMAN_CHARACTER
			,tch->GetPoint(POINT_RESIST_CLAW)
			#endif
	);

	ch->ChatPacket(CHAT_TYPE_INFO, "   ELEC:%3d%% FIRE:%3d%% ICE:%3d%% WIND:%3d%% EARTH:%3d%% DARK:%3d%%",
			tch->GetPoint(POINT_RESIST_ELEC),
			tch->GetPoint(POINT_RESIST_FIRE),
			tch->GetPoint(POINT_RESIST_ICE),
			tch->GetPoint(POINT_RESIST_WIND),
			tch->GetPoint(POINT_RESIST_EARTH),
			tch->GetPoint(POINT_RESIST_DARK));

	ch->ChatPacket(CHAT_TYPE_INFO, "   MAGIC:%3d%% CRIT:%3d%% PENE:%3d%%",
			tch->GetPoint(POINT_RESIST_MAGIC),
			tch->GetPoint(POINT_RESIST_CRITICAL),
			tch->GetPoint(POINT_RESIST_PENETRATE)
	);

	ch->ChatPacket(CHAT_TYPE_INFO, "   INSECT:%3d%% DESERT:%3d%%",
			tch->GetPoint(POINT_ATTBONUS_INSECT),
			tch->GetPoint(POINT_ATTBONUS_DESERT));

#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
	ch->ChatPacket(CHAT_TYPE_INFO, "   MAGIC REDUCTION:%3d%%", tch->GetPoint(POINT_RESIST_MAGIC_REDUCTION));
#endif

	//ch->ChatPacket(CHAT_TYPE_INFO, "ENCHANT:");
	//ch->ChatPacket(CHAT_TYPE_INFO, "   ELEC:%3d%% FIRE:%3d%% ICE:%3d%% WIND:%3d%% EARTH:%3d%% DARK:%3d%%",
	//		tch->GetPoint(POINT_ENCHANT_ELECT),
	//		tch->GetPoint(POINT_ENCHANT_FIRE),
	//		tch->GetPoint(POINT_ENCHANT_ICE),
	//		tch->GetPoint(POINT_ENCHANT_WIND),
	//		tch->GetPoint(POINT_ENCHANT_EARTH),
	//		tch->GetPoint(POINT_ENCHANT_DARK));

	ch->ChatPacket(CHAT_TYPE_INFO, "MALL:");
	ch->ChatPacket(CHAT_TYPE_INFO, "   ATT:%3d%% DEF:%3d%% EXP:%3d%% ITEMx%d GOLDx%d",
			tch->GetPoint(POINT_MALL_ATTBONUS),
			tch->GetPoint(POINT_MALL_DEFBONUS),
			tch->GetPoint(POINT_MALL_EXPBONUS),
			tch->GetPoint(POINT_MALL_ITEMBONUS) / 10,
			tch->GetPoint(POINT_MALL_GOLDBONUS) / 10);

	ch->ChatPacket(CHAT_TYPE_INFO, "BONUS:");
	ch->ChatPacket(CHAT_TYPE_INFO, "   SKILL:%3d%% NORMAL:%3d%% SKILL_DEF:%3d%% NORMAL_DEF:%3d%%",
			tch->GetPoint(POINT_SKILL_DAMAGE_BONUS),
			tch->GetPoint(POINT_NORMAL_HIT_DAMAGE_BONUS),
			tch->GetPoint(POINT_SKILL_DEFEND_BONUS),
			tch->GetPoint(POINT_NORMAL_HIT_DEFEND_BONUS));

	ch->ChatPacket(CHAT_TYPE_INFO, "ATTBONUS:");
	ch->ChatPacket(CHAT_TYPE_INFO, "   HUMAN:%3d%% ANIMAL:%3d%% ORC:%3d%% MILGYO:%3d%% UNDEAD:%3d%%",
			tch->GetPoint(POINT_ATTBONUS_HUMAN),
			tch->GetPoint(POINT_ATTBONUS_ANIMAL),
			tch->GetPoint(POINT_ATTBONUS_ORC),
			tch->GetPoint(POINT_ATTBONUS_MILGYO),
			tch->GetPoint(POINT_ATTBONUS_UNDEAD));

	ch->ChatPacket(CHAT_TYPE_INFO, "   DEVIL:%3d%% INSECT:%3d%% FIRE:%3d%% ICE:%3d%% DESERT:%3d%%",
			tch->GetPoint(POINT_ATTBONUS_DEVIL),
			tch->GetPoint(POINT_ATTBONUS_INSECT),
			tch->GetPoint(POINT_ATTBONUS_FIRE),
			tch->GetPoint(POINT_ATTBONUS_ICE),
			tch->GetPoint(POINT_ATTBONUS_DESERT));

	ch->ChatPacket(CHAT_TYPE_INFO, "   TREE:%3d%% MONSTER:%3d%%",
			tch->GetPoint(POINT_ATTBONUS_TREE),
			tch->GetPoint(POINT_ATTBONUS_MONSTER));

	ch->ChatPacket(CHAT_TYPE_INFO, "   WARR:%3d%% ASSA:%3d%% SURA:%3d%% SHAM:%3d%%"
			#ifdef ENABLE_WOLFMAN_CHARACTER
			" WOLF:%3d%%"
			#endif
			,
			tch->GetPoint(POINT_ATTBONUS_WARRIOR),
			tch->GetPoint(POINT_ATTBONUS_ASSASSIN),
			tch->GetPoint(POINT_ATTBONUS_SURA),
			tch->GetPoint(POINT_ATTBONUS_SHAMAN)
			#ifdef ENABLE_WOLFMAN_CHARACTER
			,tch->GetPoint(POINT_ATTBONUS_WOLFMAN)
			#endif
	);

	//ch->ChatPacket(CHAT_TYPE_INFO, "   SWORD:%3d%% THSWORD:%3d%% DAGGER:%3d%% BELL:%3d%% FAN:%3d%% BOW:%3d%%"
	//		#ifdef ENABLE_WOLFMAN_CHARACTER
	//		" CLAW:%3d%%"
	//		#endif
	//		,
	//		tch->GetPoint(POINT_ATTBONUS_SWORD),
	//		tch->GetPoint(POINT_ATTBONUS_TWOHAND),
	//		tch->GetPoint(POINT_ATTBONUS_DAGGER),
	//		tch->GetPoint(POINT_ATTBONUS_BELL),
	//		tch->GetPoint(POINT_ATTBONUS_FAN),
	//		tch->GetPoint(POINT_ATTBONUS_BOW)
	//		#ifdef ENABLE_WOLFMAN_CHARACTER
	//		,tch->GetPoint(POINT_ATTBONUS_CLAW)
	//		#endif
	//);

	ch->ChatPacket(CHAT_TYPE_INFO, "IMMUNE:");
	ch->ChatPacket(CHAT_TYPE_INFO, "   STUN:%d SLOW:%d FALL:%d",
		tch->GetPoint(POINT_IMMUNE_STUN),
		tch->GetPoint(POINT_IMMUNE_SLOW),
		tch->GetPoint(POINT_IMMUNE_FALL));

	for (int i = 0; i < MAX_PRIV_NUM; ++i)
	{
		if (CPrivManager::instance().GetPriv(tch, i))
		{
			int iByEmpire = CPrivManager::instance().GetPrivByEmpire(tch->GetEmpire(), i);
			int iByGuild = 0;

			if (tch->GetGuild())
				iByGuild = CPrivManager::instance().GetPrivByGuild(tch->GetGuild()->GetID(), i);

			int iByPlayer = CPrivManager::instance().GetPrivByCharacter(tch->GetPlayerID(), i);

			if (iByEmpire)
				ch->ChatPacket(CHAT_TYPE_INFO, "%s for empire : %d", LC_TEXT(c_apszPrivNames[i]), iByEmpire);

			if (iByGuild)
				ch->ChatPacket(CHAT_TYPE_INFO, "%s for guild : %d", LC_TEXT(c_apszPrivNames[i]), iByGuild);

			if (iByPlayer)
				ch->ChatPacket(CHAT_TYPE_INFO, "%s for player : %d", LC_TEXT(c_apszPrivNames[i]), iByPlayer);
		}
	}
}

struct notice_packet_func
{
	const char * m_str;
#ifdef ENABLE_FULL_NOTICE
	BYTE m_bChatType;
	notice_packet_func(const char * str, BYTE bChatType) : m_str(str), m_bChatType(bChatType)
#else
	notice_packet_func(const char * str) : m_str(str)
#endif
	{
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;
#ifdef ENABLE_FULL_NOTICE
		d->GetCharacter()->ChatPacket(m_bChatType, "%s", m_str);
#else
		d->GetCharacter()->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_str);
#endif
	}
};

struct monarch_notice_packet_func
{
	const char * m_str;
	BYTE m_bEmpire;

	monarch_notice_packet_func(BYTE bEmpire, const char * str) : m_str(str), m_bEmpire(bEmpire)
	{
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (m_bEmpire == d->GetCharacter()->GetEmpire())
		{
			d->GetCharacter()->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_str);
		}
	}
};

#ifdef ENABLE_FULL_NOTICE
void SendNotice(const char * c_pszBuf, BYTE bChatType)
#else
void SendNotice(const char * c_pszBuf)
#endif
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
#ifdef ENABLE_FULL_NOTICE
	std::for_each(c_ref_set.begin(), c_ref_set.end(), notice_packet_func(c_pszBuf, bChatType));
#else
	std::for_each(c_ref_set.begin(), c_ref_set.end(), notice_packet_func(c_pszBuf));
#endif
}

void SendMonarchNotice(BYTE bEmpire, const char* c_pszBuf)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), monarch_notice_packet_func(bEmpire, c_pszBuf));
}

struct notice_map_packet_func
{
	const char* m_str;
	int m_mapIndex;
	bool m_bBigFont;
	int m_iMinLevel;
	bool m_bReqPremium;

	notice_map_packet_func(const char* str, int idx, bool bBigFont, int minLevel, bool reqPremium) : m_str(str), m_mapIndex(idx), m_bBigFont(bBigFont), m_iMinLevel(minLevel), m_bReqPremium(reqPremium)
	{
	}

	void operator() (LPDESC d)
	{
		LPCHARACTER ch = d->GetCharacter();
		if (ch == NULL) return;
		if (ch->GetMapIndex() != m_mapIndex) return;
		if (ch->GetLevel() < m_iMinLevel) return;
		if (m_bReqPremium && !ch->IsActivePremiumSubscription()) return;

		ch->ChatPacket(m_bBigFont == true ? CHAT_TYPE_BIG_NOTICE : CHAT_TYPE_NOTICE, "%s", m_str);
	}
};

void SendNoticeMap(const char* c_pszBuf, int nMapIndex, bool bBigFont, int minLevel, bool reqPremium)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), notice_map_packet_func(c_pszBuf, nMapIndex, bBigFont, minLevel, reqPremium));
}

struct log_packet_func
{
	const char * m_str;

	log_packet_func(const char * str) : m_str(str)
	{
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (d->GetCharacter()->GetGMLevel() > GM_PLAYER)
			d->GetCharacter()->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_str);
	}
};

void SendLog(const char * c_pszBuf)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), log_packet_func(c_pszBuf));
}

#ifdef ENABLE_FULL_NOTICE
void BroadcastNotice(const char * c_pszBuf, BYTE bChatType)
#else
void BroadcastNotice(const char * c_pszBuf)
#endif
{
	TPacketGGNotice p;
#ifdef ENABLE_FULL_NOTICE
	p.bHeader = HEADER_GG_NOTICE;
#else
	p.bHeader = HEADER_GG_NOTICE;
#endif
	p.lSize = strlen(c_pszBuf) + 1;
	p.bChatType = bChatType;

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(p));
	buf.write(c_pszBuf, p.lSize);

	P2P_MANAGER::instance().Send(buf.read_peek(), buf.size()); // HEADER_GG_NOTICE

#ifdef ENABLE_FULL_NOTICE
	SendNotice(c_pszBuf, bChatType);
#else
	SendNotice(c_pszBuf);
#endif
}

void BroadcastMonarchNotice(BYTE bEmpire, const char * c_pszBuf)
{
	TPacketGGMonarchNotice p;
	p.bHeader = HEADER_GG_MONARCH_NOTICE;
	p.bEmpire = bEmpire;
	p.lSize = strlen(c_pszBuf) + 1;

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(p));
	buf.write(c_pszBuf, p.lSize);

	P2P_MANAGER::instance().Send(buf.read_peek(), buf.size());

	SendMonarchNotice(bEmpire, c_pszBuf);
}

ACMD(do_notice)
{
	BroadcastNotice(argument);
}

ACMD(do_gm_notice)
{
	std::string fullMessage = "GM: " + std::string(argument);
	BroadcastNotice(fullMessage.c_str(), CHAT_TYPE_GAMEMASTER_NOTICE);
}

ACMD(do_map_notice)
{
	SendNoticeMap(argument, ch->GetMapIndex(), false);
}

ACMD(do_big_notice)
{
#ifdef ENABLE_FULL_NOTICE
	BroadcastNotice(argument, CHAT_TYPE_BIG_NOTICE);
#else
	ch->ChatPacket(CHAT_TYPE_BIG_NOTICE, "%s", argument);
#endif
}

#ifdef ENABLE_FULL_NOTICE
ACMD(do_map_big_notice)
{
	SendNoticeMap(argument, ch->GetMapIndex(), true);
}

ACMD(do_notice_test)
{
	ch->ChatPacket(CHAT_TYPE_NOTICE, "%s", argument);
}

ACMD(do_big_notice_test)
{
	ch->ChatPacket(CHAT_TYPE_BIG_NOTICE, "%s", argument);
}
#endif

ACMD(do_monarch_notice)
{
	if (ch->IsMonarch() == true)
	{
		BroadcastMonarchNotice(ch->GetEmpire(), argument);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This function can only be used by the emperor."));
	}
}

ACMD(do_who)
{
	int iTotal;
	int * paiEmpireUserCount;
	int iLocal;

	DESC_MANAGER::instance().GetUserCount(iTotal, &paiEmpireUserCount, iLocal);

	ch->ChatPacket(CHAT_TYPE_INFO, "Total [%d] %d / %d / %d (this server %d)",
			iTotal, paiEmpireUserCount[1], paiEmpireUserCount[2], paiEmpireUserCount[3], iLocal);
}

class user_func
{
	public:
		LPCHARACTER	m_ch;
		static int count;
		static char str[128];
		static int str_len;

		user_func()
			: m_ch(NULL)
		{}

		void initialize(LPCHARACTER ch)
		{
			m_ch = ch;
			str_len = 0;
			count = 0;
			str[0] = '\0';
		}

		void operator () (LPDESC d)
		{
			if (!d->GetCharacter())
				return;

			int len = snprintf(str + str_len, sizeof(str) - str_len, "%-16s ", d->GetCharacter()->GetName());

			if (len < 0 || len >= (int) sizeof(str) - str_len)
				len = (sizeof(str) - str_len) - 1;

			str_len += len;
			++count;

			if (!(count % 4))
			{
				m_ch->ChatPacket(CHAT_TYPE_INFO, str);

				str[0] = '\0';
				str_len = 0;
			}
		}
};

int	user_func::count = 0;
char user_func::str[128] = { 0, };
int	user_func::str_len = 0;

ACMD(do_user)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	user_func func;

	func.initialize(ch);
	std::for_each(c_ref_set.begin(), c_ref_set.end(), func);

	if (func.count % 4)
		ch->ChatPacket(CHAT_TYPE_INFO, func.str);

	ch->ChatPacket(CHAT_TYPE_INFO, "Total %d", func.count);
}

ACMD(do_disconnect)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "ex) /dc <player name>");
		return;
	}

	LPDESC d = DESC_MANAGER::instance().FindByCharacterName(arg1);
	LPCHARACTER	tch = d ? d->GetCharacter() : NULL;

	if (!tch)
	{
		const CCI* pkCCI = P2P_MANAGER::instance().Find(arg1);
		if (!pkCCI)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "%s: no such a player.", arg1);
			return;
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "sending disconnect packet");

		TPacketGGDisconnectPlayer p;
		p.bHeader = HEADER_GG_DISCONNECT_PLAYER;
		strlcpy(p.szName, arg1, sizeof(p.szName));
		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGDisconnectPlayer));
		return;
	}

	if (tch == ch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "cannot disconnect myself");
		return;
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "disconnect requested");

	DESC_MANAGER::instance().DestroyLoginKey(d);
	d->SetPhase(PHASE_CLOSE);
}

ACMD(do_kill)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "ex) /kill <player name>");
		return;
	}

	LPDESC	d = DESC_MANAGER::instance().FindByCharacterName(arg1);
	LPCHARACTER tch = d ? d->GetCharacter() : NULL;

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s: no such a player", arg1);
		return;
	}

	tch->Dead();
}

#ifdef ENABLE_NEWSTUFF
ACMD(do_poison)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "ex) /poison <player name>");
		return;
	}

	LPDESC	d = DESC_MANAGER::instance().FindByCharacterName(arg1);
	LPCHARACTER tch = d ? d->GetCharacter() : NULL;

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s: no such a player", arg1);
		return;
	}

	tch->AttackedByPoison(NULL);
}
#endif
#ifdef ENABLE_WOLFMAN_CHARACTER
ACMD(do_bleeding)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "ex) /bleeding <player name>");
		return;
	}

	LPDESC	d = DESC_MANAGER::instance().FindByCharacterName(arg1);
	LPCHARACTER tch = d ? d->GetCharacter() : NULL;

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s: no such a player", arg1);
		return;
	}

	tch->AttackedByBleeding(NULL);
}
#endif

#define MISC    0
#define BINARY  1
#define NUMBER  2

namespace DoSetTypes{
	typedef enum do_set_types_s {
		GOLD, RACE, SEX, JOB, EXP, MAX_HP, MAX_SP, SKILL, ALIGNMENT, ALIGN
		#ifdef ENABLE_CHEQUE_SYSTEM
		, CHEQUE, WON
		#endif
	} do_set_types_t;
}

const struct set_struct
{
	const char *cmd;
	const char type;
	const char * help;
} set_fields[] = {
	{ "gold",		NUMBER,	NULL	},
#ifdef ENABLE_WOLFMAN_CHARACTER
	{ "race",		NUMBER,	"0. Warrior, 1. Ninja, 2. Sura, 3. Shaman, 4. Lycan"		},
#else
	{ "race",		NUMBER,	"0. Warrior, 1. Ninja, 2. Sura, 3. Shaman"		},
#endif
	{ "sex",		NUMBER,	"0. Male, 1. Female"	},
	{ "job",		NUMBER,	"0. None, 1. First, 2. Second"	},
	{ "exp",		NUMBER,	NULL	},
	{ "max_hp",		NUMBER,	NULL	},
	{ "max_sp",		NUMBER,	NULL	},
	{ "skill",		NUMBER,	NULL	},
	{ "alignment",	NUMBER,	NULL	},
	{ "align",		NUMBER,	NULL	},
#ifdef ENABLE_CHEQUE_SYSTEM
	{ "cheque",		NUMBER,	NULL	},
	{ "won",		NUMBER,	NULL	},
#endif
	{ "\n",			MISC,	NULL	}
};

ACMD(do_set)
{
	char arg1[256], arg2[256], arg3[256];

	LPCHARACTER tch = NULL;

	int i, len;
	const char* line;

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (!*arg1 || !*arg2 || !*arg3)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: set <name> <field> <value>");
#ifdef ENABLE_NEWSTUFF
		ch->ChatPacket(CHAT_TYPE_INFO, "List of the fields available:");
		for (i = 0; *(set_fields[i].cmd) != '\n'; i++)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, " %d. %s", i+1, set_fields[i].cmd);
			if (set_fields[i].help != NULL)
				ch->ChatPacket(CHAT_TYPE_INFO, "  Help: %s", set_fields[i].help);
		}
#endif
		return;
	}

	tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s not exist", arg1);
		return;
	}

	len = strlen(arg2);

	for (i = 0; *(set_fields[i].cmd) != '\n'; i++)
		if (!strncmp(arg2, set_fields[i].cmd, len))
			break;

	switch (i)
	{
		case DoSetTypes::GOLD:	// gold
			{
				YANG gold = 0;
				str_to_number(gold, arg3);
				DBManager::instance().SendMoneyLog(MONEY_LOG_MISC, 3, gold);
				tch->ChangeGold(gold);
			}
			break;

		case DoSetTypes::RACE: // race
#ifdef ENABLE_NEWSTUFF
			{
				int amount = 0;
				str_to_number(amount, arg3);
				amount = MINMAX(0, amount, JOB_MAX_NUM);
				ESex mySex = GET_SEX(tch);
				DWORD dwRace = MAIN_RACE_WARRIOR_M;
				switch (amount)
				{
					case JOB_WARRIOR:
						dwRace = (mySex==SEX_MALE)?MAIN_RACE_WARRIOR_M:MAIN_RACE_WARRIOR_W;
						break;
					case JOB_ASSASSIN:
						dwRace = (mySex==SEX_MALE)?MAIN_RACE_ASSASSIN_M:MAIN_RACE_ASSASSIN_W;
						break;
					case JOB_SURA:
						dwRace = (mySex==SEX_MALE)?MAIN_RACE_SURA_M:MAIN_RACE_SURA_W;
						break;
					case JOB_SHAMAN:
						dwRace = (mySex==SEX_MALE)?MAIN_RACE_SHAMAN_M:MAIN_RACE_SHAMAN_W;
						break;
#ifdef ENABLE_WOLFMAN_CHARACTER
					case JOB_WOLFMAN:
						dwRace = (mySex==SEX_MALE)?MAIN_RACE_WOLFMAN_M:MAIN_RACE_WOLFMAN_M;
						break;
#endif
				}
				if (dwRace!=tch->GetRaceNum())
				{
					tch->SetRace(dwRace);
					tch->ClearSkill();
					tch->SetSkillGroup(0);
					// quick mesh change workaround begin
					tch->SetPolymorph(101);
					tch->SetPolymorph(0);
					// quick mesh change workaround end
				}
			}
#endif
			break;

		case DoSetTypes::SEX: // sex
#ifdef ENABLE_NEWSTUFF
			{
				int amount = 0;
				str_to_number(amount, arg3);
				amount = MINMAX(SEX_MALE, amount, SEX_FEMALE);
				if (amount != GET_SEX(tch))
				{
					tch->ChangeSex();
					// quick mesh change workaround begin
					tch->SetPolymorph(101);
					tch->SetPolymorph(0);
					// quick mesh change workaround end
				}
			}
#endif
			break;

		case DoSetTypes::JOB: // job
#ifdef ENABLE_NEWSTUFF
			{
				int amount = 0;
				str_to_number(amount, arg3);
				amount = MINMAX(0, amount, 2);
				if (amount != tch->GetSkillGroup())
				{
					tch->ClearSkill();
					tch->SetSkillGroup(amount);
				}
			}
#endif
			break;

		case DoSetTypes::EXP: // exp
			{
				int amount = 0;
				str_to_number(amount, arg3);
				tch->PointChange(POINT_EXP, amount, true);
			}
			break;

		case DoSetTypes::MAX_HP: // max_hp
			{
				int amount = 0;
				str_to_number(amount, arg3);
				tch->PointChange(POINT_MAX_HP, amount, true);
			}
			break;

		case DoSetTypes::MAX_SP: // max_sp
			{
				int amount = 0;
				str_to_number(amount, arg3);
				tch->PointChange(POINT_MAX_SP, amount, true);
			}
			break;

		case DoSetTypes::SKILL: // active skill point
			{
				int amount = 0;
				str_to_number(amount, arg3);
				tch->PointChange(POINT_SKILL, amount, true);
			}
			break;

		case DoSetTypes::ALIGN: // alignment
		case DoSetTypes::ALIGNMENT: // alignment
			{
				int	amount = 0;
				str_to_number(amount, arg3);
				tch->UpdateAlignment(amount - ch->GetRealAlignment());
			}
			break;

#ifdef ENABLE_CHEQUE_SYSTEM
		case DoSetTypes::WON: // won
		case DoSetTypes::CHEQUE: // cheque
		{
			int cheque = 0;
			str_to_number(cheque, arg3);
			tch->PointChange(POINT_CHEQUE, cheque, true);
			tch->ChatPacket(CHAT_TYPE_INFO, "Cheque: ADD[%d] TOTAL[%d]", cheque, tch->GetCheque());
		}
		break;
#endif
	}

	if (set_fields[i].type == NUMBER)
	{
		int	amount = 0;
		str_to_number(amount, arg3);
		ch->ChatPacket(CHAT_TYPE_INFO, "%s's %s set to [%d]", tch->GetName(), set_fields[i].cmd, amount);
	}
}

ACMD(do_reset)
{
	ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
	ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
	ch->PointChange(POINT_STAMINA, ch->GetMaxStamina() - ch->GetStamina());
	ch->Save();
}

ACMD(do_advance)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: advance <name> <level>");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s not exist", arg1);
		return;
	}

	int level = 0;
	str_to_number(level, arg2);

	tch->ResetPoint(MINMAX(0, level, gPlayerMaxLevel));
}

ACMD(do_respawn)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1 && !strcasecmp(arg1, "all"))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Respaw everywhere");
		regen_reset(0, 0);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Respaw around");
		regen_reset(ch->GetX(), ch->GetY());
	}
}

ACMD(do_safebox_size)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int size = 0;

	if (*arg1)
		str_to_number(size, arg1);

	if (size > 3 || size < 0)
		size = 0;

	ch->ChatPacket(CHAT_TYPE_INFO, "Safebox size set to %d", size);
	ch->ChangeSafeboxSize(size);
}

ACMD(do_makeguild)
{
	if (ch->GetGuild())
		return;

	CGuildManager& gm = CGuildManager::instance();

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	TGuildCreateParameter cp{};
	cp.master = ch;
	strlcpy(cp.name, arg1, sizeof(cp.name));

	if (!check_name(cp.name))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This guild name is invalid."));
		return;
	}

	[[maybe_unused]] auto guildID = gm.CreateGuild(cp);
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("(%s) guild has been created. [Temporary]"), cp.name);

#ifdef ENABLE_GUILD_TOKEN_AUTH
	CGuildManager::instance().GuildRelink(guildID, ch);
	// ch->ChatPacket(CHAT_TYPE_INFO, "pid %d, guild id %d, guild leader id %d", ch->GetPlayerID(), ch->GetGuild() ? ch->GetGuild()->GetID() : 0, ch->GetGuild() ? ch->GetGuild()->GetMasterPID() : 0);
	ch->SendGuildToken();
#endif
}

ACMD(do_deleteguild)
{
	if (ch->GetGuild())
		ch->GetGuild()->RequestDisband(ch->GetPlayerID());
}

ACMD(do_greset)
{
	if (ch->GetGuild())
		ch->GetGuild()->Reset();
}

// REFINE_ROD_HACK_BUG_FIX
ACMD(do_refine_rod)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	BYTE cell = 0;
	str_to_number(cell, arg1);
	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
		fishing::RealRefineRod(ch, item);
}
// END_OF_REFINE_ROD_HACK_BUG_FIX

// REFINE_PICK
ACMD(do_refine_pick)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	BYTE cell = 0;
	str_to_number(cell, arg1);
	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
	{
		mining::CHEAT_MAX_PICK(ch, item);
		mining::RealRefinePick(ch, item);
	}
}

ACMD(do_max_pick)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	BYTE cell = 0;
	str_to_number(cell, arg1);
	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
	{
		mining::CHEAT_MAX_PICK(ch, item);
	}
}
// END_OF_REFINE_PICK

ACMD(do_invisibility)
{
	if (ch->IsAffectFlag(AFF_INVISIBILITY))
	{
		ch->RemoveAffect(AFFECT_INVISIBILITY);
	}
	else
	{
		ch->AddAffect(AFFECT_INVISIBILITY, POINT_NONE, 0, AFF_INVISIBILITY, INFINITE_AFFECT_DURATION, 0, true);
		ch->ChatPacket(CHAT_TYPE_INFO, "Cheaters can still see you. (use /spy to be cheater proof)");
	}
}

ACMD(do_event_flag)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!(*arg1) || !(*arg2))
		return;

	int value = 0;
	str_to_number(value, arg2);

	if (!strcmp(arg1, "mob_item") ||
			!strcmp(arg1, "mob_exp") ||
			!strcmp(arg1, "mob_gold") ||
			!strcmp(arg1, "mob_dam") ||
			!strcmp(arg1, "mob_gold_pct") ||
			!strcmp(arg1, "mob_item_buyer") ||
			!strcmp(arg1, "mob_exp_buyer") ||
			!strcmp(arg1, "mob_gold_buyer") ||
			!strcmp(arg1, "mob_gold_pct_buyer")
	   )
		value = MINMAX(0, value, 1000);

	//quest::CQuestManager::instance().SetEventFlag(arg1, atoi(arg2));
	quest::CQuestManager::instance().RequestSetEventFlag(arg1, value);
	ch->ChatPacket(CHAT_TYPE_INFO, "RequestSetEventFlag %s %d", arg1, value);
	sys_log(0, "RequestSetEventFlag %s %d", arg1, value);
}

ACMD(do_get_event_flag)
{
	quest::CQuestManager::instance().SendEventFlagList(ch);
}

ACMD(do_private)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: private <map index>");
		return;
	}

	long lMapIndex;
	long map_index = 0;
	str_to_number(map_index, arg1);
	if ((lMapIndex = SECTREE_MANAGER::instance().CreatePrivateMap(map_index)))
	{
		ch->SaveExitLocation();

		LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);
		ch->WarpSet(pkSectreeMap->m_setting.posSpawn.x, pkSectreeMap->m_setting.posSpawn.y, lMapIndex);
	}
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Can't find map by index %d", map_index);
}

ACMD(do_qf)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	std::string questname = pPC->GetCurrentQuestName();

	if (!questname.empty())
	{
		int value = quest::CQuestManager::Instance().GetQuestStateIndex(questname, arg1);

		pPC->SetFlag(questname + ".__status", value);
		pPC->ClearTimer();

		quest::PC::QuestInfoIterator it = pPC->quest_begin();
		unsigned int questindex = quest::CQuestManager::instance().GetQuestIndexByName(questname);

		while (it!= pPC->quest_end())
		{
			if (it->first == questindex)
			{
				it->second.st = value;
				break;
			}

			++it;
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "setting quest state flag %s %s %d", questname.c_str(), arg1, value);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "setting quest state flag failed");
	}
}

LPCHARACTER chHori, chForge, chLib, chTemple, chTraining, chTree, chPortal, chBall;

ACMD(do_b1)
{
	chHori = CHARACTER_MANAGER::instance().SpawnMobRange(14017, ch->GetMapIndex(), 304222, 742858, 304222, 742858, true, false);
	chHori->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_BUILDING_CONSTRUCTION_SMALL, 65535, 0, true);
	chHori->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);

	for (int i = 0; i < 30; ++i)
	{
		int rot = number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 800, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(number(701, 706),
				ch->GetMapIndex(),
				304222 + (int)fx,
				742858 + (int)fy,
				304222 + (int)fx,
				742858 + (int)fy,
				true,
				false);
		tch->SetAggressive();
	}

	for (int i = 0; i < 5; ++i)
	{
		int rot = number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 800, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(8009,
				ch->GetMapIndex(),
				304222 + (int)fx,
				742858 + (int)fy,
				304222 + (int)fx,
				742858 + (int)fy,
				true,
				false);
		tch->SetAggressive();
	}
}

ACMD(do_b2)
{
	chHori->RemoveAffect(AFFECT_DUNGEON_UNIQUE);
}

ACMD(do_b3)
{
	chForge = CHARACTER_MANAGER::instance().SpawnMobRange(14003, ch->GetMapIndex(), 307500, 746300, 307500, 746300, true, false);
	chForge->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);

	chLib = CHARACTER_MANAGER::instance().SpawnMobRange(14007, ch->GetMapIndex(), 307900, 744500, 307900, 744500, true, false);
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);

	chTemple = CHARACTER_MANAGER::instance().SpawnMobRange(14004, ch->GetMapIndex(), 307700, 741600, 307700, 741600, true, false);
	chTemple->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);

	chTraining= CHARACTER_MANAGER::instance().SpawnMobRange(14010, ch->GetMapIndex(), 307100, 739500, 307100, 739500, true, false);
	chTraining->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	chTree= CHARACTER_MANAGER::instance().SpawnMobRange(14013, ch->GetMapIndex(), 300800, 741600, 300800, 741600, true, false);
	chTree->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	chPortal= CHARACTER_MANAGER::instance().SpawnMobRange(14001, ch->GetMapIndex(), 300900, 744500, 300900, 744500, true, false);
	chPortal->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	chBall = CHARACTER_MANAGER::instance().SpawnMobRange(14012, ch->GetMapIndex(), 302500, 746600, 302500, 746600, true, false);
	chBall->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
}

ACMD(do_b4)
{
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_BUILDING_UPGRADE, 65535, 0, true);

	for (int i = 0; i < 30; ++i)
	{
		int rot = number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 1200, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(number(701, 706),
				ch->GetMapIndex(),
				307900 + (int)fx,
				744500 + (int)fy,
				307900 + (int)fx,
				744500 + (int)fy,
				true,
				false);
		tch->SetAggressive();
	}

	for (int i = 0; i < 5; ++i)
	{
		int rot = number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 1200, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(8009,
				ch->GetMapIndex(),
				307900 + (int)fx,
				744500 + (int)fy,
				307900 + (int)fx,
				744500 + (int)fy,
				true,
				false);
		tch->SetAggressive();
	}
}

ACMD(do_b5)
{
	M2_DESTROY_CHARACTER(chLib);
	//chHori->RemoveAffect(AFFECT_DUNGEON_UNIQUE);
	chLib = CHARACTER_MANAGER::instance().SpawnMobRange(14008, ch->GetMapIndex(), 307900, 744500, 307900, 744500, true, false);
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
}

ACMD(do_b6)
{
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_BUILDING_UPGRADE, 65535, 0, true);
}
ACMD(do_b7)
{
	M2_DESTROY_CHARACTER(chLib);
	//chHori->RemoveAffect(AFFECT_DUNGEON_UNIQUE);
	chLib = CHARACTER_MANAGER::instance().SpawnMobRange(14009, ch->GetMapIndex(), 307900, 744500, 307900, 744500, true, false);
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
}

ACMD(do_book)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	CSkillProto * pkProto;

	if (isnhdigit(*arg1))
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		pkProto = CSkillManager::instance().Get(vnum);
	}
	else
		pkProto = CSkillManager::instance().Get(arg1);

	if (!pkProto)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "There is no such a skill.");
		return;
	}

	LPITEM item = ch->AutoGiveItem(50300);
	item->SetSocket(0, pkProto->dwVnum, true, "SOCKET_GM");
}

ACMD(do_setskillother)
{
	char arg1[256], arg2[256], arg3[256];
	argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(argument, arg3, sizeof(arg3));

	if (!*arg1 || !*arg2 || !*arg3 || !isdigit(*arg3))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: setskillother <target> <skillname> <lev>");
		return;
	}

	LPCHARACTER tch;

	tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "There is no such character.");
		return;
	}

	CSkillProto * pk;

	if (isdigit(*arg2))
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg2);
		pk = CSkillManager::instance().Get(vnum);
	}
	else
		pk = CSkillManager::instance().Get(arg2);

	if (!pk)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such a skill by that name.");
		return;
	}

	BYTE level = 0;
	str_to_number(level, arg3);
	tch->SetSkillLevel(pk->dwVnum, level);
	tch->ComputePoints();
	tch->SkillLevelPacket();
}

ACMD(do_setskill)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2 || !isdigit(*arg2))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: setskill <name> <lev>");
		return;
	}

	CSkillProto * pk;

	if (isdigit(*arg1))
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		pk = CSkillManager::instance().Get(vnum);
	}

	else
		pk = CSkillManager::instance().Get(arg1);

	if (!pk)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such a skill by that name.");
		return;
	}

	BYTE level = 0;
	str_to_number(level, arg2);
	ch->SetSkillLevel(pk->dwVnum, level);
	ch->ComputePoints();
	ch->SkillLevelPacket();
}

ACMD(do_set_skill_point)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int skill_point = 0;
	if (*arg1)
		str_to_number(skill_point, arg1);

	ch->SetRealPoint(POINT_SKILL, skill_point);
	ch->SetPoint(POINT_SKILL, ch->GetRealPoint(POINT_SKILL));
	ch->PointChange(POINT_SKILL, 0);
}

ACMD(do_set_skill_group)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int skill_group = 0;
	if (*arg1)
		str_to_number(skill_group, arg1);

	ch->SetSkillGroup(skill_group);

	ch->ClearSkill();
	ch->ChatPacket(CHAT_TYPE_INFO, "skill group to %d.", skill_group);
}

ACMD(do_reload)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		switch (LOWER(*arg1))
		{
			case 'u':
				ch->ChatPacket(CHAT_TYPE_INFO, "Reloading state_user_count.");
				LoadStateUserCount();
				break;

			case 'p':
			{
				if (g_dwClientVersion >= 1000000)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "Blocked on production.");
					return;
				}

				ch->ChatPacket(CHAT_TYPE_INFO, "Reloading prototype tables,");
				db_clientdesc->DBPacket(HEADER_GD_RELOAD_PROTO, 0, NULL, 0);
			}
				break;

			case 'q':
			{
				if (g_dwClientVersion >= 1000000)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "Blocked on production.");
					return;
				}
				
				ch->ChatPacket(CHAT_TYPE_INFO, "Reloading quest.");
				quest::CQuestManager::instance().Reload();
			}
				
				break;

			case 'i':
				ch->ChatPacket(CHAT_TYPE_INFO, "Reloading ItemShop.");
				db_clientdesc->DBPacket(HEADER_GD_RELOAD_ITEMSHOP, 0, NULL, 0);
				sys_log(0, "Reloading ItemShop.");
				break;

				//RELOAD_ADMIN
			case 'a':
				ch->ChatPacket(CHAT_TYPE_INFO, "Reloading Admin infomation.");
				db_clientdesc->DBPacket(HEADER_GD_RELOAD_ADMIN, 0, NULL, 0);
				sys_log(0, "Reloading admin infomation.");
				break;
				//END_RELOAD_ADMIN
			case 'c':	// cube

				Cube_init ();
				break;
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Reloading state_user_count.");
		LoadStateUserCount();

		ch->ChatPacket(CHAT_TYPE_INFO, "Reloading prototype tables,");
		db_clientdesc->DBPacket(HEADER_GD_RELOAD_PROTO, 0, NULL, 0);
	}
}

ACMD(do_cooltime)
{
	ch->DisableCooltime();
}

ACMD(do_level)
{
	char arg2[256];
	one_argument(argument, arg2, sizeof(arg2));

	if (!*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: level <level>");
		return;
	}

	int	level = 0;
	str_to_number(level, arg2);

	ch->ResetPoint(MINMAX(1, level, gPlayerMaxLevel));

	ch->ClearSkill();
	ch->ClearSubSkill();
}

ACMD(do_gwlist)
{
	ch->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("This guild is at war."));
	CGuildManager::instance().ShowGuildWarList(ch);
}

ACMD(do_stop_guild_war)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	int id1 = 0, id2 = 0;

	str_to_number(id1, arg1);
	str_to_number(id2, arg2);

	if (!id1 || !id2)
		return;

	if (id1 > id2)
	{
		std::swap(id1, id2);
	}

	ch->ChatPacket(CHAT_TYPE_TALKING, "%d %d", id1, id2);
	CGuildManager::instance().RequestEndWar(id1, id2);
}

ACMD(do_cancel_guild_war)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	int id1 = 0, id2 = 0;
	str_to_number(id1, arg1);
	str_to_number(id2, arg2);

	if (id1 > id2)
		std::swap(id1, id2);

	CGuildManager::instance().RequestCancelWar(id1, id2);
}

ACMD(do_guild_state)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	CGuild* pGuild = CGuildManager::instance().FindGuildByName(arg1);
	if (pGuild != NULL)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "GuildID: %d", pGuild->GetID());
		ch->ChatPacket(CHAT_TYPE_INFO, "GuildMasterPID: %d", pGuild->GetMasterPID());
		ch->ChatPacket(CHAT_TYPE_INFO, "IsInWar: %d", pGuild->UnderAnyWar());
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s: This guild does not exist."), arg1);
	}
}

struct FuncWeaken
{
	LPCHARACTER m_pkGM;
	bool	m_bAll;

	FuncWeaken(LPCHARACTER ch) : m_pkGM(ch), m_bAll(false)
	{
	}

	void operator () (LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER) ent;

		int iDist = DISTANCE_APPROX(pkChr->GetX() - m_pkGM->GetX(), pkChr->GetY() - m_pkGM->GetY());

		if (!m_bAll && iDist >= 1000)
			return;

		if (pkChr->IsNPC())
			pkChr->PointChange(POINT_HP, (10 - pkChr->GetHP()));
	}
};

ACMD(do_weaken)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	FuncWeaken func(ch);

	if (*arg1 && !strcmp(arg1, "all"))
		func.m_bAll = true;

	ch->GetSectree()->ForEachAround(func);
}

ACMD(do_getqf)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	LPCHARACTER tch;

	if (!*arg1)
		tch = ch;
	else
	{
		tch = CHARACTER_MANAGER::instance().FindPC(arg1);

		if (!tch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no such character.");
			return;
		}
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPC(tch->GetPlayerID());

	if (pPC)
		pPC->SendFlagList(ch);
}

#define ENABLE_SET_STATE_WITH_TARGET
ACMD(do_set_state)
{
	char arg1[256];
	char arg2[256];

	argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO,
			"Syntax: set_state <questname> <statename>"
#ifdef ENABLE_SET_STATE_WITH_TARGET
			" [<character name>]"
#endif
		);
		return;
	}

#ifdef ENABLE_SET_STATE_WITH_TARGET
	LPCHARACTER tch = ch;
	char arg3[256];
	argument = one_argument(argument, arg3, sizeof(arg3));
	if (*arg3)
	{
		tch = CHARACTER_MANAGER::instance().FindPC(arg3);
		if (!tch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no such character.");
			return;
		}
	}
	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(tch->GetPlayerID());
#else
	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
#endif
	std::string questname = arg1;
	std::string statename = arg2;

	if (!questname.empty())
	{
		int value = quest::CQuestManager::Instance().GetQuestStateIndex(questname, statename);

		pPC->SetFlag(questname + ".__status", value);
		pPC->ClearTimer();

		quest::PC::QuestInfoIterator it = pPC->quest_begin();
		unsigned int questindex = quest::CQuestManager::instance().GetQuestIndexByName(questname);

		while (it!= pPC->quest_end())
		{
			if (it->first == questindex)
			{
				it->second.st = value;
				break;
			}

			++it;
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "setting quest state flag %s %s %d", questname.c_str(), arg1, value);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "setting quest state flag failed");
	}
}

ACMD(do_setqf)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];

	one_argument(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: setqf <flagname> <value> [<character name>]");
		return;
	}

	LPCHARACTER tch = ch;

	if (*arg3)
		tch = CHARACTER_MANAGER::instance().FindPC(arg3);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "There is no such character.");
		return;
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPC(tch->GetPlayerID());

	if (pPC)
	{
		int value = 0;
		str_to_number(value, arg2);
		pPC->SetFlag(arg1, value);
		ch->ChatPacket(CHAT_TYPE_INFO, "Quest flag set: %s %d", arg1, value);
	}
}

ACMD(do_delqf)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: delqf <flagname> [<character name>]");
		return;
	}

	LPCHARACTER tch = ch;

	if (*arg2)
		tch = CHARACTER_MANAGER::instance().FindPC(arg2);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "There is no such character.");
		return;
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPC(tch->GetPlayerID());

	if (pPC)
	{
		if (pPC->DeleteFlag(arg1))
			ch->ChatPacket(CHAT_TYPE_INFO, "Delete success.");
		else
			ch->ChatPacket(CHAT_TYPE_INFO, "Delete failed. Quest flag does not exist.");
	}
}

ACMD(do_forgetme)
{
	ch->ForgetMyAttacker();
}

ACMD(do_aggregate)
{
	ch->AggregateMonster();
}

ACMD(do_attract_ranger)
{
	ch->AttractRanger();
}

ACMD(do_pull_monster)
{
	ch->PullMonster();
}

ACMD(do_polymorph)
{
	char arg1[256], arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	if (*arg1)
	{
		DWORD dwVnum = 0;
		str_to_number(dwVnum, arg1);
		bool bMaintainStat = false;
		if (*arg2)
		{
			int value = 0;
			str_to_number(value, arg2);
			bMaintainStat = (value>0);
		}

		ch->SetPolymorph(dwVnum, bMaintainStat);
	}
}

ACMD(do_polymorph_item)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		DWORD dwVnum = 0;
		str_to_number(dwVnum, arg1);

		LPITEM item = ITEM_MANAGER::instance().CreateItem(70104, 1, 0, true);
		if (item)
		{
			item->SetSocket(0, dwVnum, true, "SOCKET_GM");
			int iEmptyPos = ch->GetEmptyInventory(item->GetSize());

			if (iEmptyPos != -1)
			{
				item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
				LogManager::instance().ItemLog(ch, item, "GM", item->GetName());
			}
			else
			{
				M2_DESTROY_ITEM(item);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Not enough inventory space."));
			}
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "#%d item not exist by that vnum.", 70103);
		}
		//ch->SetPolymorph(dwVnum, bMaintainStat);
	}
}

ACMD(do_priv_empire)
{
	char arg1[256] = {0};
	char arg2[256] = {0};
	char arg3[256] = {0};
	char arg4[256] = {0};
	int empire = 0;
	int type = 0;
	int value = 0;
	int duration = 0;

	const char* line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		goto USAGE;

	if (!line)
		goto USAGE;

	two_arguments(line, arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg3 || !*arg4)
		goto USAGE;

	str_to_number(empire, arg1);
	str_to_number(type,	arg2);
	str_to_number(value,	arg3);
	value = MINMAX(0, value, 1000);
	str_to_number(duration, arg4);

	if (empire < 0 || 3 < empire)
		goto USAGE;

	if (type < 1 || 4 < type)
		goto USAGE;

	if (value < 0)
		goto USAGE;

	if (duration < 0)
		goto USAGE;

	duration = duration * (60*60);

	sys_log(0, "_give_empire_privileage(empire=%d, type=%d, value=%d, duration=%d) by command",
			empire, type, value, duration);
	CPrivManager::instance().RequestGiveEmpirePriv(empire, type, value, duration);
	return;

USAGE:
	ch->ChatPacket(CHAT_TYPE_INFO, "usage : priv_empire <empire> <type> <value> <duration>");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <empire>    0 - 3 (0==all)");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <type>      1:item_drop, 2:gold_drop, 3:gold10_drop, 4:exp");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <value>     percent");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <duration>  hour");
}

ACMD(do_priv_guild)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		CGuild * g = CGuildManager::instance().FindGuildByName(arg1);

		if (!g)
		{
			DWORD guild_id = 0;
			str_to_number(guild_id, arg1);
			g = CGuildManager::instance().FindGuild(guild_id);
		}

		if (!g)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("A guild with this name or number does not exist."));
		else
		{
			char buf[1024+1];
			snprintf(buf, sizeof(buf), "%d", g->GetID()); // @fixme177

			using namespace quest;
			PC * pc = CQuestManager::instance().GetPC(ch->GetPlayerID());
			QuestState qs = CQuestManager::instance().OpenState("ADMIN_QUEST", QUEST_FISH_REFINE_STATE_INDEX);
			luaL_loadbuffer(qs.co, buf, strlen(buf), "ADMIN_QUEST");
			pc->SetQuest("ADMIN_QUEST", qs);

			QuestState & rqs = *pc->GetRunningQuestState();

			if (!CQuestManager::instance().RunState(rqs))
			{
				CQuestManager::instance().CloseState(rqs);
				pc->EndRunning();
				return;
			}
		}
	}
}

ACMD(do_mount_test)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		ch->MountVnum(vnum);
	}
}

ACMD(do_observer)
{
	ch->SetObserverMode(!ch->IsObserverMode());
}

ACMD(do_socket_item)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1)
	{
		DWORD dwVnum = 0;
		str_to_number(dwVnum, arg1);

		int iSocketCount = 0;
		str_to_number(iSocketCount, arg2);

		if (!iSocketCount || iSocketCount >= ITEM_SOCKET_MAX_NUM)
			iSocketCount = 3;

		if (!dwVnum)
		{
			if (!ITEM_MANAGER::instance().GetVnum(arg1, dwVnum))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "#%d item not exist by that vnum.", dwVnum);
				return;
			}
		}

		LPITEM item = ch->AutoGiveItem(dwVnum);

		if (item)
		{
			for (int i = 0; i < iSocketCount; ++i)
				item->SetSocket(i, 1, true, "SOCKET_GM");
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "#%d cannot create item.", dwVnum);
		}
	}
}

ACMD(do_xmas)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int flag = 0;

	if (*arg1)
		str_to_number(flag, arg1);

	switch (subcmd)
	{
		case SCMD_XMAS_SNOW:
			quest::CQuestManager::instance().RequestSetEventFlag("xmas_snow", flag);
			break;

		case SCMD_XMAS_BOOM:
			quest::CQuestManager::instance().RequestSetEventFlag("xmas_boom", flag);
			break;

		case SCMD_XMAS_SANTA:
			quest::CQuestManager::instance().RequestSetEventFlag("xmas_santa", flag);
			break;
	}
}

// BLOCK_CHAT
ACMD(do_block_chat_list)
{
	if (!ch || (ch->GetGMLevel() < GM_HIGH_WIZARD && ch->GetQuestFlag("chat_privilege.block") <= 0))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This command does not exist."));
		return;
	}

	DBManager::instance().ReturnQuery(QID_BLOCK_CHAT_LIST, ch->GetPlayerID(), NULL,
			"SELECT p.name, a.lDuration FROM affect%s as a, player%s as p WHERE a.bType = %d AND a.dwPID = p.id",
			get_table_postfix(), get_table_postfix(), AFFECT_BLOCK_CHAT);
}

ACMD(do_vote_block_chat)
{
	return;

	char arg1[256];
	argument = one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: vote_block_chat <name>");
		return;
	}

	const char* name = arg1;
	long lBlockDuration = 10;
	sys_log(0, "vote_block_chat %s %d", name, lBlockDuration);

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(name);

		if (pkCCI)
		{
			TPacketGGBlockChat p;

			p.bHeader = HEADER_GG_BLOCK_CHAT;
			strlcpy(p.szName, name, sizeof(p.szName));
			p.lBlockDuration = lBlockDuration;
			P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGBlockChat));
		}
		else
		{
			TPacketBlockChat p;

			strlcpy(p.szName, name, sizeof(p.szName));
			p.lDuration = lBlockDuration;
			db_clientdesc->DBPacket(HEADER_GD_BLOCK_CHAT, ch ? ch->GetDesc()->GetHandle() : 0, &p, sizeof(p));

		}

		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Chat block requested.");

		return;
	}

	if (tch && ch != tch)
		tch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, lBlockDuration, 0, true);
}

ACMD(do_block_chat)
{
	if (ch && (ch->GetGMLevel() < GM_HIGH_WIZARD && ch->GetQuestFlag("chat_privilege.block") <= 0))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This command does not exist."));
		return;
	}

	char arg1[256];
	argument = one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Usage: block_chat <name> <time> (0 to off)");

		return;
	}

	const char* name = arg1;
	long lBlockDuration = parse_time_str(argument);

	if (lBlockDuration < 0)
	{
		if (ch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "잘못된 형식의 시간입니다. h, m, s를 붙여서 지정해 주십시오.");
			ch->ChatPacket(CHAT_TYPE_INFO, "예) 10s, 10m, 1m 30s");
		}
		return;
	}

	sys_log(0, "BLOCK CHAT %s %d", name, lBlockDuration);

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(name);

		if (pkCCI)
		{
			TPacketGGBlockChat p;

			p.bHeader = HEADER_GG_BLOCK_CHAT;
			strlcpy(p.szName, name, sizeof(p.szName));
			p.lBlockDuration = lBlockDuration;
			P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGBlockChat));
		}
		else
		{
			TPacketBlockChat p;

			strlcpy(p.szName, name, sizeof(p.szName));
			p.lDuration = lBlockDuration;
			db_clientdesc->DBPacket(HEADER_GD_BLOCK_CHAT, ch ? ch->GetDesc()->GetHandle() : 0, &p, sizeof(p));
		}

		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Chat block requested.");

		return;
	}

	if (tch && ch != tch)
		tch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, lBlockDuration, 0, true);
}
// END_OF_BLOCK_CHAT

// BUILD_BUILDING
ACMD(do_build)
{
	using namespace building;

	char arg1[256], arg2[256], arg3[256], arg4[256];
	const char * line = one_argument(argument, arg1, sizeof(arg1));
	BYTE GMLevel = ch->GetGMLevel();

	CLand * pkLand = CManager::instance().FindLand(ch->GetMapIndex(), ch->GetX(), ch->GetY());

	if (!pkLand)
	{
		sys_err("%s trying to build on not buildable area.", ch->GetName());
		return;
	}

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Invalid syntax: no command");
		return;
	}

	if (GMLevel == GM_PLAYER)
	{
		if ((!ch->GetGuild() || ch->GetGuild()->GetID() != pkLand->GetOwner()))
		{
			sys_err("%s trying to build on not owned land.", ch->GetName());
			return;
		}

		if (ch->GetGuild()->GetMasterPID() != ch->GetPlayerID())
		{
			sys_err("%s trying to build while not the guild master.", ch->GetName());
			return;
		}
	}

	switch (LOWER(*arg1))
	{
		case 'c':
			{
				// /build c vnum x y x_rot y_rot z_rot
				char arg5[256], arg6[256];
				line = one_argument(two_arguments(line, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3)); // vnum x y
				one_argument(two_arguments(line, arg4, sizeof(arg4), arg5, sizeof(arg5)), arg6, sizeof(arg6)); // x_rot y_rot z_rot

				if (!*arg1 || !*arg2 || !*arg3 || !*arg4 || !*arg5 || !*arg6)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "Invalid syntax");
					return;
				}

				DWORD dwVnum = 0;
				str_to_number(dwVnum,  arg1);

				using namespace building;

				const TObjectProto * t = CManager::instance().GetObjectProto(dwVnum);
				if (!t)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The building does not exist."));
					return;
				}

				const YANG BUILDING_MAX_PRICE = 100000000;

				if (t->dwGroupVnum)
				{
					if (pkLand->FindObjectByGroup(t->dwGroupVnum))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This type of building can only be erected once."));
						return;
					}
				}

				if (t->dwDependOnGroupVnum)
				{
					//		const TObjectProto * dependent = CManager::instance().GetObjectProto(dwVnum);
					//		if (dependent)
					{
						if (!pkLand->FindObjectByGroup(t->dwDependOnGroupVnum))
						{
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The Main Building has to be erected first."));
							return;
						}
					}
				}

				if (test_server || GMLevel == GM_PLAYER)
				{
					if (t->dwPrice > BUILDING_MAX_PRICE)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Building failed because of incorrect pricing."));
						return;
					}

					if (ch->GetGold() < (int)t->dwPrice)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your guild does not have enough Yang to erect this building."));
						return;
					}

					int i;
					for (i = 0; i < OBJECT_MATERIAL_MAX_NUM; ++i)
					{
						DWORD dwItemVnum = t->kMaterials[i].dwItemVnum;
						DWORD dwItemCount = t->kMaterials[i].dwCount;

						if (dwItemVnum == 0)
							break;

						if ((int) dwItemCount > ch->CountSpecifyItem(dwItemVnum))
						{
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have enough resources to build a building."));
							return;
						}
					}
				}

				float x_rot = atof(arg4);
				float y_rot = atof(arg5);
				float z_rot = atof(arg6);

				long map_x = 0;
				str_to_number(map_x, arg2);
				long map_y = 0;
				str_to_number(map_y, arg3);

				bool isSuccess = pkLand->RequestCreateObject(dwVnum,
						ch->GetMapIndex(),
						map_x,
						map_y,
						x_rot,
						y_rot,
						z_rot, true);

				if (!isSuccess)
				{
					if (test_server)
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot erect a building at this place."));
					return;
				}

				if (test_server || GMLevel == GM_PLAYER)

				{
					ch->ChangeGold(-t->dwPrice);

					{
						int i;
						for (i = 0; i < OBJECT_MATERIAL_MAX_NUM; ++i)
						{
							DWORD dwItemVnum = t->kMaterials[i].dwItemVnum;
							DWORD dwItemCount = t->kMaterials[i].dwCount;

							if (dwItemVnum == 0)
								break;

							sys_log(0, "BUILD: material %d %u %u", i, dwItemVnum, dwItemCount);
							ch->RemoveSpecifyItem(dwItemVnum, dwItemCount);
						}
					}
				}
			}
			break;

		case 'd' :
			// build (d)elete ObjectID
			{
				one_argument(line, arg1, sizeof(arg1));

				if (!*arg1)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "Invalid syntax");
					return;
				}

				DWORD vid = 0;
				str_to_number(vid, arg1);
				pkLand->RequestDeleteObjectByVID(vid);
			}
			break;

			// BUILD_WALL

			// build w n/e/w/s
		case 'w' :
			if (GMLevel > GM_PLAYER)
			{
				int mapIndex = ch->GetMapIndex();

				one_argument(line, arg1, sizeof(arg1));

				sys_log(0, "guild.wall.build map[%d] direction[%s]", mapIndex, arg1);

				switch (arg1[0])
				{
					case 's':
						pkLand->RequestCreateWall(mapIndex,   0.0f);
						break;
					case 'n':
						pkLand->RequestCreateWall(mapIndex, 180.0f);
						break;
					case 'e':
						pkLand->RequestCreateWall(mapIndex,  90.0f);
						break;
					case 'w':
						pkLand->RequestCreateWall(mapIndex, 270.0f);
						break;
					default:
						ch->ChatPacket(CHAT_TYPE_INFO, "guild.wall.build unknown_direction[%s]", arg1);
						sys_err("guild.wall.build unknown_direction[%s]", arg1);
						break;
				}

			}
			break;

		case 'e':
			if (GMLevel > GM_PLAYER)
			{
				pkLand->RequestDeleteWall();
			}
			break;

		case 'W' :

			if (GMLevel >  GM_PLAYER)
			{
				int setID = 0, wallSize = 0;
				char arg5[256], arg6[256];
				line = two_arguments(line, arg1, sizeof(arg1), arg2, sizeof(arg2));
				line = two_arguments(line, arg3, sizeof(arg3), arg4, sizeof(arg4));
				two_arguments(line, arg5, sizeof(arg5), arg6, sizeof(arg6));

				str_to_number(setID, arg1);
				str_to_number(wallSize, arg2);

				if (setID != 14105 && setID != 14115 && setID != 14125)
				{
					sys_log(0, "BUILD_WALL: wrong wall set id %d", setID);
					break;
				}
				else
				{
					bool door_east = false;
					str_to_number(door_east, arg3);
					bool door_west = false;
					str_to_number(door_west, arg4);
					bool door_south = false;
					str_to_number(door_south, arg5);
					bool door_north = false;
					str_to_number(door_north, arg6);
					pkLand->RequestCreateWallBlocks(setID, ch->GetMapIndex(), wallSize, door_east, door_west, door_south, door_north);
				}
			}
			break;

		case 'E' :

			if (GMLevel > GM_PLAYER)
			{
				one_argument(line, arg1, sizeof(arg1));
				DWORD id = 0;
				str_to_number(id, arg1);
				pkLand->RequestDeleteWallBlocks(id);
			}
			break;

		default:
			ch->ChatPacket(CHAT_TYPE_INFO, "Invalid command %s", arg1);
			break;
	}
}
// END_OF_BUILD_BUILDING

ACMD(do_clear_quest)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	pPC->ClearQuest(arg1);
}

ACMD(do_horse_state)
{
	ch->ChatPacket(CHAT_TYPE_INFO, "Horse Information:");
	ch->ChatPacket(CHAT_TYPE_INFO, "    Level  %d", ch->GetHorseLevel());
	ch->ChatPacket(CHAT_TYPE_INFO, "    Health %d/%d (%d%%)", ch->GetHorseHealth(), ch->GetHorseMaxHealth(), ch->GetHorseHealth() * 100 / ch->GetHorseMaxHealth());
	ch->ChatPacket(CHAT_TYPE_INFO, "    Stam   %d/%d (%d%%)", ch->GetHorseStamina(), ch->GetHorseMaxStamina(), ch->GetHorseStamina() * 100 / ch->GetHorseMaxStamina());
}

ACMD(do_horse_level)
{
	char arg1[256] = {0};
	char arg2[256] = {0};
	LPCHARACTER victim;
	int	level = 0;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage : /horse_level <name> <level>");
		return;
	}

	victim = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (NULL == victim)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This character does not exist."));
		return;
	}

	str_to_number(level, arg2);
	level = MINMAX(0, level, HORSE_MAX_LEVEL);

	ch->ChatPacket(CHAT_TYPE_INFO, "horse level set (%s: %d)", victim->GetName(), level);

	victim->SetHorseLevel(level);
	victim->ComputePoints();
	victim->SkillLevelPacket();
	return;
}

ACMD(do_horse_ride)
{
	if (ch->IsHorseRiding())
		ch->StopRiding();
	else
		ch->StartRiding();
}

ACMD(do_horse_summon)
{
	ch->HorseSummon(true, true);
}

ACMD(do_horse_unsummon)
{
	ch->HorseSummon(false, true);
}

ACMD(do_horse_set_stat)
{
	char arg1[256], arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1 && *arg2)
	{
		int hp = 0;
		str_to_number(hp, arg1);
		int stam = 0;
		str_to_number(stam, arg2);
		ch->UpdateHorseHealth(hp - ch->GetHorseHealth());
		ch->UpdateHorseStamina(stam - ch->GetHorseStamina());
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage : /horse_set_stat <hp> <stamina>");
	}
}

ACMD(do_save_attribute_to_image) // command "/saveati" for alias
{
	char szFileName[256];
	char szMapIndex[256];

	two_arguments(argument, szMapIndex, sizeof(szMapIndex), szFileName, sizeof(szFileName));

	if (!*szMapIndex || !*szFileName)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: /saveati <map_index> <filename>");
		return;
	}

	long lMapIndex = 0;
	str_to_number(lMapIndex, szMapIndex);

	if (SECTREE_MANAGER::instance().SaveAttributeToImage(lMapIndex, szFileName))
		ch->ChatPacket(CHAT_TYPE_INFO, "Save done.");
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Save failed.");
}

ACMD(do_affect_remove)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: /affect_remove <player name>");
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: /affect_remove <type> <point>");

		LPCHARACTER tch = ch;

		if (*arg1)
			if (!(tch = CHARACTER_MANAGER::instance().FindPC(arg1)))
				tch = ch;

		ch->ChatPacket(CHAT_TYPE_INFO, "-- Affect List of %s -------------------------------", tch->GetName());
		ch->ChatPacket(CHAT_TYPE_INFO, "Type Point Modif Duration Flag");

		const std::list<CAffect *> & cont = tch->GetAffectContainer();

		itertype(cont) it = cont.begin();

		while (it != cont.end())
		{
			CAffect * pkAff = *it++;

			ch->ChatPacket(CHAT_TYPE_INFO, "%4d %5d %5d %8d %u",
					pkAff->dwType, pkAff->bApplyOn, pkAff->lApplyValue, pkAff->lDuration, pkAff->dwFlag);
		}
		return;
	}

	bool removed = false;

	CAffect * af;

	DWORD	type = 0;
	str_to_number(type, arg1);
	BYTE	point = 0;
	str_to_number(point, arg2);
	while ((af = ch->FindAffect(type, point)))
	{
		ch->RemoveAffect(af);
		removed = true;
	}

	if (removed)
		ch->ChatPacket(CHAT_TYPE_INFO, "Affect successfully removed.");
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Not affected by that type and point.");
}

ACMD(do_change_attr)
{
	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->ChangeAttribute();
}

ACMD(do_add_attr)
{
	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->AddAttribute();
}

ACMD(do_add_socket)
{
	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->AddSocket();
}

#ifdef ENABLE_NEWSTUFF
ACMD(do_change_rare_attr)
{
	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->ChangeRareAttribute();
}

ACMD(do_add_rare_attr)
{
	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->AddRareAttribute();
}
#endif

ACMD(do_show_arena_list)
{
	CArenaManager::instance().SendArenaMapListTo(ch);
}

ACMD(do_end_all_duel)
{
	CArenaManager::instance().EndAllDuel();
}

ACMD(do_end_duel)
{
	char szName[256];

	one_argument(argument, szName, sizeof(szName));

	LPCHARACTER pChar = CHARACTER_MANAGER::instance().FindPC(szName);
	if (pChar == NULL)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This character does not exist."));
		return;
	}

	if (CArenaManager::instance().EndDuel(pChar->GetPlayerID()) == false)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Duel has not been successfully cancelled."));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Duel cancelled successfully."));
	}
}

ACMD(do_duel)
{
	char szName1[256];
	char szName2[256];
	char szSet[256];
	char szMinute[256];
	int set = 0;
	int minute = 0;

	argument = two_arguments(argument, szName1, sizeof(szName1), szName2, sizeof(szName2));
	two_arguments(argument, szSet, sizeof(szSet), szMinute, sizeof(szMinute));

	str_to_number(set, szSet);

	if (set < 0) set = 1;
	if (set > 5) set = 5;

	if (!str_to_number(minute, szMinute))
		minute = 5;

	if (minute < 5)
		minute = 5;

	LPCHARACTER pChar1 = CHARACTER_MANAGER::instance().FindPC(szName1);
	LPCHARACTER pChar2 = CHARACTER_MANAGER::instance().FindPC(szName2);

	if (pChar1 != NULL && pChar2 != NULL)
	{
		pChar1->RemoveGoodAffect();
		pChar2->RemoveGoodAffect();

		pChar1->RemoveBadAffect();
		pChar2->RemoveBadAffect();

		LPPARTY pParty = pChar1->GetParty();
		if (pParty != NULL)
		{
			if (pParty->GetMemberCount() == 2)
			{
				CPartyManager::instance().DeleteParty(pParty);
			}
			else
			{
				pChar1->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] You have left the group."));
				pParty->Quit(pChar1->GetPlayerID());
			}
		}

		pParty = pChar2->GetParty();
		if (pParty != NULL)
		{
			if (pParty->GetMemberCount() == 2)
			{
				CPartyManager::instance().DeleteParty(pParty);
			}
			else
			{
				pChar2->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] You have left the group."));
				pParty->Quit(pChar2->GetPlayerID());
			}
		}

		if (CArenaManager::instance().StartDuel(pChar1, pChar2, set, minute) == true)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The duel has been successfully started."));
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is a problem with initiating the duel."));
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There are no combatants."));
	}
}

//#define ENABLE_STATPLUS_NOLIMIT
ACMD(do_stat_plus_amount)
{
	char szPoint[256];

	one_argument(argument, szPoint, sizeof(szPoint));

	if (*szPoint == '\0')
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change your status while you are transformed."));
		return;
	}

	int nRemainPoint = ch->GetPoint(POINT_STAT);

	if (nRemainPoint <= 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("No status points left."));
		return;
	}

	int nPoint = 0;
	str_to_number(nPoint, szPoint);

	if (nRemainPoint < nPoint)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Remaining status points are too low."));
		return;
	}

	if (nPoint < 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You entered an incorrect value."));
		return;
	}

#ifndef ENABLE_STATPLUS_NOLIMIT
	switch (subcmd)
	{
		case POINT_HT :
			if (nPoint + ch->GetPoint(POINT_HT) > 90)
			{
				nPoint = 90 - ch->GetPoint(POINT_HT);
			}
			break;

		case POINT_IQ :
			if (nPoint + ch->GetPoint(POINT_IQ) > 90)
			{
				nPoint = 90 - ch->GetPoint(POINT_IQ);
			}
			break;

		case POINT_ST :
			if (nPoint + ch->GetPoint(POINT_ST) > 90)
			{
				nPoint = 90 - ch->GetPoint(POINT_ST);
			}
			break;

		case POINT_DX :
			if (nPoint + ch->GetPoint(POINT_DX) > 90)
			{
				nPoint = 90 - ch->GetPoint(POINT_DX);
			}
			break;

		default :
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Suborder or the Order is incorrect."));
			return;
			break;
	}
#endif

	if (nPoint != 0)
	{
		ch->SetRealPoint(subcmd, ch->GetRealPoint(subcmd) + nPoint);
		ch->SetPoint(subcmd, ch->GetPoint(subcmd) + nPoint);
		ch->ComputePoints();
		ch->PointChange(subcmd, 0);

		ch->PointChange(POINT_STAT, -nPoint);
		ch->ComputePoints();
	}
}

struct tTwoPID
{
	int pid1;
	int pid2;
};

ACMD(do_break_marriage)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	tTwoPID pids = { 0, 0 };

	str_to_number(pids.pid1, arg1);
	str_to_number(pids.pid2, arg2);

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Broken contract between player %d and player %d."), pids.pid1, pids.pid2);
	db_clientdesc->DBPacket(HEADER_GD_BREAK_MARRIAGE, 0, &pids, sizeof(pids));
}

ACMD(do_effect)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	int	effect_type = 0;
	str_to_number(effect_type, arg1);
	ch->EffectPacket(effect_type);
}

struct FCountInMap
{
	int m_Count[4];
	FCountInMap() { memset(m_Count, 0, sizeof(int) * 4); }
	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			if (ch && ch->IsPC())
				++m_Count[ch->GetEmpire()];
		}
	}
	int GetCount(BYTE bEmpire) { return m_Count[bEmpire]; }
};

ACMD(do_threeway_war_info)
{
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Information for the Kingdoms"));
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Choose the Map Information of the Holy Land %d Entrance %d %d %d"), GetSungziMapIndex(), GetPassMapIndex(1), GetPassMapIndex(2), GetPassMapIndex(3));
	ch->ChatPacket(CHAT_TYPE_INFO, "ThreewayPhase %d", CThreeWayWar::instance().GetRegenFlag());

	for (int n = 1; n < 4; ++n)
	{
		LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap(GetSungziMapIndex());

		FCountInMap c;

		if (pSecMap)
		{
			pSecMap->for_each(c);
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "%s killscore %d usercount %d",
				EMPIRE_NAME(n),
			   	CThreeWayWar::instance().GetKillScore(n),
				c.GetCount(n));
	}
}

ACMD(do_threeway_war_myinfo)
{
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Information about the status of the kingdom battle"));
	ch->ChatPacket(CHAT_TYPE_INFO, "Deadcount %d",
			CThreeWayWar::instance().GetReviveTokenForPlayer(ch->GetPlayerID()));
}

ACMD(do_rmcandidacy)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: rmcandidacy <name>");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "Target is in %d channel (my channel %d)", pkCCI->bChannel, g_bChannel);
				return;
			}
		}
	}

	db_clientdesc->DBPacket(HEADER_GD_RMCANDIDACY, 0, NULL, 32);
	db_clientdesc->Packet(arg1, 32);
}

ACMD(do_setmonarch)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: setmonarch <name>");
		return;
	}

	db_clientdesc->DBPacket(HEADER_GD_SETMONARCH, 0, NULL, 32);
	db_clientdesc->Packet(arg1, 32);
}

ACMD(do_rmmonarch)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: rmmonarch <name>");
		return;
	}

	db_clientdesc->DBPacket(HEADER_GD_RMMONARCH, 0, NULL, 32);
	db_clientdesc->Packet(arg1, 32);
}

ACMD(do_check_monarch_money)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	int empire = 0;
	str_to_number(empire, arg1);
	int NationMoney = CMonarch::instance().GetMoney(empire);

	ch->ChatPacket(CHAT_TYPE_INFO, "국고: %d 원", NationMoney);
}

ACMD(do_reset_subskill)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: reset_subskill <name>");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (tch == NULL)
		return;

	tch->ClearSubSkill();
	ch->ChatPacket(CHAT_TYPE_INFO, "Subskill of [%s] was reset", tch->GetName());
}

ACMD(do_siege)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	int	empire = strtol(arg1, NULL, 10);
	int tower_count = strtol(arg2, NULL, 10);

	if (empire == 0) empire = number(1, 3);
	if (tower_count < 5 || tower_count > 10) tower_count = number(5, 10);

	TPacketGGSiege packet;
	packet.bHeader = HEADER_GG_SIEGE;
	packet.bEmpire = empire;
	packet.bTowerCount = tower_count;

	P2P_MANAGER::instance().Send(&packet, sizeof(TPacketGGSiege));

	switch (castle_siege(empire, tower_count))
	{
		case 0 :
			ch->ChatPacket(CHAT_TYPE_INFO, "SIEGE FAILED");
			break;
		case 1 :
			ch->ChatPacket(CHAT_TYPE_INFO, "SIEGE START Empire(%d) Tower(%d)", empire, tower_count);
			break;
		case 2 :
			ch->ChatPacket(CHAT_TYPE_INFO, "SIEGE END");
			break;
	}
}

ACMD(do_temp)
{
	if (false == test_server)
		return;

	char	arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (0 == arg1[0] || 0 == arg2[0])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: empire money");
		return;
	}

	int	empire = 0;
	str_to_number(empire, arg1);
	int	money = 0;
	str_to_number(money, arg2);

	CMonarch::instance().SendtoDBAddMoney(money, empire, ch);
}

ACMD(do_frog)
{
	char	arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (0 == arg1[0])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: empire(1, 2, 3)");
		return;
	}

	int	empire = 0;
	str_to_number(empire, arg1);

	switch (empire)
	{
		case 1:
		case 2:
		case 3:
			if (IS_CASTLE_MAP(ch->GetMapIndex()))
			{
				castle_spawn_frog(empire);
				castle_save();
			}
			else
				ch->ChatPacket(CHAT_TYPE_INFO, "You must spawn frog in castle");
			break;

		default:
			ch->ChatPacket(CHAT_TYPE_INFO, "Usage: empire(1, 2, 3)");
			break;
	}
}

ACMD(do_flush)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (0 == arg1[0])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage : /flush player_id");
		return;
	}

	DWORD pid = (DWORD) strtoul(arg1, NULL, 10);
	auto tch = CHARACTER_MANAGER::instance().FindByPID(pid);
	if (!tch) {
		ch->ChatPacket(CHAT_TYPE_INFO, "player_id %d not found", pid);
		return;
	}
	tch->Save();
}

ACMD(do_eclipse)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (strtol(arg1, NULL, 10) == 1)
	{
		quest::CQuestManager::instance().RequestSetEventFlag("eclipse", 1);
	}
	else
	{
		quest::CQuestManager::instance().RequestSetEventFlag("eclipse", 0);
	}
}

ACMD(do_event_helper)
{
	char arg1[256];
	int mode = 0;

	one_argument(argument, arg1, sizeof(arg1));
	str_to_number(mode, arg1);

	if (mode == 1)
	{
		xmas::SpawnEventHelper(true);
		ch->ChatPacket(CHAT_TYPE_INFO, "Event Helper Spawn");
	}
	else
	{
		xmas::SpawnEventHelper(false);
		ch->ChatPacket(CHAT_TYPE_INFO, "Event Helper Delete");
	}
}

struct FMobCounter
{
	int nCount;

	void operator () (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER pChar = static_cast<LPCHARACTER>(ent);

			if (pChar->IsMonster() == true || pChar->IsStone())
			{
				nCount++;
			}
		}
	}
};

ACMD(do_get_mob_count)
{
	LPSECTREE_MAP pSectree = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());

	if (pSectree == NULL)
		return;

	FMobCounter f;
	f.nCount = 0;

	pSectree->for_each(f);

	ch->ChatPacket(CHAT_TYPE_INFO, "MapIndex: %d MobCount %d", ch->GetMapIndex(), f.nCount);
}

ACMD(do_clear_land)
{
	const building::CLand* pLand = building::CManager::instance().FindLand(ch->GetMapIndex(), ch->GetX(), ch->GetY());

	if( NULL == pLand )
	{
		return;
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "Guild Land(%d) Cleared", pLand->GetID());

	building::CManager::instance().ClearLand(pLand->GetID());
}

ACMD(do_special_item)
{
    ITEM_MANAGER::instance().ConvSpecialDropItemFile();
}

ACMD(do_drop_info)
{
	char arg1[256];
	DWORD mobVnum = 0;

	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		str_to_number(mobVnum, arg1);
	}
	else
	{
		LPCHARACTER pkTarget = ch->GetTarget();
		if (pkTarget && pkTarget->IsNPC()) {
			mobVnum = pkTarget->GetRaceNum();
		}
	}

	const CMob* mob = CMobManager::instance().Get(mobVnum);
	if (mob == NULL)
	{
		ITEM_MANAGER::instance().ListSpecialItemDrop(ch, mobVnum);
		//ch->ChatPacket(CHAT_TYPE_INFO, "mob with vnum %d does not exist.", mobVnum);
		return;
	}
	ITEM_MANAGER::instance().ListMobItemDrop(ch, mobVnum);
}

ACMD(do_mob_count)
{
	char arg1[256];
	DWORD mobVnum = 0;

	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		str_to_number(mobVnum, arg1);

		DWORD count = CHARACTER_MANAGER::instance().CountRace(mobVnum, ch->GetMapIndex(), ch);
		ch->ChatPacket(CHAT_TYPE_INFO, "Race (%d) count: %d", mobVnum, count);
	}
}

ACMD(do_set_stat)
{
	char szName [256];
	char szChangeAmount[256];

	two_arguments (argument, szName, sizeof (szName), szChangeAmount, sizeof(szChangeAmount));

	if (*szName == '\0' || *szChangeAmount == '\0')
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "Invalid argument.");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(szName);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(szName);

		if (pkCCI)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, "Cannot find player(%s). %s is not in your game server.", szName, szName);
			return;
		}
		else
		{
			ch->ChatPacket (CHAT_TYPE_INFO, "Cannot find player(%s). Perhaps %s doesn't login or exist.", szName, szName);
			return;
		}
	}
	else
	{
		if (tch->IsPolymorphed())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change your status while you are transformed."));
			return;
		}

		if (subcmd != POINT_HT && subcmd != POINT_IQ && subcmd != POINT_ST && subcmd != POINT_DX)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Suborder or the Order is incorrect."));
			return;
		}
		int nRemainPoint = tch->GetPoint(POINT_STAT);
		int nCurPoint = tch->GetRealPoint(subcmd);
		int nChangeAmount = 0;
		str_to_number(nChangeAmount, szChangeAmount);
		int nPoint = nCurPoint + nChangeAmount;

		int n = -1;
		switch (subcmd)
		{
		case POINT_HT:
			if (nPoint < JobInitialPoints[tch->GetJob()].ht)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot set stat under initial stat."));
				return;
			}
			n = 0;
			break;
		case POINT_IQ:
			if (nPoint < JobInitialPoints[tch->GetJob()].iq)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot set stat under initial stat."));
				return;
			}
			n = 1;
			break;
		case POINT_ST:
			if (nPoint < JobInitialPoints[tch->GetJob()].st)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot set stat under initial stat."));
				return;
			}
			n = 2;
			break;
		case POINT_DX:
			if (nPoint < JobInitialPoints[tch->GetJob()].dx)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot set stat under initial stat."));
				return;
			}
			n = 3;
			break;
		}

		if (nPoint > 90)
		{
			nChangeAmount -= nPoint - 90;
			nPoint = 90;
		}

		if (nRemainPoint < nChangeAmount)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Remaining status points are too low."));
			return;
		}

		tch->SetRealPoint(subcmd, nPoint);
		tch->SetPoint(subcmd, tch->GetPoint(subcmd) + nChangeAmount);
		tch->ComputePoints();
		tch->PointChange(subcmd, 0);

		tch->PointChange(POINT_STAT, -nChangeAmount);
		tch->ComputePoints();

		const char* stat_name[4] = {"con", "int", "str", "dex"};
		if (-1 == n)
			return;
		ch->ChatPacket(CHAT_TYPE_INFO, "%s's %s change %d to %d", szName, stat_name[n], nCurPoint, nPoint);
	}
}

ACMD(do_get_item_id_list)
{
	for (int i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; i++)
	{
		LPITEM item = ch->GetInventoryItem(i);
		if (item != NULL)
			ch->ChatPacket(CHAT_TYPE_INFO, "cell : %d, name : %s, id : %d", item->GetCell(), item->GetName(), item->GetID());
	}
}

ACMD(do_set_socket)
{
	char arg1 [256];
	char arg2 [256];
	char arg3 [256];

	one_argument (two_arguments (argument, arg1, sizeof (arg1), arg2, sizeof(arg2)), arg3, sizeof (arg3));

	int item_id, socket_num, value;
	if (!str_to_number (item_id, arg1) || !str_to_number (socket_num, arg2) || !str_to_number (value, arg3))
		return;

	LPITEM item = ITEM_MANAGER::instance().Find (item_id);
	if (item)
		item->SetSocket (socket_num, value, true, "SOCKET_GM");
}

ACMD (do_can_dead)
{
	if (subcmd)
		ch->SetArmada();
	else
		ch->ResetArmada();
}

ACMD (do_all_skill_master)
{
	ch->SetHorseLevel(SKILL_MAX_LEVEL);
	for (int i = 0; i < SKILL_MAX_NUM; i++)
	{
		if (true == ch->CanUseSkill(i))
		{
			switch(i)
			{
				// @fixme154 BEGIN
				// taking out the it->second->bMaxLevel from map_pkSkillProto (&& 1==40|SKILL_MAX_LEVEL) will be very resource-wasting, so we go full ugly so far
				case SKILL_COMBO:
					ch->SetSkillLevel(i, 2);
					break;
				case SKILL_LANGUAGE1:
				case SKILL_LANGUAGE2:
				case SKILL_LANGUAGE3:
					ch->SetSkillLevel(i, 20);
					break;
				case SKILL_HORSE_SUMMON:
					ch->SetSkillLevel(i, 10);
					break;
				case SKILL_HORSE:
					ch->SetSkillLevel(i, HORSE_MAX_LEVEL);
					break;
				// CanUseSkill will be true for skill_horse_skills if riding
				case SKILL_HORSE_WILDATTACK:
				case SKILL_HORSE_CHARGE:
				case SKILL_HORSE_ESCAPE:
				case SKILL_HORSE_WILDATTACK_RANGE:
					ch->SetSkillLevel(i, 20);
					break;
				// @fixme154 END
				default:
					ch->SetSkillLevel(i, SKILL_MAX_LEVEL);
					break;
			}
		}
		else
		{
			switch(i)
			{
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:
				ch->SetSkillLevel(i, 20); // @fixme154 40 -> 20
				break;
			}
		}
	}
	ch->ComputePoints();
	ch->SkillLevelPacket();
}

ACMD (do_item_full_set)
{
	BYTE job = ch->GetJob();
	LPITEM item;
	for (int i = 0; i < 6; i++)
	{
		item = ch->GetWear(i);
		if (item != NULL)
			ch->UnequipItem(item);
	}
	item = ch->GetWear(WEAR_SHIELD);
	if (item != NULL)
		ch->UnequipItem(item);

	switch (job)
	{
	case JOB_SURA:
		{
			item = ITEM_MANAGER::instance().CreateItem(11699);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(13049);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(15189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(12529 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(14109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(17209 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(16209 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
		}
		break;
	case JOB_WARRIOR:
		{
			item = ITEM_MANAGER::instance().CreateItem(11299);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(13049);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(15189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(3159 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(12249 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(14109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(17109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(16109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
		}
		break;
	case JOB_SHAMAN:
		{
			item = ITEM_MANAGER::instance().CreateItem(11899);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(13049);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(15189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(7159 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(12669 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(14109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(17209 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(16209 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
		}
		break;
	case JOB_ASSASSIN:
		{
			item = ITEM_MANAGER::instance().CreateItem(11499);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(13049);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(15189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(1139 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(12389 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(14109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(17189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(16189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
		}
		break;
#ifdef ENABLE_WOLFMAN_CHARACTER
	case JOB_WOLFMAN:
		{
			item = ITEM_MANAGER::instance().CreateItem(21049);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(13049);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(15189);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(6049);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(21559);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(14109);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(17209);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(16209);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
		}
		break;
#endif
	}
	ch->ComputePoints(); //@fixme300
}

ACMD (do_attr_full_set)
{
	BYTE job = ch->GetJob();
	LPITEM item;

	switch (job)
	{
	case JOB_WARRIOR:
	case JOB_ASSASSIN:
	case JOB_SURA:
	case JOB_SHAMAN:
#ifdef ENABLE_WOLFMAN_CHARACTER
	case JOB_WOLFMAN:
#endif
	{
		// 무사 몸빵 셋팅.
		// 이것만 나와 있어서 임시로 모든 직군 다 이런 속성 따름.
		item = ch->GetWear(WEAR_HEAD);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, POINT_ATT_SPEED, 8);
			item->SetForceAttribute(1, POINT_HP_REGEN, 10);
			item->SetForceAttribute(2, POINT_SP_REGEN, 10);
			item->SetForceAttribute(3, POINT_DODGE, 15);
			item->SetForceAttribute(4, POINT_STEAL_SP, 10);
		}

		item = ch->GetWear(WEAR_WEAPON);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, POINT_CASTING_SPEED, 20);
			item->SetForceAttribute(1, POINT_CRITICAL_PCT, 10);
			item->SetForceAttribute(2, POINT_PENETRATE_PCT, 10);
			item->SetForceAttribute(3, POINT_ATTBONUS_DEVIL, 20);
			item->SetForceAttribute(4, POINT_ST, 12);
		}

		item = ch->GetWear(WEAR_SHIELD);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, POINT_HT, 12);
			item->SetForceAttribute(1, POINT_BLOCK, 15);
			item->SetForceAttribute(2, POINT_REFLECT_MELEE, 10);
			item->SetForceAttribute(3, POINT_IMMUNE_STUN, 1);
			item->SetForceAttribute(4, POINT_IMMUNE_SLOW, 1);
		}

		item = ch->GetWear(WEAR_BODY);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, POINT_MAX_HP, 1500);
			item->SetForceAttribute(1, POINT_CASTING_SPEED, 20);
			item->SetForceAttribute(2, POINT_STEAL_HP, 10);
			item->SetForceAttribute(3, POINT_REFLECT_MELEE, 10);
			item->SetForceAttribute(4, POINT_ATT_GRADE_BONUS, 50);
		}

		item = ch->GetWear(WEAR_FOOTS);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, POINT_MAX_HP, 1500);
			item->SetForceAttribute(1, POINT_MAX_SP, 80);
			item->SetForceAttribute(2, POINT_MOV_SPEED, 8);
			item->SetForceAttribute(3, POINT_ATT_SPEED, 8);
			item->SetForceAttribute(4, POINT_CRITICAL_PCT, 10);
		}

		item = ch->GetWear(WEAR_WRIST);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, POINT_MAX_HP, 1500);
			item->SetForceAttribute(1, POINT_MAX_SP, 80);
			item->SetForceAttribute(2, POINT_PENETRATE_PCT, 10);
			item->SetForceAttribute(3, POINT_STEAL_HP, 10);
			item->SetForceAttribute(4, POINT_MANA_BURN_PCT, 10);
		}
		item = ch->GetWear(WEAR_NECK);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, POINT_MAX_HP, 1500);
			item->SetForceAttribute(1, POINT_MAX_SP, 80);
			item->SetForceAttribute(2, POINT_CRITICAL_PCT, 10);
			item->SetForceAttribute(3, POINT_PENETRATE_PCT, 10);
			item->SetForceAttribute(4, POINT_STEAL_SP, 10);
		}
		item = ch->GetWear(WEAR_EAR);
		if (item != NULL)
		{
			item->ClearAttribute();
			item->SetForceAttribute(0, POINT_MOV_SPEED, 20);
			item->SetForceAttribute(1, POINT_MANA_BURN_PCT, 10);
			item->SetForceAttribute(2, POINT_POISON_REDUCE, 5);
			item->SetForceAttribute(3, POINT_ATTBONUS_DEVIL, 20);
			item->SetForceAttribute(4, POINT_ATTBONUS_UNDEAD, 20);
		}
	}
		break;
	}
	ch->ComputePoints(); //@fixme300
}

ACMD (do_full_set)
{
	do_all_skill_master(ch, NULL, 0, 0);
	do_item_full_set(ch, NULL, 0, 0);
	do_attr_full_set(ch, NULL, 0, 0);
}

ACMD (do_use_item)
{
	char arg1 [256];

	one_argument (argument, arg1, sizeof (arg1));

	int cell = 0;
	str_to_number(cell, arg1);

	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
	{
		ch->UseItem(TItemPos (INVENTORY, cell));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "아이템이 없어서 착용할 수 없어.");
	}
}

ACMD (do_clear_affect)
{
	ch->ClearAffect(true);
}

ACMD (do_dragon_soul)
{
	char arg1[512];
	const char* rest = one_argument (argument, arg1, sizeof(arg1));
	switch (arg1[0])
	{
	case 'a':
		{
			one_argument (rest, arg1, sizeof(arg1));
			int deck_idx;
			if (str_to_number(deck_idx, arg1) == false)
			{
				return;
			}
			ch->DragonSoul_ActivateDeck(deck_idx);
		}
		break;
	case 'd':
		{
			ch->DragonSoul_DeactivateAll();
		}
		break;
	}
}

ACMD (do_ds_list)
{
	for (int i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; i++)
	{
		TItemPos cell(DRAGON_SOUL_INVENTORY, i);

		LPITEM item = ch->GetItem(cell);
		if (item != NULL)
			ch->ChatPacket(CHAT_TYPE_INFO, "cell : %d, name : %s, id : %d", item->GetCell(), item->GetName(), item->GetID());
	}
}
#ifdef ENABLE_IKASHOP_RENEWAL
ACMD(do_offshop_force_close_shop) {
	char arg1[50];
	argument = one_argument(argument, arg1, sizeof(arg1));
	if (arg1[0] != 0 && isdigit(arg1[0])) {

		DWORD id = 0;
		str_to_number(id, arg1);

		if (id == 0) {
			ch->ChatPacket(CHAT_TYPE_INFO, "syntax : offshop_force_close_shop  <player-id>  ");
			return;
		}
		else {
			auto pkShop = ikashop::GetManager().GetShopByOwnerID(id);
			if (!pkShop) {
				ch->ChatPacket(CHAT_TYPE_INFO, "Cannot find shop by id %u ", id);
				return;
			}
			else {
				ikashop::GetManager().SendShopForceSoftCloseDBPacket(id, true);
				ch->ChatPacket(CHAT_TYPE_INFO, "shop closed successfully.");
			}
		}

	} else {
		ch->ChatPacket(CHAT_TYPE_INFO, "syntax : offshop_force_close_shop  <player-id>  ");
		return;
	}
}
#endif

ACMD(do_special_spawn_info)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int vnum;
	str_to_number(vnum, arg1);

	std::map<DWORD, LPSPECIAL_SPAWN>& spawnsMap = SpecialSpawnManager::instance().m_map_specialSpawns;
	auto it = spawnsMap.find(vnum);
	if (it == spawnsMap.end())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Given spawn special vnum does not exist.");
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Vnum %d, timeLeft: %d", vnum, it->second->GetTimeLeft());
	}
}


ACMD(do_set_special_flag)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1 && *arg2)
	{
		if (!isdigit(*arg2))
			return;

		int value = 0;
		str_to_number(value, arg2);

		ch->SetSpecialFlag(arg1, value, false);
	}
}

ACMD(do_get_special_flag)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		std::string flag = std::string(arg1);
		ch->ChatPacket(CHAT_TYPE_INFO, "special flag %s value %d", flag.c_str(), ch->GetSpecialFlag(arg1));
	}
}

ACMD(do_maintenance)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		if (!isdigit(*arg1))
			return;

		int value = 0;
		str_to_number(value, arg1);
		if (value)
		{
			g_bIsMaintenance = true;
			ch->ChatPacket(CHAT_TYPE_INFO, "Maintenance ON!");
			ch->ChatPacket(CHAT_TYPE_INFO, "This command blocks only logging in. It doesn't disconnect connected players.");
		}
		else
		{
			g_bIsMaintenance = false;
			ch->ChatPacket(CHAT_TYPE_INFO, "Maintenance OFF!");
			ch->ChatPacket(CHAT_TYPE_INFO, "Players can log in.");
		}

		TPacketGGMaintenance packet;
		packet.bHeader = HEADER_GG_MAINTENANCE;
		packet.bState = value;
		P2P_MANAGER::instance().Send(&packet, sizeof(TPacketGGMaintenance));

		TPacketGDMaintenance db_packet;
		db_packet.bHeader = HEADER_GD_MAINTENANCE;
		db_packet.bState = value;
		db_clientdesc->DBPacket(HEADER_GD_MAINTENANCE, ch->GetDesc()->GetHandle(), &db_packet, sizeof(TPacketGDMaintenance));
	}
}

ACMD(do_spy)
{
	if (ch->IsEntityViewSendable())
	{
		ch->SetEntityViewSendable(false);
		ch->RemoveViewAllViewers();
		ch->SetSpy(true);
		ch->AddAffect(AFFECT_SPY, POINT_NONE, 0, AFF_INVISIBILITY, INFINITE_AFFECT_DURATION, 0, true);
		ch->SetSpecialFlag("admin_spy", 1, false);
		ch->SetArmada();
		ch->ChatPacket(CHAT_TYPE_INFO, "Entered SPY mode.");
	}
	else
	{
		ch->SetSpy(false);
		ch->SetEntityViewSendable(true);
		ch->RemoveAffect(AFFECT_SPY);
		ch->ResetArmada();
		ch->SetSpecialFlag("admin_spy", 0, false);
		ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ(), true);
		ch->ChatPacket(CHAT_TYPE_INFO, "Spy mode left.");
	}

}

#include "messenger_manager.h"
ACMD(do_test_until_time)
{
	if (!test_server)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	MessengerManager::instance().Block(ch, arg1);
}

ACMD(do_test_hack)
{
	if (!test_server)
		return;

	for (auto it : CHARACTER_MANAGER::Instance().GetCharacterVIDMap())
	{
		LPCHARACTER mob = it.second;
		if (mob && mob->IsNPC())
		{
			if (ch->DistanceTo(mob) > 2000)
				continue;

			ch->SetTarget(mob);
			ch->MainFlyTarget(mob->GetVID(), ch->GetX(), ch->GetY());
			ch->Shoot(0);
		}
	}

	auto target = ch->GetTarget();
	if (!target)
		return;

	ch->ChatDebug("uzywamy!");
	//ch->MainFlyTarget(target->GetVID(), ch->GetX(), ch->GetY());

	ch->UseSkill(SKILL_POISON_CLOUD, target);

	for (int i = 0; i < 5; i++)
		ch->Attack(target, SKILL_POISON_CLOUD, 35);
		//ch->Shoot(SKILL_POISON_CLOUD);
}

ACMD(do_special_shop_meta)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (isdigit(*arg1))
	{
		DWORD shopVnum = 0;
		str_to_number(shopVnum, arg1);
		auto meta = CSpecialShopManager::instance().GetShopMetaData(shopVnum);

		auto deltaTime = get_global_time() - meta.lastUpdate;
		ch->ChatPacket(CHAT_TYPE_INFO, "special shop (vnum %d) seed %d update time %d (delta %d)", shopVnum, meta.seed, meta.lastUpdate, deltaTime);
		ch->ChatPacket(CHAT_TYPE_INFO, "current duration: %s", seconds_to_smart_time(deltaTime));

		if (shopVnum == 101) // deviltower
		{
			auto left_time = get_time_until(5, 0, meta.lastUpdate);
			ch->ChatPacket(CHAT_TYPE_INFO, "Additional Deviltower Shop info");
			ch->ChatPacket(CHAT_TYPE_INFO, "left time for update %s (%d)", seconds_to_smart_time(left_time), left_time);
		}
	}
}

ACMD(do_captcha)
{
	return;
}

ACMD(do_block) {
	char arg1[256];
	char arg2[256];
	char arg3[256];

	one_argument(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));

	if (!isdigit(*arg1) || !isdigit(*arg2))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: /block_player <pid> <duration> optional: <reason>");
		return;
	}

	DWORD pid = 0;
	str_to_number(pid, arg1);

	DWORD duration = 0;
	str_to_number(duration, arg2);

	std::string reason(arg3);
	std::ranges::replace(reason, '_', ' ');

	if (reason.length() < 2)
		reason = "NO_REASON";

	auto data = BanManager::instance().GetBlockPlayerData(pid);

	if (data.pid == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No player with pid %d found.", pid);
		return;
	}

	if (data.pid == ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Can't ban yourself.");
		return;
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "Ban requested for player %s.", data.name);
	bool isBlock = BanManager::instance().Block(data.aid, data.pid, duration, ch->GetPlayerID(), reason, true);

	if (!isBlock)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Player %s is already on block list. Flush if you want to execute ban /block_flush", data.name);
		return;
	}
}

ACMD(do_block_hwid)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!isdigit(*arg1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: /block_hwid <aid>");
		return;
	}

	DWORD aid = 0;
	str_to_number(aid, arg1);

	auto data = BanManager::instance().GetBlockHwidData(aid);
	if (std::strlen(data.hwid) < 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Account with id %d not found.", aid);
		return;
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "HWID ban requested.");
	BanManager::instance().BlockHWID(data.hwid);
}

ACMD(do_block_flush)
{
	int count = BanManager::instance().FlushBlock();
	ch->ChatPacket(CHAT_TYPE_INFO, "Flushed account block. Count %d.", count);
}

ACMD(do_block_pool)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];

	one_argument(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));

	if (!isdigit(*arg1) || !isdigit(*arg2))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: /block_pool <pid> <duration> optional: <reason>");
		return;
	}

	DWORD pid = 0;
	str_to_number(pid, arg1);

	DWORD duration = 0;
	str_to_number(duration, arg2);

	std::string reason(arg3);
	std::ranges::replace(reason, '_', ' ');
	if (reason.length() < 2)
		reason = "NO_REASON";

	auto data = BanManager::instance().GetBlockPlayerData(pid);

	if (data.pid == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No player with pid %d found.", pid);
		return;
	}

	if (data.pid == ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Can't ban yourself.");
		return;
	}
	
	if (!BanManager::instance().Block(data.aid, data.pid, duration, ch->GetPlayerID(), reason, false))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Player %s is already on block list. Flush if you want to execute ban /block_flush", data.name);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Ban pool request for player %s.", data.name);
	}
}

ACMD(do_usage)
{
	TPacketGDUsage pack{};
	pack.invokerPid = ch->GetPlayerID();
	db_clientdesc->DBPacket(HEADER_GD_USAGE, 0, &pack, sizeof(pack));
}

ACMD(do_map_spawn_delay)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];

	one_argument(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));
	int map_index = 0, value = 0;


	if (!*arg1 || !*arg2 || !*arg3)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: map_spawn_delay <type ('boss' or 'normal')> <map index> <0-100 in % (0 to off)>");
		return;
	}

	if (isnhdigit(*arg2) && isnhdigit(*arg3))
	{
		str_to_number(map_index, arg2);
		str_to_number(value, arg3);

		std::string flagName = "fastMobSpawn";
		if (strcmp(arg1, "boss") == 0)
		{
			flagName = "fastBossSpawn";
			if (value > 0 && value < 30)
				value = 30;
		}

		flagName += std::to_string(map_index);

		if (value >= 100)
			value = 0;

		if (value < 0)
			value = 0;

		quest::CQuestManager::instance().RequestSetEventFlag(flagName, value);
		if (value > 0)
			ch->ChatPacket(CHAT_TYPE_INFO, "Map %d set spawn delay to %d%% of original value.", map_index, value);
		else
			ch->ChatPacket(CHAT_TYPE_INFO, "Map %d set spawn delay original values.", map_index);
	}
}

ACMD(do_map_spawn_delay_list)
{
	ch->ChatPacket(CHAT_TYPE_INFO, "Does not take indices above 109.");

	for (int i = 0; i < 110; i++)
	{
		std::string normalFlagName = "fastMobSpawn" + std::to_string(i);
		std::string bossFlagName = "fastBossSpawn" + std::to_string(i);

		int normalVal = quest::CQuestManager::instance().GetEventFlag(normalFlagName);
		int bossVal = quest::CQuestManager::instance().GetEventFlag(bossFlagName);

		if (normalVal || bossVal)
			ch->ChatPacket(CHAT_TYPE_INFO, "### SPAWN DELAYS FOR MAP INDEX %d ###", i);

		if (normalVal)
			ch->ChatPacket(CHAT_TYPE_INFO, "    Normal spawn: %d%%", normalVal);
		
		if (bossVal)
			ch->ChatPacket(CHAT_TYPE_INFO, "    Boss spawn: %d%%", bossVal);
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "End of list.");
}

ACMD(do_check_mob)
{
	LPCHARACTER target = ch->GetTarget();
	if (!target)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "no target.");
		return;
	}
	DWORD targetVID = static_cast<DWORD>(target->GetVID());
	ch->ChatPacket(CHAT_TYPE_INFO, "### Target Info (vnum: %d) (vid %d) (hp %d/%d) ###", target->GetRaceNum(), targetVID, target->GetHP(), target->GetMaxHP());

	std::string state = "unknown";
	if (target->IsStateIdle())
		state = "IDLE";

	if (target->IsStateBattle())
		state = "BATTLE";

	if (target->IsStateMove())
		state = "MOVE";

	ch->ChatPacket(CHAT_TYPE_INFO, "state: %s position: %d", state.c_str(), target->GetPosition());

	if (const LPCHARACTER targetTarget = target->GetVictim())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "HasTarget: vid %d", static_cast<DWORD>(targetTarget->GetVID()));
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "######");
}

// ===========================================================================
// Panel GM (F9) + "Zapisane miejsca" - komendy serwera
//
// Wklej caly ten plik na koniec game/src/cmd_gm.cpp, a deklaracje i wiersze
// tabeli z cmd_cpp_fragment.txt do game/src/cmd.cpp.
// ===========================================================================

// ============================================================================

// Panel GM (F9) - port z linii r40250 (commit 8fd32b84) na mt2009, 2026-09-12.

// Kazda komenda nizej jest sprawdzana przez gm_level w tabeli cmd.cpp.

// ============================================================================

EVENTFUNC(gmpanel_kick_self_event)

{

	char_event_info* info = dynamic_cast<char_event_info*>( event->info );

	if (info == NULL)

		return 0;



	LPCHARACTER targetCh = info->ch;

	if (targetCh == NULL)

		return 0;



	LPDESC targetDesc = targetCh->GetDesc();

	if (targetDesc)

		DESC_MANAGER::instance().DestroyDesc(targetDesc);

	return 0;

}



// ---- GM panel (F9) by OskarPWA: every action is a text command checked against the

// caller's gm level in cmd_info[]; the client only sends and shows. ----

static void SendGMPanelMobListChunks(LPCHARACTER ch, int typeFilter)

{

	char query[192];

	snprintf(query, sizeof(query),

			"SELECT vnum, locale_name FROM player.mob_proto WHERE type = %d "

			"AND locale_name NOT LIKE '%%?%%' AND locale_name <> '' ORDER BY vnum", typeFilter);

	std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(query));



	char chunk[400];

	int chunkLen = 0;

	chunk[0] = '\0';



	if (msg.get() && msg->uiSQLErrno == 0 && msg->Get() && msg->Get()->pSQLResult)

	{

		MYSQL_ROW row;

		while ((row = mysql_fetch_row(msg->Get()->pSQLResult)) != NULL)

		{

			if (!row[0] || !row[1])

				continue;



			char nameBuf[32];

			strlcpy(nameBuf, row[1], sizeof(nameBuf));

			for (char* p = nameBuf; *p; ++p)

			{

				if (*p == ' ' || *p == '|' || *p == ';' || *p == ':')

					*p = '_';

			}



			char entry[48];

			snprintf(entry, sizeof(entry), "%s:%s;", row[0], nameBuf);

			size_t entryLen = strlen(entry);



			if (chunkLen + (int)entryLen >= (int)sizeof(chunk) - 1)

			{

				ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelItemListChunk 0|%s", chunk);

				chunkLen = 0;

				chunk[0] = '\0';

			}



			memcpy(chunk + chunkLen, entry, entryLen + 1);

			chunkLen += (int)entryLen;

		}

	}



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelItemListChunk 1|%s", chunk);

}



ACMD(do_gmpanel_lookup)

{

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));



	if (!*arg1)

	{

		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: gmpanel_lookup <nick>");

		return;

	}

	// The name reaches DirectQuery unescaped below (this codebase's DB

	// helper has no bound-parameter form) - refuse anything that could

	// break out of the quoted literal instead of trying to escape it.

	for (const char* p = arg1; *p; ++p)

	{

		if (*p == '\'' || *p == '"' || *p == '\\')

		{

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelLookupResult ERR_BADNAME");

			return;

		}

	}



	// If the target is online, the player.player row is only a snapshot

	// from their last save - pull live values straight off the CHARACTER

	// instead so stats like skill points aren't stale.

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (tch && tch->GetDesc())

	{

		char response[1024];

		snprintf(response, sizeof(response),

				"GMPanelLookupResult %s|%d|%d|%d|%d|%u|%lld|%d|%d|%d|%d|%d|%d|%d|%ld|%s|%d|%d|%s|%s|%d|%d",

				tch->GetName(), tch->GetLevel(), (int)tch->GetJob(), tch->GetHP(), tch->GetSP(),

				// mt2009: GetGold() zwraca YANG (64 bit) - z %d rozjezdza sie lista

				// argumentow i kolejne %s czytaja smieci (segfault w snprintf, 2026-09-12).

				tch->GetExp(), (long long)tch->GetGold(),

				tch->GetPoint(POINT_ST), tch->GetPoint(POINT_HT), tch->GetPoint(POINT_DX), tch->GetPoint(POINT_IQ),

				tch->GetPoint(POINT_STAT), tch->GetPoint(POINT_SKILL), tch->GetPoint(POINT_SUB_SKILL),

				(long)tch->GetMapIndex(), "ONLINE",

				(int)tch->GetHorseLevel(), tch->GetPoint(POINT_HORSE_SKILL),

				tch->GetDesc()->GetAccountTable().login, tch->GetDesc()->GetHostName(),

				tch->GetRealPoint(POINT_PLAYTIME), tch->GetAlignment() / 10);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "%s", response);

		return;

	}



	char query[512];

	snprintf(query, sizeof(query),

			"SELECT p.name, p.level, p.job, p.hp, p.mp, p.exp, p.gold, "

			"p.st, p.ht, p.dx, p.iq, p.stat_point, p.skill_point, p.sub_skill_point, "

			"p.map_index, DATE_FORMAT(p.last_play, '%%Y-%%m-%%d_%%H:%%i'), p.horse_level, p.horse_skill_point, "

			"a.login, p.ip, p.playtime, p.alignment / 10 "

			"FROM player.player p JOIN account.account a ON a.id = p.account_id "

			"WHERE p.name = '%s' LIMIT 1", arg1);

	std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(query));

	if (!msg.get() || msg->uiSQLErrno != 0 || !msg->Get() || !msg->Get()->pSQLResult ||

			msg->Get()->uiNumRows == 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelLookupResult ERR_NOTFOUND|%s", arg1);

		return;

	}

	MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

	if (!row)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelLookupResult ERR_NOTFOUND|%s", arg1);

		return;

	}

	char response[1024];

	snprintf(response, sizeof(response),

			"GMPanelLookupResult %s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s",

			row[0] ? row[0] : "", row[1] ? row[1] : "0", row[2] ? row[2] : "0",

			row[3] ? row[3] : "0", row[4] ? row[4] : "0", row[5] ? row[5] : "0",

			row[6] ? row[6] : "0", row[7] ? row[7] : "0", row[8] ? row[8] : "0",

			row[9] ? row[9] : "0", row[10] ? row[10] : "0", row[11] ? row[11] : "0",

			row[12] ? row[12] : "0", row[13] ? row[13] : "0", row[14] ? row[14] : "0",

			row[15] ? row[15] : "0", row[16] ? row[16] : "0", row[17] ? row[17] : "0",

			row[18] ? row[18] : "", row[19] ? row[19] : "0", row[20] ? row[20] : "0",

			row[21] ? row[21] : "0");

	ch->ChatPacket(CHAT_TYPE_COMMAND, "%s", response);

}



// "Utworz Przedmiot" tab of the F9 GM panel (client: GMPanel page_createitem

// in interfacemodule.py). Single pipe-delimited argument, no spaces anywhere

// in it - the client->server chat command parser (one_argument) splits on

// whitespace same as the client's own server->client command dispatcher, so

// any field containing a space would truncate the rest silently.

// Layout (23 fields): owner|vnum|count|s0..s5|(t0,v0)..(t6,v6)

// Panel GM "Utworz Przedmiot": klient (GM_PANEL_APPLY_SUFFIXES w interfacemodule.py)

// wysyla klasyczne numery APPLY_* (1=MAX_HP ... 5=STR ... 91=ANTI_PENETRATE_PCT).

// mt2009 nie ma enumu APPLY_* - typ bonusu na przedmiocie to indeks POINT_*

// (playerbot_engine_compat.h mapuje stare nazwy na POINT_*). Wpisanie klasycznego

// numeru wprost dawalo np. STR=5 -> POINT_HP -> w tooltipie "UNKNOWN_TYPE[5]".

// 0 = brak odpowiednika w tym silniku (bonus pomijany).

static BYTE GMPanelClassicApplyToPoint(int classicApply)

{

	static const BYTE s_aTable[] = {

		POINT_NONE, POINT_MAX_HP, POINT_MAX_SP, POINT_HT,

		POINT_IQ, POINT_ST, POINT_DX, POINT_ATT_SPEED,

		POINT_MOV_SPEED, POINT_CASTING_SPEED, POINT_HP_REGEN, POINT_SP_REGEN,

		POINT_POISON_PCT, POINT_STUN_PCT, POINT_SLOW_PCT, POINT_CRITICAL_PCT,

		POINT_PENETRATE_PCT, POINT_ATTBONUS_HUMAN, POINT_ATTBONUS_ANIMAL, POINT_ATTBONUS_ORC,

		POINT_ATTBONUS_MILGYO, POINT_ATTBONUS_UNDEAD, POINT_ATTBONUS_DEVIL, POINT_STEAL_HP,

		POINT_STEAL_SP, POINT_MANA_BURN_PCT, POINT_DAMAGE_SP_RECOVER, POINT_BLOCK,

		POINT_DODGE, POINT_RESIST_SWORD, POINT_RESIST_TWOHAND, POINT_RESIST_DAGGER,

		POINT_RESIST_BELL, POINT_RESIST_FAN, POINT_RESIST_BOW, POINT_RESIST_FIRE,

		POINT_RESIST_ELEC, POINT_RESIST_MAGIC, POINT_RESIST_WIND, POINT_REFLECT_MELEE,

		POINT_NONE, POINT_POISON_REDUCE, POINT_KILL_SP_RECOVER, POINT_EXP_DOUBLE_BONUS,

		POINT_GOLD_DOUBLE_BONUS, POINT_ITEM_DROP_BONUS, POINT_POTION_BONUS, POINT_KILL_HP_RECOVERY,

		POINT_IMMUNE_STUN, POINT_IMMUNE_SLOW, POINT_IMMUNE_FALL, POINT_SKILL,

		POINT_BOW_DISTANCE, POINT_ATT_GRADE_BONUS, POINT_DEF_GRADE_BONUS, POINT_MAGIC_ATT_GRADE,

		POINT_MAGIC_DEF_GRADE, POINT_CURSE_PCT, POINT_MAX_STAMINA, POINT_ATTBONUS_WARRIOR,

		POINT_ATTBONUS_ASSASSIN, POINT_ATTBONUS_SURA, POINT_ATTBONUS_SHAMAN, POINT_ATTBONUS_MONSTER,

		POINT_MALL_ATTBONUS, POINT_MALL_DEFBONUS, POINT_MALL_EXPBONUS, POINT_MALL_ITEMBONUS,

		POINT_MALL_GOLDBONUS, POINT_MAX_HP_PCT, POINT_MAX_SP_PCT, POINT_SKILL_DAMAGE_BONUS,

		POINT_NORMAL_HIT_DAMAGE_BONUS, POINT_SKILL_DEFEND_BONUS, POINT_NORMAL_HIT_DEFEND_BONUS, POINT_PC_BANG_EXP_BONUS,

		POINT_PC_BANG_DROP_BONUS, POINT_NONE, POINT_RESIST_WARRIOR, POINT_RESIST_ASSASSIN,

		POINT_RESIST_SURA, POINT_RESIST_SHAMAN, POINT_ENERGY, POINT_DEF_GRADE,

		POINT_COSTUME_ATTR_BONUS, POINT_MAGIC_ATT_BONUS_PER, POINT_MELEE_MAGIC_ATT_BONUS_PER, POINT_RESIST_ICE,

		POINT_RESIST_EARTH, POINT_RESIST_DARK, POINT_RESIST_CRITICAL, POINT_RESIST_PENETRATE,

	};

	if (classicApply <= 0 || classicApply >= (int)(sizeof(s_aTable) / sizeof(s_aTable[0])))

		return POINT_NONE;

	return s_aTable[classicApply];

}



ACMD(do_gmpanel_createitem)

{

	char arg1[512];

	one_argument(argument, arg1, sizeof(arg1));



	if (!*arg1)

	{

		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: gmpanel_createitem <data>");

		return;

	}



	// Layout (24 fields): owner|vnum|count|location|s0..s5|(t0,v0)..(t6,v6)

	// location is one of "inv" (Ekwipunek), "safe" (Magazyn/safebox) or

	// "mall" (Itemshop depot) - both safe/mall need that window already

	// loaded server-side (the owner opened it at least once this session),

	// same constraint the normal client-driven paths have.

	const int FIELD_COUNT = 24;

	char* fields[FIELD_COUNT];

	int fieldCount = 0;



	char* saveptr = NULL;

	char* tok = strtok_r(arg1, "|", &saveptr);

	while (tok && fieldCount < FIELD_COUNT)

	{

		fields[fieldCount++] = tok;

		tok = strtok_r(NULL, "|", &saveptr);

	}



	if (fieldCount != FIELD_COUNT)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelCreateItemResult ERR_BADDATA");

		return;

	}



	LPCHARACTER owner = CHARACTER_MANAGER::instance().FindPC(fields[0]);

	if (!owner)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelCreateItemResult ERR_OWNER_OFFLINE");

		return;

	}



	DWORD dwVnum = (DWORD)strtoul(fields[1], NULL, 10);

	int iCount = MINMAX(1, atoi(fields[2]), ITEM_MAX_COUNT);

	const char* location = fields[3];



	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwVnum, iCount, 0, true);

	if (!item)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelCreateItemResult ERR_BADVNUM");

		return;

	}



	// Klient wysyla 6 pol gniazd, przedmiot ma ITEM_SOCKET_MAX_NUM (3) - dalej

	// SetSocket pisalby poza tablice (asercja skompilowana na release).

	for (int i = 0; i < 6 && i < ITEM_SOCKET_MAX_NUM; ++i)

	{

		long lSocket = atol(fields[4 + i]);

		if (lSocket > 0)

			item->SetSocket(i, lSocket);

	}



	for (int i = 0; i < 7; ++i)

	{

		const BYTE bType = GMPanelClassicApplyToPoint(atoi(fields[10 + i * 2]));

		int sValue = atoi(fields[11 + i * 2]);

		if (bType != POINT_NONE)

			item->SetForceAttribute(i, bType, (short)sValue);

	}



	if (!strcmp(location, "safe"))

	{

		CSafebox* pkSafebox = owner->GetSafebox();

		if (!pkSafebox)

		{

			M2_DESTROY_ITEM(item);

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelCreateItemResult ERR_SAFEBOX_CLOSED");

			return;

		}



		int iEmptyPos = -1;

		const int safeboxSize = owner->GetSafeboxSize();

		for (int pos = 0; pos < safeboxSize; ++pos)

		{

			if (pkSafebox->IsEmpty(pos, item->GetSize()))

			{

				iEmptyPos = pos;

				break;

			}

		}

		if (iEmptyPos == -1)

		{

			M2_DESTROY_ITEM(item);

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelCreateItemResult ERR_NOSPACE");

			return;

		}



		pkSafebox->Add(iEmptyPos, item);

		LogManager::instance().ItemLog(ch, item, "GMPANEL_SAFEBOX", item->GetName());

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelCreateItemResult OK");

		return;

	}

	else if (!strcmp(location, "mall"))

	{

		CSafebox* pkMall = owner->GetMall();

		if (!pkMall)

		{

			M2_DESTROY_ITEM(item);

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelCreateItemResult ERR_MALL_CLOSED");

			return;

		}



		int iEmptyPos = -1;

		for (int pos = 0; pos < SAFEBOX_MAX_NUM; ++pos)

		{

			if (pkMall->IsValidPosition(pos) && pkMall->IsEmpty(pos, item->GetSize()))

			{

				iEmptyPos = pos;

				break;

			}

		}

		if (iEmptyPos == -1)

		{

			M2_DESTROY_ITEM(item);

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelCreateItemResult ERR_NOSPACE");

			return;

		}



		pkMall->Add(iEmptyPos, item);

		LogManager::instance().ItemLog(ch, item, "GMPANEL_MALL", item->GetName());

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelCreateItemResult OK");

		return;

	}



	int iEmptyPos = owner->GetEmptyInventory(item->GetSize());

	if (iEmptyPos == -1)

	{

		M2_DESTROY_ITEM(item);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelCreateItemResult ERR_NOSPACE");

		return;

	}



	item->AddToCharacter(owner, TItemPos(INVENTORY, iEmptyPos));

	LogManager::instance().ItemLog(ch, item, "GMPANEL", item->GetName());



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelCreateItemResult OK");

}



// "Utworz Przedmiot" tab item picker (client: interfacemodule.py

// GMPanelWindow.__FetchItemList). A single ChatPacket is capped at

// CHAT_MAX_LEN (512 bytes, see char.cpp ChatPacket), so a category's full

// item list is sent as a sequence of "GMPanelItemListChunk <isLast>|<data>"

// messages the client concatenates until isLast=1. Spaces/':'/';'/'|' in

// item names are replaced with '_' since they're either the client<->server

// whitespace-split delimiter or our own field separators - the client

// converts '_' back to a space for display.

ACMD(do_gmpanel_itemlist)

{

	char arg1[64];

	one_argument(argument, arg1, sizeof(arg1));



	if (!*arg1)

	{

		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: gmpanel_itemlist <category>");

		return;

	}



	const std::vector<TItemTable>& table = ITEM_MANAGER::instance().GetTable();



	char chunk[400];

	int chunkLen = 0;

	chunk[0] = '\0';



	for (size_t idx = 0; idx < table.size(); ++idx)

	{

		const TItemTable& proto = table[idx];



		bool bMatch = false;

		if (!strcmp(arg1, "weapon"))

			bMatch = (proto.bType == ITEM_WEAPON);

		else if (!strcmp(arg1, "armor_body"))

			bMatch = (proto.bType == ITEM_ARMOR && proto.bSubType == ARMOR_BODY);

		else if (!strcmp(arg1, "armor_head"))

			bMatch = (proto.bType == ITEM_ARMOR && proto.bSubType == ARMOR_HEAD);

		else if (!strcmp(arg1, "armor_shield"))

			bMatch = (proto.bType == ITEM_ARMOR && proto.bSubType == ARMOR_SHIELD);

		else if (!strcmp(arg1, "armor_wrist"))

			bMatch = (proto.bType == ITEM_ARMOR && proto.bSubType == ARMOR_WRIST);

		else if (!strcmp(arg1, "armor_foots"))

			bMatch = (proto.bType == ITEM_ARMOR && proto.bSubType == ARMOR_FOOTS);

		else if (!strcmp(arg1, "armor_ear"))

			bMatch = (proto.bType == ITEM_ARMOR && proto.bSubType == ARMOR_EAR);

		else if (!strcmp(arg1, "armor_neck"))

			bMatch = (proto.bType == ITEM_ARMOR && proto.bSubType == ARMOR_NECK);

		else if (!strcmp(arg1, "material"))

			bMatch = (proto.bType == ITEM_MATERIAL);

		else if (!strcmp(arg1, "stone"))

		{

			bMatch = (strcasestr(proto.szLocaleName, "kamie") != NULL && strcasestr(proto.szLocaleName, "dusz") != NULL);

			if (bMatch && (strstr(proto.szLocaleName, "+6") != NULL || strstr(proto.szLocaleName, "+7") != NULL ||

					strstr(proto.szLocaleName, "+8") != NULL || strstr(proto.szLocaleName, "+9") != NULL))

				bMatch = false;

		}

		else if (!strcmp(arg1, "other"))

		{

			bool weapon = (proto.bType == ITEM_WEAPON);

			bool armorMain = (proto.bType == ITEM_ARMOR && (proto.bSubType == ARMOR_BODY || proto.bSubType == ARMOR_HEAD ||

					proto.bSubType == ARMOR_SHIELD || proto.bSubType == ARMOR_WRIST || proto.bSubType == ARMOR_FOOTS ||

					proto.bSubType == ARMOR_NECK || proto.bSubType == ARMOR_EAR));

			bool material = (proto.bType == ITEM_MATERIAL);

			bMatch = !weapon && !armorMain && !material;

		}



		if (!bMatch)

			continue;



		char nameBuf[ITEM_NAME_MAX_LEN + 1];

		strlcpy(nameBuf, proto.szLocaleName, sizeof(nameBuf));

		for (char* p = nameBuf; *p; ++p)

		{

			if (*p == ' ' || *p == '|' || *p == ';' || *p == ':')

				*p = '_';

		}



		char entry[80];

		snprintf(entry, sizeof(entry), "%u:%s;", proto.dwVnum, nameBuf);

		size_t entryLen = strlen(entry);



		if (chunkLen + (int)entryLen >= (int)sizeof(chunk) - 1)

		{

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelItemListChunk 0|%s", chunk);

			chunkLen = 0;

			chunk[0] = '\0';

		}



		memcpy(chunk + chunkLen, entry, entryLen + 1);

		chunkLen += (int)entryLen;

	}



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelItemListChunk 1|%s", chunk);

}



ACMD(do_gmpanel_account)

{

	char arg1[512];

	one_argument(argument, arg1, sizeof(arg1));



	if (!*arg1)

	{

		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: gmpanel_account <data>");

		return;

	}



	const int FIELD_COUNT = 4;

	char* fields[FIELD_COUNT];

	int fieldCount = 0;



	char* saveptr = NULL;

	char* tok = strtok_r(arg1, "|", &saveptr);

	while (tok && fieldCount < FIELD_COUNT)

	{

		fields[fieldCount++] = tok;

		tok = strtok_r(NULL, "|", &saveptr);

	}



	if (fieldCount != FIELD_COUNT)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAccountResult ERR_BADDATA");

		return;

	}



	const char* nick = fields[0];

	const char* action = fields[1];

	int days = MINMAX(1, atoi(fields[2]), 3650);



	for (const char* p = nick; *p; ++p)

	{

		if (*p == '\'' || *p == '"' || *p == '\\')

		{

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAccountResult ERR_BADNAME");

			return;

		}

	}



	char query[512];



	if (!strcmp(action, "check"))

	{

		snprintf(query, sizeof(query),

				"SELECT a.status, (a.availDt <= NOW()) AS avail FROM account.account a "

				"JOIN player.player p ON p.account_id = a.id WHERE p.name = '%s'", nick);

		std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(query));

		if (!msg.get() || msg->uiSQLErrno != 0 || !msg->Get() || !msg->Get()->pSQLResult ||

				msg->Get()->uiNumRows == 0)

		{

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAccountResult ERR_NOTFOUND");

			return;

		}

		MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAccountResult STATUS|%s|%s",

				row[0] ? row[0] : "?", (row[1] && row[1][0] == '1') ? "OK" : "LOCKED");

		return;

	}

	else if (!strcmp(action, "kick"))

	{

		LPDESC targetDesc = DESC_MANAGER::instance().FindByCharacterName(nick);

		LPCHARACTER targetChar = targetDesc ? targetDesc->GetCharacter() : NULL;

		if (!targetChar)

		{

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAccountResult ERR_NOTONLINE");

			return;

		}

		if (targetChar == ch)

		{

			// Destroying your own desc mid-command is unsafe (the

			// ChatPacket below would then touch an already-destroyed

			// character) - defer to the next tick instead.

			char_event_info* info = AllocEventInfo<char_event_info>();

			info->ch = ch;

			event_create(gmpanel_kick_self_event, info, PASSES_PER_SEC(1));

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAccountResult OK");

			return;

		}

		DESC_MANAGER::instance().DestroyDesc(targetDesc);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAccountResult OK");

		return;

	}

	else if (!strcmp(action, "temp"))

	{

		snprintf(query, sizeof(query),

				"UPDATE account.account a JOIN player.player p ON p.account_id = a.id "

				"SET a.availDt = DATE_ADD(NOW(), INTERVAL %d DAY) WHERE p.name = '%s'", days, nick);

	}

	else if (!strcmp(action, "perm"))

	{

		snprintf(query, sizeof(query),

				"UPDATE account.account a JOIN player.player p ON p.account_id = a.id "

				"SET a.status = 'BLOCK' WHERE p.name = '%s'", nick);

	}

	else if (!strcmp(action, "unlock"))

	{

		snprintf(query, sizeof(query),

				"UPDATE account.account a JOIN player.player p ON p.account_id = a.id "

				"SET a.status = 'OK', a.availDt = '2000-01-01 00:00:00' WHERE p.name = '%s'", nick);

	}

	else

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAccountResult ERR_BADDATA");

		return;

	}



	std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(query));

	if (!msg.get() || msg->uiSQLErrno != 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAccountResult ERR_QUERY");

		return;

	}



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAccountResult OK");

}



// "Dodaj GM" tab of the F9 GM panel. Writes common.gmlist directly and

// triggers the same live-reload the existing ".reload a" GM command uses

// (HEADER_GD_RELOAD_ADMIN to db, which re-reads gmlist/gmhost and pushes

// it back down) so a newly-added GM doesn't need to relog for it to take

// effect. Layout (3 fields): nick|rank|action ("add" or "remove").

ACMD(do_gmpanel_addgm)

{

	char arg1[512];

	one_argument(argument, arg1, sizeof(arg1));



	if (!*arg1)

	{

		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: gmpanel_addgm <data>");

		return;

	}



	const int FIELD_COUNT = 3;

	char* fields[FIELD_COUNT];

	int fieldCount = 0;



	char* saveptr = NULL;

	char* tok = strtok_r(arg1, "|", &saveptr);

	while (tok && fieldCount < FIELD_COUNT)

	{

		fields[fieldCount++] = tok;

		tok = strtok_r(NULL, "|", &saveptr);

	}



	if (fieldCount != FIELD_COUNT)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAddGMResult ERR_BADDATA");

		return;

	}



	const char* nick = fields[0];

	const char* rank = fields[1];

	const char* action = fields[2];



	for (const char* p = nick; *p; ++p)

	{

		if (*p == '\'' || *p == '"' || *p == '\\')

		{

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAddGMResult ERR_BADNAME");

			return;

		}

	}



	char query[1024];



	if (!strcmp(action, "remove"))

	{

		snprintf(query, sizeof(query), "DELETE FROM common.gmlist WHERE mName = '%s'", nick);

		std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(query));

		if (!msg.get() || msg->uiSQLErrno != 0)

		{

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAddGMResult ERR_QUERY");

			return;

		}

		db_clientdesc->DBPacket(HEADER_GD_RELOAD_ADMIN, 0, NULL, 0);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAddGMResult OK");

		return;

	}



	static const char* s_validRanks[] = { "IMPLEMENTOR", "HIGH_WIZARD", "GOD", "LOW_WIZARD", NULL };

	bool bValidRank = false;

	for (int i = 0; s_validRanks[i]; ++i)

	{

		if (!strcmp(rank, s_validRanks[i]))

		{

			bValidRank = true;

			break;

		}

	}

	if (!bValidRank)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAddGMResult ERR_BADRANK");

		return;

	}



	snprintf(query, sizeof(query), "SELECT id FROM player.player WHERE name = '%s'", nick);

	{

		std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(query));

		if (!msg.get() || msg->uiSQLErrno != 0 || !msg->Get() || !msg->Get()->pSQLResult ||

				msg->Get()->uiNumRows == 0)

		{

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAddGMResult ERR_NOTFOUND");

			return;

		}

	}



	// Update in place if a row already exists for this nick - only insert

	// a fresh one when there truly isn't one, so there is never a window

	// where the row is gone (see the comment above this function).

	snprintf(query, sizeof(query), "UPDATE common.gmlist SET mAuthority = '%s' WHERE mName = '%s'", rank, nick);

	std::unique_ptr<SQLMsg> updMsg(AccountDB::instance().DirectQuery(query));

	if (!updMsg.get() || updMsg->uiSQLErrno != 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAddGMResult ERR_QUERY");

		return;

	}



	if (!updMsg->Get() || updMsg->Get()->uiAffectedRows == 0)

	{

		snprintf(query, sizeof(query),

				"INSERT INTO common.gmlist (mAccount, mName, mContactIP, mServerIP, mAuthority) "

				"SELECT a.login, p.name, '*.*.*.*', 'ALL', '%s' FROM player.player p "

				"JOIN account.account a ON a.id = p.account_id WHERE p.name = '%s'", rank, nick);

		std::unique_ptr<SQLMsg> insMsg(AccountDB::instance().DirectQuery(query));

		if (!insMsg.get() || insMsg->uiSQLErrno != 0)

		{

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAddGMResult ERR_QUERY");

			return;

		}

	}



	db_clientdesc->DBPacket(HEADER_GD_RELOAD_ADMIN, 0, NULL, 0);

	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAddGMResult OK");

}



// "Spawn Botow" tab of the F9 GM panel - thin wrapper around the existing

// bot_spawn_many/bot_despawn_many logic (CPlayerBotManager::Spawn/Despawn),

// just answering through the panel's chat-command channel instead of plain

// chat text. Layout (4 fields): firstPid|count|empire|action.

ACMD(do_gmpanel_spawn)

{

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));



	if (!*arg1)

	{

		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: gmpanel_spawn <data>");

		return;

	}



	const int FIELD_COUNT = 4;

	char* fields[FIELD_COUNT];

	int fieldCount = 0;



	char* saveptr = NULL;

	char* tok = strtok_r(arg1, "|", &saveptr);

	while (tok && fieldCount < FIELD_COUNT)

	{

		fields[fieldCount++] = tok;

		tok = strtok_r(NULL, "|", &saveptr);

	}



	if (fieldCount != FIELD_COUNT)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnResult ERR_BADDATA");

		return;

	}



	DWORD dwFirstPlayerID = 0;

	int iCount = 0;

	int iEmpire = 0;

	str_to_number(dwFirstPlayerID, fields[0]);

	str_to_number(iCount, fields[1]);

	str_to_number(iEmpire, fields[2]);

	const char* action = fields[3];



	if (dwFirstPlayerID == 0 || iCount <= 0 || iCount > 500)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnResult ERR_BADDATA");

		return;

	}



	int iDone = 0;

	if (!strcmp(action, "despawn"))

	{

		for (int i = 0; i < iCount; ++i)

			if (CPlayerBotManager::instance().Despawn(dwFirstPlayerID + i))

				++iDone;

	}

	else

	{

		if (iEmpire <= 0 || iEmpire >= EMPIRE_MAX_NUM)

		{

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnResult ERR_BADEMPIRE");

			return;

		}

		for (int i = 0; i < iCount; ++i)

			if (CPlayerBotManager::instance().Spawn(dwFirstPlayerID + i, static_cast<BYTE>(iEmpire)))

				++iDone;

	}



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnResult OK|%d|%d|%u",

			iDone, iCount, static_cast<unsigned int>(CPlayerBotManager::instance().GetCount()));

}



// Reuses do_playerbot_rank's exact scan/sort, answering through the same

// chunked GMPanelItemListChunk channel do_gmpanel_itemlist uses (one

// ChatPacket is capped at CHAT_MAX_LEN=512) - capped to the top 20 by

// level, plus a synthetic pid-0 entry carrying the true total count (the

// GM panel's list only ever shows a handful of rows either way).

ACMD(do_gmpanel_botlist)

{

	struct TGMPanelBotEntry

	{

		DWORD pid;

		std::string name;

		BYTE level;

		long mapIndex;



		bool operator < (const TGMPanelBotEntry& other) const

		{

			return level > other.level;

		}

	};



	std::vector<TGMPanelBotEntry> ranks;

	for (DWORD pid = 4; pid <= 500; ++pid)

	{

		LPCHARACTER bot = CHARACTER_MANAGER::instance().FindByPID(pid);

		if (bot && CPlayerBotManager::instance().IsManaged(pid))

		{

			TGMPanelBotEntry e;

			e.pid = pid;

			e.name = bot->GetName();

			e.level = bot->GetLevel();

			e.mapIndex = bot->GetMapIndex();

			ranks.push_back(e);

		}

	}

	std::sort(ranks.begin(), ranks.end());



	char chunk[400];

	int chunkLen = 0;

	chunk[0] = '\0';



	char header[64];

	snprintf(header, sizeof(header), "0:Aktywne_ogolem:_%u;", (unsigned int)ranks.size());

	size_t headerLen = strlen(header);

	memcpy(chunk + chunkLen, header, headerLen + 1);

	chunkLen += (int)headerLen;



	size_t showCount = std::min((size_t)20, ranks.size());

	for (size_t i = 0; i < showCount; ++i)

	{

		char nameBuf[64];

		strlcpy(nameBuf, ranks[i].name.c_str(), sizeof(nameBuf));

		for (char* p = nameBuf; *p; ++p)

		{

			if (*p == ' ' || *p == '|' || *p == ';' || *p == ':')

				*p = '_';

		}



		char entry[100];

		snprintf(entry, sizeof(entry), "%u:%s_Lv%u_Mapa%ld;", ranks[i].pid, nameBuf, ranks[i].level, ranks[i].mapIndex);

		size_t entryLen = strlen(entry);



		if (chunkLen + (int)entryLen >= (int)sizeof(chunk) - 1)

		{

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelItemListChunk 0|%s", chunk);

			chunkLen = 0;

			chunk[0] = '\0';

		}



		memcpy(chunk + chunkLen, entry, entryLen + 1);

		chunkLen += (int)entryLen;

	}



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelItemListChunk 1|%s", chunk);

}



// ============================================================================

// /botadmin* - komendy dla GUI zarzadzania botami (patrz uiPlayerbotAdmin.py

// po stronie klienta). Wszystkie sa wysylane WYLACZNIE przez okno GUI, nigdy

// recznie przez gracza - stad minimalna obsluga bledow, tylko twarde

// odrzucenie zlej skladni.

//

// BEZPIECZENSTWO: jedyne miejsce ktore faktycznie decyduje czy ktos smie tego

// uzyc to tabela cmd_info w cmd.cpp - wszystkie komendy ponizej sa tam

// zarejestrowane na GM_IMPLEMENTOR, tym samym poziomie co istniejace

// /bot_spawn i /bot_despawn. Klient NIE wie czy ktos jest GM - ikonka jest

// zawsze widoczna, ale gdy zwykly gracz ja kliknie, /botadmin zostanie

// odrzucone przez interpret_command() zanim ta funkcja w ogole sie wykona.

// ============================================================================



// /botadmin - otwiera GUI.

// do_botadmin* - the bot-admin sub-window (uiPlayerbotAdmin.py client
// side), separate from do_gmpanel_open above (that one opens the main F9
// GMPanelWindow instead). Same GM_IMPLEMENTOR gate as everything else in
// this file (cmd_info in cmd.cpp); the client shows the icon to everyone
// but a non-GM press is rejected before this ever runs.
// /botadmin - otwiera GUI.
ACMD(do_botadmin)
{
	// F10 from a player: silence, the same as F9 (do_gmpanel_open).
	if (ch->GetGMLevel() < GM_IMPLEMENTOR)
		return;
	ch->ChatPacket(CHAT_TYPE_COMMAND, "OpenPlayerbotAdminWindow");
}

// /botadmin_stats - zakladka "Ogolne": aktywne boty / w PT / na straganie.
ACMD(do_botadmin_stats)
{
	size_t total = 0, inParty = 0, stalls = 0;
	CPlayerBotManager::instance().GetActivitySummary(total, inParty, stalls);

	ch->ChatPacket(CHAT_TYPE_COMMAND, "PlayerbotAdminStats %u %u %u",
			static_cast<unsigned int>(total),
			static_cast<unsigned int>(inParty),
			static_cast<unsigned int>(stalls));
}

// /botadmin_list - zakladka "Zarzadzanie": pelna lista aktywnych botow.
// Ten sam zakres PID co istniejace /bot_rank (4..500).
ACMD(do_botadmin_list)
{
	for (DWORD pid = 4; pid <= 500; ++pid)
	{
		LPCHARACTER bot = CHARACTER_MANAGER::instance().FindByPID(pid);
		if (!bot || !CPlayerBotManager::instance().IsManaged(pid))
			continue;

		ch->ChatPacket(CHAT_TYPE_COMMAND, "PlayerbotAdminBotRow %u %u %u %ld %ld %s",
				pid, bot->GetLevel(), bot->GetEmpire(), bot->GetX(), bot->GetY(), bot->GetName());
	}

	ch->ChatPacket(CHAT_TYPE_COMMAND, "PlayerbotAdminBotListEnd");
}

// /botadmin_botlog <pid> - zakladka "Akcje na zywo": ostatnie linie danego bota.
// Spacje w tresci linii zamieniane sa tu na '~', klient zamienia z powrotem.
ACMD(do_botadmin_botlog)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	DWORD pid = 0;
	str_to_number(pid, arg1);

	if (pid == 0 || !CPlayerBotManager::instance().IsManaged(pid))
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "PlayerbotAdminBotLogEnd %u", pid);
		return;
	}

	std::vector<std::string> lines;
	CPlayerBotManager::instance().GetBotLines(pid, lines);

	for (size_t i = 0; i < lines.size(); ++i)
	{
		std::string wire = lines[i];
		for (size_t c = 0; c < wire.size(); ++c)
		{
			if (wire[c] == ' ')
				wire[c] = '~';
		}

		ch->ChatPacket(CHAT_TYPE_COMMAND, "PlayerbotAdminBotLogLine %u %s", pid, wire.c_str());
	}

	ch->ChatPacket(CHAT_TYPE_COMMAND, "PlayerbotAdminBotLogEnd %u", pid);
}

// /botadmin_give <pid> <item vnum> <ilosc> - daje przedmiot do ekwipunku bota.
// Dziala WYLACZNIE na postaciach zarzadzanych przez CPlayerBotManager.
ACMD(do_botadmin_give)
{
	char arg1[256], arg2[256], arg3[256];
	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
	one_argument(argument, arg3, sizeof(arg3));

	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: /botadmin_give <pid> <vnum> <ilosc>");
		return;
	}

	DWORD pid = 0;
	DWORD dwVnum = 0;
	int iCount = 1;

	str_to_number(pid, arg1);
	str_to_number(dwVnum, arg2);

	if (*arg3)
	{
		str_to_number(iCount, arg3);
		iCount = MINMAX(1, iCount, ITEM_MAX_COUNT);
	}

	if (pid == 0 || dwVnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: /botadmin_give <pid> <vnum> <ilosc>");
		return;
	}

	LPCHARACTER bot = CHARACTER_MANAGER::instance().FindByPID(pid);
	if (!bot || !CPlayerBotManager::instance().IsManaged(pid))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Nie ma zarzadzanego bota o PID %u.", pid);
		return;
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwVnum, iCount, 0, true);
	if (!item)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "#%u - taki przedmiot nie istnieje.", dwVnum);
		return;
	}

	int iEmptyPos = bot->GetEmptyInventory(item->GetSize());
	if (iEmptyPos == -1)
	{
		M2_DESTROY_ITEM(item);
		ch->ChatPacket(CHAT_TYPE_INFO, "Bot %s nie ma miejsca w ekwipunku.", bot->GetName());
		return;
	}

	item->AddToCharacter(bot, TItemPos(INVENTORY, iEmptyPos));
	LogManager::instance().ItemLog(ch, item, "BOTADMIN_GUI", bot->GetName());

	ch->ChatPacket(CHAT_TYPE_INFO, "Dano %s x%d botowi %s.", item->GetName(), iCount, bot->GetName());
}

// /botadmin_achievements - zakladka "Osiagniecia": kto pierwszy co zdobyl.
// Lista id musi sie zgadzac z s_playerBotAchievementDefs w playerbot_manager.cpp
// i z ACHIEVEMENT_LABELS w kliencie (uiPlayerbotAdmin.py) - etykiety (PO
// POLSKU, ze spacjami) sa NIGDY wysylane przez siec, trzymane lokalnie po
// obu stronach pod tym samym id.
ACMD(do_botadmin_achievements)
{
	static const int s_achievementIds[] = { 1, 2, 3 };

	for (size_t i = 0; i < sizeof(s_achievementIds) / sizeof(s_achievementIds[0]); ++i)
	{
		DWORD winnerPid = 0;
		std::string winnerName;

		if (CPlayerBotManager::instance().GetAchievementWinner(s_achievementIds[i], winnerPid, winnerName))
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "PlayerbotAdminAchievementRow %d %u %s",
					s_achievementIds[i], winnerPid, winnerName.c_str());
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "PlayerbotAdminAchievementRow %d 0 -",
					s_achievementIds[i]);
		}
	}

	ch->ChatPacket(CHAT_TYPE_COMMAND, "PlayerbotAdminAchievementsEnd");
}

ACMD(do_gmpanel_open)

{

	// Registered for GM_PLAYER, so a player pressing F9 hears nothing
	// instead of "no such command"; the threshold is kept here.
	if (ch->GetGMLevel() < GM_HIGH_WIZARD)
		return;
	ch->ChatPacket(CHAT_TYPE_COMMAND, "OpenGMPanelWindow");

}



// /gmpanel_available_bots - "Lista botow gotowych do spawnu" w zakladce

// Spawn Botow: registered PIDs that are NOT currently spawned, ordered

// ascending (natural order of the underlying std::set<DWORD>). Reuses the

// same GMPanelItemListChunk wire protocol as do_gmpanel_botlist - a chat

// packet is capped at CHAT_MAX_LEN, so the payload goes out in chunks.

ACMD(do_gmpanel_available_bots)

{

	std::vector<DWORD> available;

	CPlayerBotManager::instance().GetAvailableBots(available, 300);



	char chunk[400];

	int chunkLen = 0;

	chunk[0] = '\0';



	// No header/count pseudo-row here (unlike do_gmpanel_botlist's "Aktywne

	// ogolem") - that entry is only harmless in the read-only bot list

	// (widget=None there). Here the picker field IS spawnPidEdit, so

	// clicking a row writes straight into "ID Bota" - a stray "0" row would

	// silently corrupt it. The total is visible via pagination ("Dalej")

	// instead.

	for (size_t i = 0; i < available.size(); ++i)

	{

		char entry[32];

		snprintf(entry, sizeof(entry), "%u:%u;", available[i], available[i]);

		size_t entryLen = strlen(entry);



		if (chunkLen + (int)entryLen >= (int)sizeof(chunk) - 1)

		{

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelItemListChunk 0|%s", chunk);

			chunkLen = 0;

			chunk[0] = '\0';

		}



		memcpy(chunk + chunkLen, entry, entryLen + 1);

		chunkLen += (int)entryLen;

	}



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelItemListChunk 1|%s", chunk);

}



// /gmpanel_moblist - "Moby" picker on the new Spawn Mobow tab.

ACMD(do_gmpanel_moblist)

{

	SendGMPanelMobListChunks(ch, CHAR_TYPE_MONSTER);

}



// /gmpanel_metinlist - "Metiny" picker, same tab.

ACMD(do_gmpanel_metinlist)

{

	SendGMPanelMobListChunks(ch, CHAR_TYPE_STONE);

}



// /gmpanel_spawnmob <vnum>|<count>|<kind> - both Spawn buttons on the new

// Spawn Mobow tab (kind is "mob" or "metin", echoed back as-is so the

// client knows which of its two status lines to update). Reuses do_mob's

// exact spawn mechanism (SpawnMobRange, same +/-200..750 box around the GM,

// same test_server-aware count cap) - vnum only, never a name, since the

// client always fills the "ID" edit line with a numeric vnum whether typed

// directly or picked from the name-only moblist/metinlist above.

ACMD(do_gmpanel_spawnmob)

{

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));



	if (!*arg1)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnMobResult mob|ERR_BADDATA");

		return;

	}



	const int FIELD_COUNT = 3;

	char* fields[FIELD_COUNT];

	int fieldCount = 0;



	char* saveptr = NULL;

	char* tok = strtok_r(arg1, "|", &saveptr);

	while (tok && fieldCount < FIELD_COUNT)

	{

		fields[fieldCount++] = tok;

		tok = strtok_r(NULL, "|", &saveptr);

	}



	const char* kind = (fieldCount == FIELD_COUNT) ? fields[2] : "mob";



	if (fieldCount != FIELD_COUNT)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnMobResult %s|ERR_BADDATA", kind);

		return;

	}



	DWORD vnum = 0;

	int iCount = 0;

	str_to_number(vnum, fields[0]);

	str_to_number(iCount, fields[1]);



	const CMob* pkMob = vnum ? CMobManager::instance().Get(vnum) : NULL;

	if (!pkMob || iCount <= 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnMobResult %s|%s", kind, pkMob ? "ERR_BADDATA" : "ERR_NOTFOUND");

		return;

	}



	if (test_server)

		iCount = MIN(40, iCount);

	else

		iCount = MIN(20, iCount);



	int iDone = 0;

	for (int i = 0; i < iCount; ++i)

	{

		if (CHARACTER_MANAGER::instance().SpawnMobRange(vnum,

				ch->GetMapIndex(),

				ch->GetX() - number(200, 750),

				ch->GetY() - number(200, 750),

				ch->GetX() + number(200, 750),

				ch->GetY() + number(200, 750),

				true,

				pkMob->m_table.bType == CHAR_TYPE_STONE))

			++iDone;

	}



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnMobResult %s|OK|%d|%d", kind, iDone, iCount);

}



// /gmpanel_check_gm - client-initiated, sent once right after entering the

// game (game.py, GameWindow.__init__). Only reachable at all if gm_level

// allows it (GM_HIGH_WIZARD in cmd_info, checked fresh - see do_gmpanel_open

// for the same pattern); if it runs, the caller IS a GM, so just answer.

// Replaces relying solely on the server's own one-shot push 3s after login

// (see gmpanel_flag_event, char.cpp) as the only source of "SetGMFlag" -

// that push is left in place too, this is a second, client-driven chance

// for the flag to actually land.

ACMD(do_gmpanel_check_gm)

{

	// The client asks this on every entry into the game, GM or not.
	if (ch->GetGMLevel() < GM_HIGH_WIZARD)
		return;
	ch->ChatPacket(CHAT_TYPE_COMMAND, "SetGMFlag");

}



// /gmpanel_view_equip <vid> - "EQ" na menu celu (uitarget.py). Text-based

// replacement for the vanilla /view_equip, which crashes this client build

// (confirmed 3/3 via syslog: view_equip always immediately precedes

// DISCONNECT - almost certainly a WEAR_MAX_NUM/struct-size mismatch between

// this fork's TPacketViewEquip and whatever this compiled client expects).

// Reuses the plain-text GMPanelItemListChunk protocol instead of that

// native struct, same as do_gmpanel_botlist/do_gmpanel_available_bots.

ACMD(do_gmpanel_view_equip)

{

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));



	DWORD vid = 0;

	str_to_number(vid, arg1);



	LPCHARACTER tch = vid ? CHARACTER_MANAGER::instance().Find(vid) : NULL;



	char chunk[400];

	int chunkLen = 0;

	chunk[0] = '\0';



	if (!tch || !tch->IsPC())

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMEquipChunk %u 1 -", vid);

		return;

	}



	for (int i = 0; i < WEAR_MAX_NUM; ++i)

	{

		LPITEM item = tch->GetWear(i);

		if (!item)

			continue;



		// slot:vnum:count:s0:s1:s2:t0:v0:t1:v1:t2:v2:t3:v3:t4:v4:t5:v5:t6:v6;

		// Bounds MUST be ITEM_SOCKET_MAX_NUM(3)/ITEM_ATTRIBUTE_MAX_NUM(7) -

		// these are the real m_alSockets/m_aAttr array sizes (item.h). Do

		// NOT copy the "for i < 6" socket loop from do_gmpanel_createitem

		// elsewhere in this file - that one writes past the 3-slot array

		// on a release build (SetSocket's own bounds check is only an

		// assert(), compiled out) - flagged separately, not fixed here.

		const long* sockets = item->GetSockets();

		const TPlayerItemAttribute* attrs = item->GetAttributes();



		char entry[160];

		int len = snprintf(entry, sizeof(entry), "%d:%u:%d:%ld:%ld:%ld",

				i, item->GetVnum(), (int)item->GetCount(),

				sockets[0], sockets[1], sockets[2]);

		for (int a = 0; a < ITEM_ATTRIBUTE_MAX_NUM && len < (int)sizeof(entry) - 20; ++a)

		{

			len += snprintf(entry + len, sizeof(entry) - len, ":%d:%d",

					(int)attrs[a].bType, (int)attrs[a].sValue);

		}

		len += snprintf(entry + len, sizeof(entry) - len, ";");

		size_t entryLen = strlen(entry);



		if (chunkLen + (int)entryLen >= (int)sizeof(chunk) - 1)

		{

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMEquipChunk %u 0 %s", vid, chunk);

			chunkLen = 0;

			chunk[0] = '\0';

		}



		memcpy(chunk + chunkLen, entry, entryLen + 1);

		chunkLen += (int)entryLen;

	}



	if (chunkLen == 0)

		chunk[0] = '-', chunk[1] = '\0';



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMEquipChunk %u 1 %s", vid, chunk);

}



// ============================================================================

// "Sprawdz Gracza" tab, dodatkowe akcje GM (2026-09-09): Daj Yang, Daj

// Smocze Monety, Zmien poziom konia, Zmien range, Dodaj statystyki, lista

// skilli + Zmien skille. Wszystkie na jednym pipe-delimited argumencie

// ("nick|..." lub "nick|pole|wartosc"), dokladnie ten sam wzorzec co

// do_gmpanel_spawn (strtok_r na "|"). Dzialaja i na postaci online (live

// CHARACTER + sync packet) i offline (DirectQuery UPDATE) gdzie to ma sens -

// "Smocze Monety" i "range" sa czysto kontowe/informacyjne, wiec dla nich

// zawsze DB UPDATE niezaleznie od statusu online.

// ============================================================================



ACMD(do_gmpanel_give_gold)

{

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));



	char* fields[2];

	int fieldCount = 0;

	char* saveptr = NULL;

	char* tok = strtok_r(arg1, "|", &saveptr);

	while (tok && fieldCount < 2)

	{

		fields[fieldCount++] = tok;

		tok = strtok_r(NULL, "|", &saveptr);

	}

	if (fieldCount != 2)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelGiveGoldResult ERR_BADDATA");

		return;

	}



	int amount = 0;

	str_to_number(amount, fields[1]);

	if (amount == 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelGiveGoldResult ERR_BADDATA");

		return;

	}



	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(fields[0]);

	if (tch && tch->GetDesc())

	{

		DBManager::instance().SendMoneyLog(MONEY_LOG_MISC, 3, amount);

		tch->PointChange(POINT_GOLD, amount, true);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelGiveGoldResult OK|%lld", (long long)tch->GetGold());

		return;

	}



	char query[256];

	snprintf(query, sizeof(query), "UPDATE player.player SET gold = gold + (%d) WHERE name = '%s'", amount, fields[0]);

	std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(query));

	if (!msg.get() || msg->uiSQLErrno != 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelGiveGoldResult ERR_QUERY");

		return;

	}

	if (!msg->Get() || msg->Get()->uiAffectedRows == 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelGiveGoldResult ERR_NOTFOUND");

		return;

	}

	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelGiveGoldResult OK_OFFLINE");

}



// "Smocze Monety" = account.account.cash (confirmed against itemshop's own

// buy.php: currency=='mileage' -> "Smocze Znaki", else -> "Smocze Monety").

// Account-level, not tied to the live CHARACTER at all - always a DB UPDATE.

ACMD(do_gmpanel_give_cash)

{

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));



	char* fields[2];

	int fieldCount = 0;

	char* saveptr = NULL;

	char* tok = strtok_r(arg1, "|", &saveptr);

	while (tok && fieldCount < 2)

	{

		fields[fieldCount++] = tok;

		tok = strtok_r(NULL, "|", &saveptr);

	}

	if (fieldCount != 2)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelGiveCashResult ERR_BADDATA");

		return;

	}



	int amount = 0;

	str_to_number(amount, fields[1]);

	if (amount == 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelGiveCashResult ERR_BADDATA");

		return;

	}



	char query[320];

	snprintf(query, sizeof(query),

			"UPDATE account.account a JOIN player.player p ON p.account_id = a.id "

			// mt2009: account.account ma tylko cash/cash_mark/mileage (bez total_cash).

			"SET a.cash = a.cash + (%d) WHERE p.name = '%s'",

			amount, fields[0]);

	std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(query));

	if (!msg.get() || msg->uiSQLErrno != 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelGiveCashResult ERR_QUERY");

		return;

	}

	if (!msg->Get() || msg->Get()->uiAffectedRows == 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelGiveCashResult ERR_NOTFOUND");

		return;

	}

	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelGiveCashResult OK");

}



// "Zmien poziom konia" - sets horse_skill_point ("Punkty konne" in the

// lookup display) to an absolute target value (not +=, matches "na ile

// punktow MA BYC" in the request, unlike "Daj Yang" which adds).

ACMD(do_gmpanel_set_horse_points)

{

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));



	char* fields[2];

	int fieldCount = 0;

	char* saveptr = NULL;

	char* tok = strtok_r(arg1, "|", &saveptr);

	while (tok && fieldCount < 2)

	{

		fields[fieldCount++] = tok;

		tok = strtok_r(NULL, "|", &saveptr);

	}

	if (fieldCount != 2)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetHorsePointsResult ERR_BADDATA");

		return;

	}



	int value = 0;

	str_to_number(value, fields[1]);

	value = MINMAX(0, value, 30); // HORSE_MAX_LEVEL, horse_rider.h



	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(fields[0]);

	if (tch && tch->GetDesc())

	{

		tch->SetHorseLevel(value);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetHorsePointsResult OK|%d", value);

		return;

	}



	char query[256];

	snprintf(query, sizeof(query), "UPDATE player.player SET horse_level = %d WHERE name = '%s'", value, fields[0]);

	std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(query));

	if (!msg.get() || msg->uiSQLErrno != 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetHorsePointsResult ERR_QUERY");

		return;

	}

	if (!msg->Get() || msg->Get()->uiAffectedRows == 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetHorsePointsResult ERR_NOTFOUND");

		return;

	}

	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetHorsePointsResult OK_OFFLINE|%d", value);

}



// "Zmien range" = player.player.alignment (user confirmed: this is the

// same field /set <nick> align <value> already writes - not a new system).

// That vanilla case, oddly, computes its delta from ch->GetRealAlignment()

// (the CALLER's own alignment) instead of tch's - looks like a copy-paste

// bug in the original source, not something to copy. Using tch's own

// GetRealAlignment() here instead so this actually sets the TARGET to the

// requested absolute value.

ACMD(do_gmpanel_set_range)

{

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));



	char* fields[2];

	int fieldCount = 0;

	char* saveptr = NULL;

	char* tok = strtok_r(arg1, "|", &saveptr);

	while (tok && fieldCount < 2)

	{

		fields[fieldCount++] = tok;

		tok = strtok_r(NULL, "|", &saveptr);

	}

	if (fieldCount != 2)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetRangeResult ERR_BADDATA");

		return;

	}



	int value = 0;

	str_to_number(value, fields[1]);

	int rawValue = value * 10;



	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(fields[0]);

	if (tch && tch->GetDesc())

	{

		tch->UpdateAlignment(rawValue - tch->GetRealAlignment());

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetRangeResult OK|%d", value);

		return;

	}



	char query[256];

	snprintf(query, sizeof(query), "UPDATE player.player SET alignment = %d WHERE name = '%s'", rawValue, fields[0]);

	std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(query));

	if (!msg.get() || msg->uiSQLErrno != 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetRangeResult ERR_QUERY");

		return;

	}

	if (!msg->Get() || msg->Get()->uiAffectedRows == 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetRangeResult ERR_NOTFOUND");

		return;

	}

	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetRangeResult OK_OFFLINE|%d", value);

	// (echoes the display-scale "value", not the x10 "rawValue" just written)

}



// "Dodaj statystyki" - sets st/ht/dx/iq to an absolute value (e.g. "Sila z

// 34 pkt na 5000"), uncapped by the normal JobInitialPoints floor that

// gates the player's own stat-point spending (cmd_general.cpp) - this is

// an admin override, not the normal levelup path. Mirrors that same

// command's SetRealPoint+SetPoint+ComputePoints+PointChange(...,0) sync

// pattern (incl. the MAX_HP/MAX_SP follow-up for ht/iq).

ACMD(do_gmpanel_set_stat)

{

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));



	char* fields[3];

	int fieldCount = 0;

	char* saveptr = NULL;

	char* tok = strtok_r(arg1, "|", &saveptr);

	while (tok && fieldCount < 3)

	{

		fields[fieldCount++] = tok;

		tok = strtok_r(NULL, "|", &saveptr);

	}

	if (fieldCount != 3)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetStatResult ERR_BADDATA");

		return;

	}



	BYTE pointType;

	const char* column;

	if (!strcmp(fields[1], "st")) { pointType = POINT_ST; column = "st"; }

	else if (!strcmp(fields[1], "ht")) { pointType = POINT_HT; column = "ht"; }

	else if (!strcmp(fields[1], "dx")) { pointType = POINT_DX; column = "dx"; }

	else if (!strcmp(fields[1], "iq")) { pointType = POINT_IQ; column = "iq"; }

	else

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetStatResult ERR_BADSTAT");

		return;

	}



	int value = 0;

	str_to_number(value, fields[2]);

	value = MINMAX(0, value, 30000); // matches the "(Max 30.000)" label in the GM panel



	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(fields[0]);

	if (tch && tch->GetDesc())

	{

		tch->SetRealPoint(pointType, value);

		tch->SetPoint(pointType, value);

		tch->ComputePoints();

		tch->PointChange(pointType, 0);

		if (pointType == POINT_HT)

			tch->PointChange(POINT_MAX_HP, 0);

		else if (pointType == POINT_IQ)

			tch->PointChange(POINT_MAX_SP, 0);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetStatResult OK|%s|%d", fields[1], value);

		return;

	}



	char query[256];

	snprintf(query, sizeof(query), "UPDATE player.player SET %s = %d WHERE name = '%s'", column, value, fields[0]);

	std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(query));

	if (!msg.get() || msg->uiSQLErrno != 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetStatResult ERR_QUERY");

		return;

	}

	if (!msg->Get() || msg->Get()->uiAffectedRows == 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetStatResult ERR_NOTFOUND");

		return;

	}

	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetStatResult OK_OFFLINE|%s|%d", fields[1], value);

}



// "Zmien skille" - list + edit. ONLINE ONLY on purpose: skill_level is a

// raw BLOB column (player.player.skill_level) with no documented layout

// available here, and guessing its binary format would repeat exactly the

// kind of native-struct assumption that already crashed the client once

// today (TPacketViewEquip/WEAR_MAX_NUM mismatch) - except this time on the

// SERVER's own data, which is worse to get wrong. CHARACTER::GetSkillLevel/

// SetSkillLevel (used by the existing vanilla /setskillother command) are

// the only trusted read/write path, and both require a live CHARACTER.

ACMD(do_gmpanel_skilllist)

{

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));



	if (!*arg1)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSkillListResult ERR_BADDATA");

		return;

	}



	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch || !tch->GetDesc())

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSkillListResult ERR_OFFLINE");

		return;

	}



	char chunk[400];

	int chunkLen = 0;

	chunk[0] = '\0';

	bool any = false;



	for (DWORD vnum = 1; vnum <= 262; ++vnum)

	{

		CSkillProto* pk = CSkillManager::instance().Get(vnum);

		if (!pk)

			continue;



		int level = tch->GetSkillLevel(vnum);

		if (level <= 0)

			continue;

		any = true;



		// pk->szName is raw EUC-KR, never localized in this fork (checked -

		// client/locale_pl/skilltable.txt has the same untranslated Korean

		// bytes, not real Polish text) - showing it as-is just garbles.

		// "Skill #<vnum>" instead: honest, and Zmien skille already keys

		// off vnum anyway so this loses no functionality.

		char entry[48];

		snprintf(entry, sizeof(entry), "%u:Skill_#%u:%d:%d;", vnum, vnum, level, SKILL_MAX_LEVEL);

		size_t entryLen = strlen(entry);



		if (chunkLen + (int)entryLen >= (int)sizeof(chunk) - 1)

		{

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSkillListResult 0|%s", chunk);

			chunkLen = 0;

			chunk[0] = '\0';

		}



		memcpy(chunk + chunkLen, entry, entryLen + 1);

		chunkLen += (int)entryLen;

	}



	if (!any)

	{

		const char* empty = "0:(brak_wyuczonych_skilli):0:0;";

		size_t len = strlen(empty);

		memcpy(chunk, empty, len + 1);

		chunkLen = (int)len;

	}



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSkillListResult 1|%s", chunk);

}



ACMD(do_gmpanel_setskill)

{

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));



	char* fields[3];

	int fieldCount = 0;

	char* saveptr = NULL;

	char* tok = strtok_r(arg1, "|", &saveptr);

	while (tok && fieldCount < 3)

	{

		fields[fieldCount++] = tok;

		tok = strtok_r(NULL, "|", &saveptr);

	}

	if (fieldCount != 3)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetSkillResult ERR_BADDATA");

		return;

	}



	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(fields[0]);

	if (!tch || !tch->GetDesc())

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetSkillResult ERR_OFFLINE");

		return;

	}



	DWORD vnum = 0;

	str_to_number(vnum, fields[1]);

	CSkillProto* pk = CSkillManager::instance().Get(vnum);

	if (!pk)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetSkillResult ERR_BADSKILL");

		return;

	}



	int levelInt = 0;

	str_to_number(levelInt, fields[2]);

	levelInt = MINMAX(0, levelInt, SKILL_MAX_LEVEL);

	BYTE level = (BYTE)levelInt;



	tch->SetSkillLevel(pk->dwVnum, level);

	tch->ComputePoints();

	tch->SkillLevelPacket();



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetSkillResult OK|%u|%d", pk->dwVnum, level);

}





// ===========================================================================

//  Panel GM (F9), karty "Spawn Mobow" i "Sterowanie Serwerem"

//

//  Eight commands OskarPWA's newer client sends and the merge of his first

//  version never had. Every one of them answers on the same CHAT_TYPE_COMMAND

//  channel as the rest of the panel, with the reply name game.py dispatches on

//  and the exact field shape interfacemodule.py parses - see the comment above

//  each for which handler reads it.

// ===========================================================================



// The polymorph marble in these serverfiles. item_proto carries six items of

// type ITEM_POLYMORPH (50322, 70104..70107, 71093) and they differ in nothing

// that matters here; 70104 is the one the engine's own do_polymorph_item

// creates, so the panel hands out the same item a GM would get by typing /p.

#define GM_PANEL_POLYMORPH_VNUM 70104



// /gmpanel_spawnrandommobs <v1,v2,...>|<count> - the "Bossy" rows on the Spawn

// Mobow tab. The client sends a pool and a count and expects that many mobs

// drawn AT RANDOM from the pool, never `count` copies of one: "10 bossow" is

// meant to be ten different ones where the pool allows it.

//

// Spawned exactly the way do_gmpanel_spawnmob (and do_mob before it) does, in

// the same box around the GM, so nothing new can go wrong with placement.

ACMD(do_gmpanel_spawnrandommobs)

{

	char arg1[1024];

	one_argument(argument, arg1, sizeof(arg1));



	if (!*arg1)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnBossResult ERR_BADDATA");

		return;

	}



	char* saveptr = NULL;

	char* poolField = strtok_r(arg1, "|", &saveptr);

	char* countField = strtok_r(NULL, "|", &saveptr);

	if (!poolField || !countField)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnBossResult ERR_BADDATA");

		return;

	}



	int iCount = 0;

	str_to_number(iCount, countField);

	if (iCount <= 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnBossResult ERR_BADDATA");

		return;

	}



	// The pool as the client sent it, minus anything this world does not have.

	// A vnum the client's static table carries and mob_proto does not is not an

	// error - the panel ships one list for every serverfiles set there is.

	std::vector<DWORD> pool;

	char* poolSave = NULL;

	for (char* tok = strtok_r(poolField, ",", &poolSave); tok;

			tok = strtok_r(NULL, ",", &poolSave))

	{

		DWORD vnum = 0;

		str_to_number(vnum, tok);

		if (vnum && CMobManager::instance().Get(vnum))

			pool.push_back(vnum);

	}



	if (pool.empty())

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnBossResult ERR_NOTFOUND");

		return;

	}



	iCount = MIN(test_server ? 40 : 20, iCount);



	int iDone = 0;

	for (int i = 0; i < iCount; ++i)

	{

		const DWORD vnum = pool[number(0, (int)pool.size() - 1)];

		const CMob* pkMob = CMobManager::instance().Get(vnum);

		if (!pkMob)

			continue;

		if (CHARACTER_MANAGER::instance().SpawnMobRange(vnum,

				ch->GetMapIndex(),

				ch->GetX() - number(200, 750),

				ch->GetY() - number(200, 750),

				ch->GetX() + number(200, 750),

				ch->GetY() + number(200, 750),

				true,

				pkMob->m_table.bType == CHAR_TYPE_STONE))

			++iDone;

	}



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnBossResult OK|%d|%d", iDone, iCount);

}



// /gmpanel_spawnrandommetin <lo>|<hi>|<count> - the "Metiny" rows on the same

// tab. The client sends a level band and no vnums at all, deliberately: it has

// no idea which stones this world carries, and a hard-coded list would be wrong

// on every serverfiles set but one. The pool is built here from mob_proto's own

// stones, which CMobManager already holds in memory - no query, and it answers

// for whatever the operator's files contain.

ACMD(do_gmpanel_spawnrandommetin)

{

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));



	if (!*arg1)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnMetinResult ERR_BADDATA");

		return;

	}



	char* saveptr = NULL;

	char* loField = strtok_r(arg1, "|", &saveptr);

	char* hiField = strtok_r(NULL, "|", &saveptr);

	char* countField = strtok_r(NULL, "|", &saveptr);

	if (!loField || !hiField || !countField)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnMetinResult ERR_BADDATA");

		return;

	}



	int iLow = 0, iHigh = 0, iCount = 0;

	str_to_number(iLow, loField);

	str_to_number(iHigh, hiField);

	str_to_number(iCount, countField);

	if (iCount <= 0 || iLow < 0 || iHigh < iLow)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnMetinResult ERR_BADDATA");

		return;

	}



	std::vector<DWORD> pool;

	for (CMobManager::iterator it = CMobManager::instance().begin();

			it != CMobManager::instance().end(); ++it)

	{

		const CMob* pkMob = it->second;

		if (!pkMob || pkMob->m_table.bType != CHAR_TYPE_STONE)

			continue;

		if (pkMob->m_table.bLevel < iLow || pkMob->m_table.bLevel > iHigh)

			continue;

		pool.push_back(pkMob->m_table.dwVnum);

	}



	if (pool.empty())

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnMetinResult ERR_NOTFOUND");

		return;

	}



	iCount = MIN(test_server ? 40 : 20, iCount);



	int iDone = 0;

	for (int i = 0; i < iCount; ++i)

	{

		const DWORD vnum = pool[number(0, (int)pool.size() - 1)];

		if (CHARACTER_MANAGER::instance().SpawnMobRange(vnum,

				ch->GetMapIndex(),

				ch->GetX() - number(200, 750),

				ch->GetY() - number(200, 750),

				ch->GetX() + number(200, 750),

				ch->GetY() + number(200, 750),

				true,

				true))

			++iDone;

	}



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSpawnMetinResult OK|%d|%d", iDone, iCount);

}



// /gmpanel_polyitem <mob vnum> - "Marmur przemiany" on the item tab. A

// polymorph marble is one item type (ITEM_POLYMORPH) whose socket 0 carries the

// monster: ItemProcess_Polymorph reads it there and refuses a zero, so the

// socket has to be set before the item reaches the bag. Straight into the GM's

// own inventory, no owner field - SetPolyItemResult has nowhere to show one.

ACMD(do_gmpanel_polyitem)

{

	char arg1[64];

	one_argument(argument, arg1, sizeof(arg1));



	DWORD dwMobVnum = 0;

	str_to_number(dwMobVnum, arg1);

	if (!dwMobVnum || !CMobManager::instance().Get(dwMobVnum))

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelPolyItemResult ERR_BADVNUM");

		return;

	}



	// Created and placed exactly as the engine's own do_polymorph_item does,

	// vnum included: 70104 is the marble in these serverfiles and the only one

	// that command has ever used. The item is destroyed again when the bag turns

	// out to have no cell for it, so nothing is left behind on the floor.

	LPITEM item = ITEM_MANAGER::instance().CreateItem(GM_PANEL_POLYMORPH_VNUM, 1, 0, true);

	if (!item)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelPolyItemResult ERR_NOITEM");

		return;

	}



	item->SetSocket(0, dwMobVnum);



	const int iEmptyPos = ch->GetEmptyInventory(item->GetSize());

	if (iEmptyPos == -1)

	{

		M2_DESTROY_ITEM(item);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelPolyItemResult ERR_NOSPACE");

		return;

	}



	item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));

	LogManager::instance().ItemLog(ch, item, "GMPANEL_POLYITEM", item->GetName());



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelPolyItemResult OK");

}



// /gmpanel_getaiweights - the seventeen sliders and switches on the Sterowanie

// Serwerem tab, read out of the running core rather than out of the file, so

// what the panel shows is what the bots are actually doing. See

// PlayerBotBuildWeightReport in playerbot_manager.h.

ACMD(do_gmpanel_getaiweights)

{

	char szReport[512];

	if (!PlayerBotBuildWeightReport(szReport, sizeof(szReport)))

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAIWeightsResult ERR_QUERY");

		return;

	}

	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelAIWeightsResult %s", szReport);

}



// /gmpanel_setaiweight <KEY>|<value> - one slider or switch. Written into

// playerbot_weights.tsv (the file the web panel writes and every core in the

// stack re-reads on a five-second clock), so the change survives a restart and

// reaches the other two cores as well as this one.

//

// SetAIWeightResult is quiet on OK - the slider's own position is the feedback -

// so only a refusal ever shows, and that is why the failure branches carry a

// code rather than a sentence.

ACMD(do_gmpanel_setaiweight)

{

	char arg1[128];

	one_argument(argument, arg1, sizeof(arg1));



	char* saveptr = NULL;

	char* keyField = strtok_r(arg1, "|", &saveptr);

	char* valueField = strtok_r(NULL, "|", &saveptr);

	if (!keyField || !valueField || !*keyField)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetAIWeightResult ERR_BADDATA");

		return;

	}



	// The key goes into a file an operator reads; nothing but a name.

	for (const char* p = keyField; *p; ++p)

	{

		if (!isalnum((unsigned char)*p) && *p != '_')

		{

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetAIWeightResult ERR_BADDATA");

			return;

		}

	}



	const long lValue = strtol(valueField, NULL, 10);

	if (!PlayerBotSetWeight(keyField, lValue))

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetAIWeightResult ERR_QUERY");

		return;

	}



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetAIWeightResult OK");

}



// /gmpanel_getrates - experience, item drop and yang, as percentages, out of

// the same player.web_admin_rates row the web panel's rates page edits.

// "<exp>|<drop>|<yang>", in that order, which is what SetRatesResult zips

// against GM_PANEL_RATE_FIELDS.

ACMD(do_gmpanel_getrates)

{

	std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(

			"SELECT name, value FROM player.web_admin_rates"));

	if (!msg.get() || msg->uiSQLErrno != 0 || !msg->Get() || !msg->Get()->pSQLResult)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelRatesResult ERR_QUERY");

		return;

	}



	// Absent rows are a hundred percent, which is what the game does with no

	// row at all - a fresh install has never opened the rates page.

	int iExp = 100, iDrop = 100, iYang = 100;

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(msg->Get()->pSQLResult)))

	{

		if (!row[0] || !row[1])

			continue;

		const int iValue = atoi(row[1]);

		if (!strcasecmp(row[0], "exp"))

			iExp = iValue;

		else if (!strcasecmp(row[0], "drop"))

			iDrop = iValue;

		else if (!strcasecmp(row[0], "yang"))

			iYang = iValue;

	}



	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelRatesResult %d|%d|%d", iExp, iDrop, iYang);

}



// /gmpanel_setrate <name>|<value> - one of the three, stored and no more. The

// game reads its rates out of CONFIG at startup, so the number only reaches the

// world on the next restart - which is the button directly below this one on

// the same tab, and why this does not restart anything by itself: a GM saving

// three fields would otherwise restart the server three times.

//

// 1..10000 is m2-rates' own range; refusing here means the request that

// eventually carries it can never be one it rejects.

ACMD(do_gmpanel_setrate)

{

	char arg1[128];

	one_argument(argument, arg1, sizeof(arg1));



	char* saveptr = NULL;

	char* nameField = strtok_r(arg1, "|", &saveptr);

	char* valueField = strtok_r(NULL, "|", &saveptr);

	if (!nameField || !valueField)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetRateResult ERR_BADDATA");

		return;

	}



	if (strcasecmp(nameField, "exp") && strcasecmp(nameField, "drop") &&

			strcasecmp(nameField, "yang"))

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetRateResult ERR_BADDATA");

		return;

	}



	const int iValue = atoi(valueField);

	if (iValue < 1 || iValue > 10000)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetRateResult ERR_RANGE");

		return;

	}



	// nameField is one of the three literals tested above, so the query below

	// carries nothing a client chose.

	char szQuery[256];

	snprintf(szQuery, sizeof(szQuery),

			"INSERT INTO player.web_admin_rates (name, value) VALUES ('%s', %d)"

			" ON DUPLICATE KEY UPDATE value = %d",

			!strcasecmp(nameField, "exp") ? "exp" :

			(!strcasecmp(nameField, "drop") ? "drop" : "yang"), iValue, iValue);



	std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(szQuery));

	if (!msg.get() || msg->uiSQLErrno != 0)

	{

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetRateResult ERR_QUERY");

		return;

	}



	// mt2009: the multipliers the world actually uses are the event flags

	// mob_exp / mob_item / mob_gold (+ _buyer twins) - what web_admin.quest

	// sets and what a restart re-reads (m2-rates). Set them now as well, so

	// the new rate is live at once; the row above keeps the panel's view of it.

	{

		const bool bExp = !strcasecmp(nameField, "exp");

		const bool bDrop = !strcasecmp(nameField, "drop");

		quest::CQuestManager::instance().RequestSetEventFlag(

				bExp ? "mob_exp" : (bDrop ? "mob_item" : "mob_gold"), iValue);

		quest::CQuestManager::instance().RequestSetEventFlag(

				bExp ? "mob_exp_buyer" : (bDrop ? "mob_item_buyer" : "mob_gold_buyer"), iValue);

	}

	sys_log(0, "GMPANEL: %s set rate %s = %d", ch->GetName(), nameField, iValue);

	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelSetRateResult OK|%s|%d", nameField, iValue);

}



// /gmpanel_restartserver - the restart button on the same tab, and the thing

// that makes the three rates above take effect.

//

// The stack already has exactly one way to restart the cores and it is not a

// signal: the game container's m2-rates polls $SPOOL/request every five seconds

// and, when the id is one it has not seen, writes the rates it carries and

// restarts. So this writes that request with the rates currently in the

// database - nothing changes unless the GM changed something - and lets the

// supervisor do the rest. The web panel's restart button is the same file.

//

// Written beside and renamed over, because m2-rates may read it at any moment.

ACMD(do_gmpanel_restartserver)

{

	int iExp = 100, iDrop = 100, iYang = 100;

	std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(

			"SELECT name, value FROM player.web_admin_rates"));

	if (msg.get() && msg->uiSQLErrno == 0 && msg->Get() && msg->Get()->pSQLResult)

	{

		MYSQL_ROW row;

		while ((row = mysql_fetch_row(msg->Get()->pSQLResult)))

		{

			if (!row[0] || !row[1])

				continue;

			const int iValue = atoi(row[1]);

			if (!strcasecmp(row[0], "exp"))

				iExp = iValue;

			else if (!strcasecmp(row[0], "drop"))

				iDrop = iValue;

			else if (!strcasecmp(row[0], "yang"))

				iYang = iValue;

		}

	}



	const char* szSpool = getenv("M2_RATES_SPOOL");

	if (!szSpool || !*szSpool)

		szSpool = "/opt/m2spool";



	char szPath[256];

	char szTemp[256];

	snprintf(szPath, sizeof(szPath), "%s/request", szSpool);

	snprintf(szTemp, sizeof(szTemp), "%s/request.gmpanel", szSpool);



	FILE* fp = fopen(szTemp, "w");

	if (!fp)

	{

		sys_err("GMPANEL: cannot write %s", szTemp);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelRestartResult ERR_SPOOL");

		return;

	}



	// The id is what m2-rates compares against the one it last acted on, so it

	// only has to differ from the previous one; the time is what its own status

	// line reports back to whoever asks.

	const time_t now = time(0);

	const bool bWritten =

			fprintf(fp, "id=gmpanel-%ld-%u\n", (long)now, (unsigned)ch->GetPlayerID()) > 0 &&

			fprintf(fp, "exp=%d\ndrop=%d\nyang=%d\n", iExp, iDrop, iYang) > 0 &&

			fprintf(fp, "time=%ld\n", (long)now) > 0;



	if (fclose(fp) != 0 || !bWritten || rename(szTemp, szPath) != 0)

	{

		unlink(szTemp);

		sys_err("GMPANEL: cannot replace %s", szPath);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelRestartResult ERR_SPOOL");

		return;

	}



	sys_log(0, "GMPANEL: %s asked for a restart (exp %d, drop %d, yang %d)",

			ch->GetName(), iExp, iDrop, iYang);

	ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelRestartResult OK");

}



// /gmpanel_warp_map <x_m> <y_m> - teleport GM-a w miejsce klikniete na mapie

// (M + Ctrl + lewy przycisk, uiminimap.py). Klient liczy pozycje LOKALNA dla

// mapy, na ktorej stoi (ulamek szerokosci atlasu * rozmiar mapy), a WarpSet

// chce globalnych - baze mapy zna tylko serwer, wiec przeliczenie jest tutaj.

// Natywne /warp przyjmuje wspolrzedne globalne i do tego sie nie nadaje.

ACMD(do_gmpanel_warp_map)

{

	char arg1[128], arg2[128];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));



	if (!*arg1 || !*arg2)

	{

		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: gmpanel_warp_map <x_m> <y_m>");

		return;

	}



	int x = 0, y = 0;

	str_to_number(x, arg1);

	str_to_number(y, arg2);

	if (x < 0 || y < 0)

		return;



	const long lMapIndex = ch->GetMapIndex();

	PIXEL_POSITION base;

	if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(lMapIndex, base))

	{

		ch->ChatPacket(CHAT_TYPE_INFO, "Nie znam bazy tej mapy (index %ld).", lMapIndex);

		return;

	}



	const long lX = base.x + (long)x * 100;

	const long lY = base.y + (long)y * 100;

	// Cel poza chodzalnym terenem (woda, skala, dziura w mapie) zostawilby GM-a

	// wiszacego w miejscu, z ktorego nie da sie ruszyc - wtedy nie ruszamy sie wcale.

	if (!SECTREE_MANAGER::instance().IsMovablePosition(lMapIndex, lX, lY))

	{

		ch->ChatPacket(CHAT_TYPE_INFO, "Tam nie da sie wejsc (%d, %d).", x, y);

		return;

	}



	sys_log(0, "GMPANEL: %s warp na mapie %ld do (%d, %d) = (%ld, %ld)",

			ch->GetName(), lMapIndex, x, y, lX, lY);

	ch->WarpSet(lX, lY, lMapIndex);

}



// ============================================================================

// "Zapisane miejsca" - wlasne punkty powrotu GM-a (2026-09-12)

//

// Dziala jak zwoj powrotu, tylko cel wskazuje sie samemu: "Zapisz" zapamietuje

// miejsce, w ktorym postac aktualnie stoi, "Wczytaj" tam wraca, a ponowne

// "Zapisz" na tym samym miejscu po prostu je nadpisuje.

//

// Pozycja pochodzi ZAWSZE z postaci po stronie serwera - klient przysyla tylko

// etykiete do wyswietlenia, wiec nie da sie tedy teleportowac gdziekolwiek.

// Kluczem jest konto, a nie postac: dzieki temu punkty przezywaja takze

// zmiane postaci na tym samym polaczeniu.

// ============================================================================



namespace

{

	const int GMPANEL_WAYPOINT_SLOTS = 5;



	bool EnsureWaypointTable()

	{

		static bool s_bEnsured = false;

		if (s_bEnsured)

			return true;



		const char* createTable =

				"CREATE TABLE IF NOT EXISTS common.gmpanel_waypoints ("

				"account_id INT UNSIGNED NOT NULL, "

				"slot TINYINT UNSIGNED NOT NULL, "

				"label VARCHAR(32) NOT NULL DEFAULT '', "

				"map_index INT NOT NULL DEFAULT 0, "

				"x INT NOT NULL DEFAULT 0, "

				"y INT NOT NULL DEFAULT 0, "

				"saved_at INT UNSIGNED NOT NULL DEFAULT 0, "

				"PRIMARY KEY (account_id, slot)"

				") ENGINE=InnoDB DEFAULT CHARSET=latin1";

		std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(createTable));

		if (!msg.get() || msg->uiSQLErrno != 0)

		{

			sys_err("GMPANEL_WAYPOINT: nie moge zalozyc common.gmpanel_waypoints");

			return false;

		}

		s_bEnsured = true;

		return true;

	}



	// Etykieta to jedyna rzecz, ktora przychodzi od klienta i ladu je w

	// zapytaniu SQL. Przepuszczamy wiec wylacznie znaki nazw - razem z

	// polskimi, ktore w tym kliencie przychodza jako bajty >= 0x80 - a

	// cudzyslowy, ukosniki i srredniki po prostu wypadaja.

	void SanitizeWaypointLabel(const char* c_pszSrc, char* pszDst, size_t dstSize)

	{

		size_t out = 0;

		for (const unsigned char* p = (const unsigned char*) c_pszSrc; *p && out + 1 < dstSize; ++p)

		{

			const unsigned char c = *p;

			if (isalnum(c) || c >= 0x80 || c == '_' || c == '-' || c == '.' ||

					c == '(' || c == ')' || c == '+' || c == ',')

				pszDst[out++] = (char) c;

		}

		pszDst[out] = '\0';



		if (out == 0)

			strlcpy(pszDst, "bez_nazwy", dstSize);

	}

}



ACMD(do_gmpanel_waypoint)

{

	char arg1[32], arg2[16], arg3[64];

	one_argument(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));



	LPDESC d = ch->GetDesc();

	if (!d || !EnsureWaypointTable())

		return;



	const DWORD dwAccountID = d->GetAccountTable().id;

	if (dwAccountID == 0)

		return;



	if (!strcmp(arg1, "list"))

	{

		char query[192];

		snprintf(query, sizeof(query),

				"SELECT slot, label FROM common.gmpanel_waypoints WHERE account_id = %u ORDER BY slot",

				dwAccountID);

		std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(query));

		if (!msg.get() || !msg->Get() || !msg->Get()->pSQLResult)

			return;



		MYSQL_ROW row;

		while ((row = mysql_fetch_row(msg->Get()->pSQLResult)) != NULL)

		{

			if (!row[0])

				continue;

			ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelWaypoint %s|%s", row[0], row[1] ? row[1] : "");

		}

		return;

	}



	int iSlot = 0;

	str_to_number(iSlot, arg2);

	if (iSlot < 1 || iSlot > GMPANEL_WAYPOINT_SLOTS)

	{

		ch->ChatPacket(CHAT_TYPE_INFO, "Zly numer zapisanego miejsca.");

		return;

	}



	if (!strcmp(arg1, "save"))

	{

		char label[32];

		SanitizeWaypointLabel(arg3, label, sizeof(label));



		char query[384];

		snprintf(query, sizeof(query),

				"REPLACE INTO common.gmpanel_waypoints "

				"(account_id, slot, label, map_index, x, y, saved_at) "

				"VALUES (%u, %d, '%s', %ld, %ld, %ld, %u)",

				dwAccountID, iSlot, label, ch->GetMapIndex(),

				(long) ch->GetX(), (long) ch->GetY(), (unsigned) get_global_time());

		std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(query));

		if (!msg.get() || msg->uiSQLErrno != 0)

		{

			ch->ChatPacket(CHAT_TYPE_INFO, "Blad bazy - miejsce niezapisane.");

			return;

		}



		ch->ChatPacket(CHAT_TYPE_COMMAND, "GMPanelWaypoint %d|%s", iSlot, label);

		ch->ChatPacket(CHAT_TYPE_INFO, "Zapisano miejsce %d: %s (%ld, %ld).",

				iSlot, label, (long) ch->GetX() / 100, (long) ch->GetY() / 100);

		return;

	}



	if (!strcmp(arg1, "load"))

	{

		char query[224];

		snprintf(query, sizeof(query),

				"SELECT x, y, label FROM common.gmpanel_waypoints WHERE account_id = %u AND slot = %d",

				dwAccountID, iSlot);

		std::unique_ptr<SQLMsg> msg(AccountDB::instance().DirectQuery(query));

		if (!msg.get() || !msg->Get() || !msg->Get()->pSQLResult || msg->Get()->uiNumRows == 0)

		{

			ch->ChatPacket(CHAT_TYPE_INFO, "To miejsce jest jeszcze puste.");

			return;

		}



		MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

		if (!row || !row[0] || !row[1])

			return;



		// Mape wyznacza sama wspolrzedna (tak samo jak zwykle /warp), wiec

		// instancje lochow tu nie wroca - i dobrze, bo kazde wejscie do lochu

		// tworzy nowa, a stara juz nie istnieje.

		if (!ch->WarpSet(strtol(row[0], NULL, 10), strtol(row[1], NULL, 10)))

		{

			ch->ChatPacket(CHAT_TYPE_INFO, "Nie moge teraz tam teleportowac.");

			return;

		}



		ch->ChatPacket(CHAT_TYPE_INFO, "Wracam do zapisanego miejsca %d: %s.",

				iSlot, row[2] ? row[2] : "");

		return;

	}



	ch->ChatPacket(CHAT_TYPE_INFO, "Usage: gmpanel_waypoint <save|load|list> <1-%d> [nazwa]",

			GMPANEL_WAYPOINT_SLOTS);

}


//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
