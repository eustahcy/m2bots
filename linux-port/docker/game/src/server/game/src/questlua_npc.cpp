#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "questmanager.h"
#include "char.h"
#include "party.h"
#include "xmas_event.h"
#include "char_manager.h"
#include "shop_manager.h"
#include "guild.h"
#include "packet.h"
#include "buffer_manager.h"
#include "mob_manager.h"
#include "sectree_manager.h"

namespace quest
{
	// "npc" lua functions

	ALUA(npc_open_shop)
	{
		int iShopVnum = 0;

		if (lua_gettop(L) == 1)
		{
			if (lua_isnumber(L, 1))
				iShopVnum = (int) lua_tonumber(L, 1);
		}

		if (CQuestManager::instance().GetCurrentNPCCharacterPtr())
			CShopManager::instance().StartShopping(CQuestManager::instance().GetCurrentCharacterPtr(), CQuestManager::instance().GetCurrentNPCCharacterPtr(), iShopVnum);
		return 0;
	}

	ALUA(npc_is_pc)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (npc && npc->IsPC())
			lua_pushboolean(L, 1);
		else
			lua_pushboolean(L, 0);
		return 1;
	}

	ALUA(npc_get_empire)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (npc)
			lua_pushnumber(L, npc->GetEmpire());
		else
			lua_pushnumber(L, 0);
		return 1;
	}

	ALUA(npc_get_race)
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentNPCRace());
		return 1;
	}

	ALUA(npc_get_guild)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		CGuild* pGuild = NULL;
		if (npc)
			pGuild = npc->GetGuild();

		lua_pushnumber(L, pGuild ? pGuild->GetID() : 0);
		return 1;
	}

	ALUA(npc_get_remain_skill_book_count)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (!npc || npc->IsPC() || npc->GetRaceNum() != xmas::MOB_SANTA_VNUM)
		{
			lua_pushnumber(L, 0);
			return 1;
		}
		lua_pushnumber(L, MAX(0, npc->GetPoint(POINT_ATT_GRADE_BONUS)));
		return 1;
	}

	ALUA(npc_dec_remain_skill_book_count)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (!npc || npc->IsPC() || npc->GetRaceNum() != xmas::MOB_SANTA_VNUM)
		{
			return 0;
		}
		npc->SetPoint(POINT_ATT_GRADE_BONUS, MAX(0, npc->GetPoint(POINT_ATT_GRADE_BONUS)-1));
		return 0;
	}

	ALUA(npc_get_remain_hairdye_count)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (!npc || npc->IsPC() || npc->GetRaceNum() != xmas::MOB_SANTA_VNUM)
		{
			lua_pushnumber(L, 0);
			return 1;
		}
		lua_pushnumber(L, MAX(0, npc->GetPoint(POINT_DEF_GRADE_BONUS)));
		return 1;
	}

	ALUA(npc_dec_remain_hairdye_count)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (!npc || npc->IsPC() || npc->GetRaceNum() != xmas::MOB_SANTA_VNUM)
		{
			return 0;
		}
		npc->SetPoint(POINT_DEF_GRADE_BONUS, MAX(0, npc->GetPoint(POINT_DEF_GRADE_BONUS)-1));
		return 0;
	}

	ALUA(npc_is_quest)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		if (npc)
		{
			const std::string & r_st = q.GetCurrentQuestName();

			if (q.GetQuestIndexByName(r_st) == npc->GetQuestBy())
			{
				lua_pushboolean(L, 1);
				return 1;
			}
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	ALUA(npc_kill)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		ch->SetQuestNPCID(0);
		if (npc)
			npc->DeadNoReward(); // @fixme188 from Dead()
		return 0;
	}

	ALUA(npc_purge)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		ch->SetQuestNPCID(0);
		if (npc)
		{
			M2_DESTROY_CHARACTER(npc);
		}
		return 0;
	}

	ALUA(npc_is_near)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		lua_Number dist = 10;

		if (lua_isnumber(L, 1))
			dist = lua_tonumber(L, 1);

		if (ch == NULL || npc == NULL)
		{
			lua_pushboolean(L, false);
		}
		else
		{
			lua_pushboolean(L, DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY()) < dist*100);
		}

		return 1;
	}

	ALUA(npc_is_near_vid)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid vid");
			lua_pushboolean(L, 0);
			return 1;
		}

		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = CHARACTER_MANAGER::instance().Find((DWORD)lua_tonumber(L, 1));
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		lua_Number dist = 10;

		if (lua_isnumber(L, 2))
			dist = lua_tonumber(L, 2);

		if (ch == NULL || npc == NULL)
		{
			lua_pushboolean(L, false);
		}
		else
		{
			lua_pushboolean(L, DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY()) < dist*100);
		}

		return 1;
	}

	ALUA(npc_unlock)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		if ( npc != NULL )
		{
			if (npc->IsPC())
				return 0;

			if (npc->GetQuestNPCID() == ch->GetPlayerID())
			{
				npc->SetQuestNPCID(0);
			}
		}
		return 0;
	}

	ALUA(npc_lock)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		if (!npc || npc->IsPC())
		{
			lua_pushboolean(L, TRUE);
			return 1;
		}

		if (npc->GetQuestNPCID() == 0 || npc->GetQuestNPCID() == ch->GetPlayerID())
		{
			npc->SetQuestNPCID(ch->GetPlayerID());
			lua_pushboolean(L, TRUE);
		}
		else
		{
			lua_pushboolean(L, FALSE);
		}

		return 1;
	}

	ALUA(npc_get_leader_vid)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		LPPARTY party = npc->GetParty();
		LPCHARACTER leader = party->GetLeader();

		if (leader)
			lua_pushnumber(L, leader->GetVID());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	ALUA(npc_get_vid)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		lua_pushnumber(L, npc ? npc->GetVID() : -1);
		return 1;
	}

	ALUA(npc_get_vid_attack_mul)
	{
		//CQuestManager& q = CQuestManager::instance();

		lua_Number vid = lua_tonumber(L, 1);
		LPCHARACTER targetChar = CHARACTER_MANAGER::instance().Find(vid);

		if (targetChar)
			lua_pushnumber(L, targetChar->GetAttMul());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	ALUA(npc_set_vid_attack_mul)
	{
		//CQuestManager& q = CQuestManager::instance();

		lua_Number vid = lua_tonumber(L, 1);
		lua_Number attack_mul = lua_tonumber(L, 2);

		LPCHARACTER targetChar = CHARACTER_MANAGER::instance().Find(vid);

		if (targetChar)
			targetChar->SetAttMul(attack_mul);

		return 0;
	}

	ALUA(npc_get_vid_damage_mul)
	{
		//CQuestManager& q = CQuestManager::instance();

		lua_Number vid = lua_tonumber(L, 1);
		LPCHARACTER targetChar = CHARACTER_MANAGER::instance().Find(vid);

		if (targetChar)
			lua_pushnumber(L, targetChar->GetDamMul());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	ALUA(npc_set_vid_damage_mul)
	{
		//CQuestManager& q = CQuestManager::instance();

		lua_Number vid = lua_tonumber(L, 1);
		lua_Number damage_mul = lua_tonumber(L, 2);

		LPCHARACTER targetChar = CHARACTER_MANAGER::instance().Find(vid);

		if (targetChar)
			targetChar->SetDamMul(damage_mul);

		return 0;
	}
#ifdef ENABLE_NEWSTUFF
	ALUA(npc_get_level0)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		lua_pushnumber(L, npc->GetLevel());
		return 1;
	}

	ALUA(npc_get_name0)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		lua_pushstring(L, npc->GetName());
		return 1;
	}

	ALUA(npc_get_pid0)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		lua_pushnumber(L, npc->GetPlayerID());
		return 1;
	}

	ALUA(npc_get_vnum0)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		lua_pushnumber(L, npc->GetRaceNum());
		return 1;
	}

	ALUA(npc_get_type0)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		lua_pushnumber(L, npc ? npc->GetMobTable().bType : 0);
		return 1;
	}

	ALUA(npc_is_available0)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		lua_pushboolean(L, npc!=NULL);
		return 1;
	}

	ALUA(npc_select_vid0)
	{
		LPCHARACTER npc_old = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		LPCHARACTER npc_new = CHARACTER_MANAGER::instance().Find(lua_tonumber(L, 1));
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (npc_new)
			ch->SetQuestNPCID(npc_new->GetVID());
		lua_pushnumber(L, npc_old->GetVID());
		return 1;
	}

	ALUA(npc_talk)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (!npc)
			return 0;

		std::string text = lua_tostring(L, 1);
		if (text.empty())
			return 0;

		struct packet_chat pack_chat{};
		pack_chat.header	= HEADER_GC_CHAT;
		pack_chat.size  = sizeof(struct packet_chat) + text.size() + 1;
		pack_chat.type	  = CHAT_TYPE_TALKING;
		pack_chat.id		= npc->GetVID();
		pack_chat.bEmpire   = 0;

		TEMP_BUFFER buf{};
		buf.write(&pack_chat, sizeof(struct packet_chat));
		buf.write(text.c_str(), text.size() + 1);

		npc->PacketAround(buf.read_peek(), buf.size());
		return 0;
	}

	ALUA(npc_set_string_flag)
	{
		if (lua_gettop(L) < 2)
		{
			sys_err("npc_set_string_flag : not enough arguments!");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (npc) {
			std::string flag = lua_tostring(L, 1);
			int value = lua_tonumber(L, 2);
			npc->SetSpecialFlag(flag, value, true, false);
		}
		return 0;
	}

	ALUA(npc_get_string_flag)
	{
		if (lua_gettop(L) < 1)
		{
			sys_err("npc_get_string_flag : not enough arguments!");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (npc) {
			std::string flag = lua_tostring(L, 1);
			lua_pushnumber(L, npc->GetSpecialFlag(flag));
			return 1;
		}
		return 0;
	}

	ALUA(npc_is_dead)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (npc) {
			lua_pushboolean(L, npc->IsDead());
			return 1;
		}

		lua_pushboolean(L, FALSE);
		return 1;
	}

	ALUA(npc_is_custom_flag2)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER target = q.GetCurrentNPCCharacterPtr();
		if (!target)
			return 0;

		DWORD flag = lua_tonumber(L, 1);

		if (target && target->IsCustomNpcFlag(flag))
		{
			lua_pushboolean(L, true);
		}
		else
		{
			lua_pushboolean(L, FALSE);
		}
		return 1;
	}

	ALUA(npc_set_custom_npc_flag2)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER target = q.GetCurrentNPCCharacterPtr();
		if (!target)
			return 0;

		DWORD flag = lua_tonumber(L, 1);
		bool is_add = lua_isboolean(L, 2) ? lua_toboolean(L, 2) : true;

		if (target && target->IsNPC())
		{
			target->SetCustomNpcFlag(flag, is_add);
		}
		return 0;
	}

	ALUA(npc_set_custom_npc_flag)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
			return 0;

		DWORD vid = lua_tonumber(L, 1);
		DWORD flag = lua_tonumber(L, 2);
		bool is_add = lua_isboolean(L, 3) ? lua_toboolean(L, 3) : true;
		LPCHARACTER target = CHARACTER_MANAGER::instance().Find(vid);

		if (target && target->IsNPC())
		{
			target->SetCustomNpcFlag(flag, is_add);
		}
		return 0;
	}

	ALUA(npc_exist)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		DWORD vid = lua_tonumber(L, 1);
		LPCHARACTER target = CHARACTER_MANAGER::instance().Find(vid);

		if (target)
		{
			lua_pushboolean(L, TRUE);
		}
		else
		{
			lua_pushboolean(L, FALSE);
		}
		return 1;
	}

	ALUA(npc_say)
	{
		// metin to takie gowno ze daj spokoj
		// zakomentowany kod powoduje ze ponizszy sie nie wykonuje
		//if (lua_isstring(L, 1))
		//{
		//	CQuestManager& q = CQuestManager::instance();
		//	LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		//	std::string text = lua_tostring(L, 1);

		//	if (!npc)
		//		return 0;

		//	npc->MonsterChat(text.c_str());
		//	return 0;
		//}

		if (!lua_isnumber(L, 1) || !lua_isstring(L, 2))
			return 0;

		DWORD vid = lua_tonumber(L, 1);
		LPCHARACTER npc = CHARACTER_MANAGER::instance().Find(vid);
		std::string text = lua_tostring(L, 2);

		if (!npc)
			return 0;

		npc->MonsterChat(text.c_str());
		return 0;
	}

	ALUA(npc_say_me)
	{
		if (!lua_isstring(L, 1))
			return 0;

		LPCHARACTER pc = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pc)
			return 0;

		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();

		if (!npc)
			return 0;

		std::string text = lua_tostring(L, 1);
		npc->MonsterChatPlayer(pc, text.c_str());
		return 0;
	}

	ALUA(npc_select)
	{
		LPCHARACTER cNPC = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		LPCHARACTER pc = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1) || !pc)
		{
			sys_err("WRONG ARGUMENT!!");
			return 0;
		}

		DWORD tVID = (DWORD)lua_tonumber(L, 1);

		if (!cNPC)
		{
			pc->SetQuestNPCID(tVID);
			lua_pushnumber(L, tVID);
			return 1;
		}

		DWORD oldVID = cNPC->GetVID();

		LPCHARACTER npc = CHARACTER_MANAGER::instance().Find(tVID);

		if (!npc)
		{
			lua_pushnumber(L, 0);
		}
		else
		{
			pc->SetQuestNPCID(npc->GetVID());
			lua_pushnumber(L, oldVID);
		}

		return 1;
	}

	ALUA(npc_goto)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
			return 0;

		LPCHARACTER cNPC = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (!cNPC)
			return 0;

		long x = lua_tonumber(L, 1) * 100;
		long y = lua_tonumber(L, 2) * 100;

		LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(cNPC->GetMapIndex());

		long dest_x = pMap->m_setting.iBaseX + x;
		long dest_y = pMap->m_setting.iBaseY + y;

		cNPC->Goto(dest_x, dest_y);
		cNPC->SetRotationToXY(dest_x, dest_y);
		cNPC->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
		return 0;
	}

	ALUA(npc_get_point) {
		if (lua_gettop(L) < 2)
		{
			sys_err("npc_get_point : not enough arguments!");
			return 0;
		}
		DWORD tVID = (DWORD)lua_tonumber(L, 1);
		int			 type = lua_tonumber(L, 2);

		LPCHARACTER npc = CHARACTER_MANAGER::instance().Find(tVID);

		if (!npc)
			return 0;

		lua_pushnumber(L, npc->GetPoint(type));
		return 1;
	}

	ALUA(npc_point_change) {
		if (lua_gettop(L) < 3)
		{
			sys_err("npc_get_point : not enough arguments!");
			return 0;
		}
		DWORD tVID = (DWORD)lua_tonumber(L, 1);
		int			 type = lua_tonumber(L, 2);
		int			 value = lua_tonumber(L, 3);

		LPCHARACTER npc = CHARACTER_MANAGER::instance().Find(tVID);

		if (!npc)
			return 0;

		npc->PointChange(type, value);
		return 0;
	}

	ALUA(npc_update_packet)
	{
		if (lua_gettop(L) < 1)
		{
			sys_err("npc_update_packet : not enough arguments!");
			return 0;
		}

		WORD tVID = (DWORD)lua_tonumber(L, 1);
		LPCHARACTER npc = CHARACTER_MANAGER::instance().Find(tVID);
		if (!npc)
			return 0;
		npc->UpdatePacket();
		return 0;
	}

	ALUA(npc_get_x)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (npc)
			lua_pushnumber(L, npc->GetX() / 100);
		else
			lua_pushnumber(L, -1);
		return 1;
	}

	ALUA(npc_get_y)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (npc)
			lua_pushnumber(L, npc->GetY() / 100);
		else
			lua_pushnumber(L, -1);
		return 1;
	}

	ALUA(npc_distance_to_me)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER pc = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (pc && npc)
			lua_pushnumber(L, pc->DistanceTo(npc));
		else
			lua_pushnumber(L, -1);
		return 1;
	}

	ALUA(npc_distance_to_point)
	{
		if (lua_gettop(L) < 2)
		{
			sys_err("npc_distance_to_point : not enough arguments!");
			return 0;
		}
		int x = lua_tonumber(L, 1);
		int y = lua_tonumber(L, 2);

		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (npc)
		{
			lua_pushnumber(L, DISTANCE_APPROX(npc->GetX() - x, npc->GetY() - y));
		}
		else
			lua_pushnumber(L, -1);
		return 1;
	}

	ALUA(npc_get_drop_item)
	{
		CQuestManager& q = CQuestManager::instance();

		if (lua_isnumber(L, 1))
		{
			DWORD mob_vnum = lua_tonumber(L, 1);
			const CMob* pkMob = CMobManager::instance().Get(mob_vnum);
			if (pkMob) {
				lua_pushnumber(L, pkMob->m_table.dwDropItemVnum);
				return 1;
			}
		}

		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (npc) {
			lua_pushnumber(L, npc->GetMobDropItemVnum());
			return 1;
		}

		lua_pushnumber(L, 0);
		return 1;
	}

	ALUA(npc_get_level)
	{
		CQuestManager& q = CQuestManager::instance();

		if (lua_isnumber(L, 1))
		{
			DWORD mob_vnum = lua_tonumber(L, 1);
			const CMob* pkMob = CMobManager::instance().Get(mob_vnum);
			if (pkMob) {
				lua_pushnumber(L, pkMob->m_table.bLevel);
				return 1;
			}
		}

		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (npc) {
			lua_pushnumber(L, npc->GetLevel());
			return 1;
		}

		lua_pushnumber(L, 0);
		return 1;
	}

	ALUA(npc_get_rank)
	{
		CQuestManager& q = CQuestManager::instance();

		if (lua_isnumber(L, 1))
		{
			DWORD mob_vnum = lua_tonumber(L, 1);
			const CMob* pkMob = CMobManager::instance().Get(mob_vnum);
			if (pkMob) {
				lua_pushnumber(L, pkMob->m_table.bRank);
				return 1;
			}
		}

		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (npc) {
			lua_pushnumber(L, npc->GetMobRank());
			return 1;
		}

		lua_pushnumber(L, 0);
		return 1;
	}

	ALUA(npc_is_miniboss)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (npc) {
			lua_pushboolean(L, IsMiniBoss(npc->GetRaceNum()));
			return 1;
		}
		lua_pushboolean(L, FALSE);
		return 1;
	}

#endif

	void RegisterNPCFunctionTable()
	{
		luaL_reg npc_functions[] =
		{
			{ "getrace",			npc_get_race			},
			{ "get_race",			npc_get_race			},
			{ "open_shop",			npc_open_shop			},
			{ "get_empire",			npc_get_empire			},
			{ "is_pc",				npc_is_pc			},
			{ "is_quest",			npc_is_quest			},
			{ "kill",				npc_kill			},
			{ "purge",				npc_purge			},
			{ "is_near",			npc_is_near			},
			{ "is_near_vid",			npc_is_near_vid			},
			{ "lock",				npc_lock			},
			{ "unlock",				npc_unlock			},
			{ "get_guild",			npc_get_guild			},
			{ "get_leader_vid",		npc_get_leader_vid	},
			{ "get_vid",			npc_get_vid	},
			{ "get_vid_attack_mul",		npc_get_vid_attack_mul	},
			{ "set_vid_attack_mul",		npc_set_vid_attack_mul	},
			{ "get_vid_damage_mul",		npc_get_vid_damage_mul	},
			{ "set_vid_damage_mul",		npc_set_vid_damage_mul	},

			// X-mas santa special
			{ "get_remain_skill_book_count",	npc_get_remain_skill_book_count },
			{ "dec_remain_skill_book_count",	npc_dec_remain_skill_book_count },
			{ "get_remain_hairdye_count",	npc_get_remain_hairdye_count	},
			{ "dec_remain_hairdye_count",	npc_dec_remain_hairdye_count	},
#ifdef ENABLE_NEWSTUFF
			{ "get_level0",			npc_get_level0},	// [return lua number]
			{ "get_level",			npc_get_level0},	// alias
			{ "get_name0",			npc_get_name0},		// [return lua string]
			{ "get_name",			npc_get_name0},		// alias
			{ "get_pid0",			npc_get_pid0},		// [return lua number]
			{ "get_pid",			npc_get_pid0},		// alias
			{ "get_vnum0",			npc_get_vnum0},		// [return lua number]
			{ "get_vnum",			npc_get_vnum0},		// alias
			{ "get_type0",			npc_get_type0},		// [return lua number]
			{ "get_type",			npc_get_type0},		// alias
			{ "is_available0",		npc_is_available0},	// [return lua boolean]
			{ "is_available",		npc_is_available0},	// alias
			{ "select_vid0",		npc_select_vid0},	// [return lua number]
			{ "select_vid",			npc_select_vid0},	// alias
			{ "talk",				npc_talk},			// [return nothing]
#endif
			{ "set_string_flag",		npc_set_string_flag},
			{ "get_string_flag",		npc_get_string_flag},

			{ "is_dead",				npc_is_dead					},
			{ "is_custom_flag2",		npc_is_custom_flag2			},
			{ "set_custom_npc_flag",	npc_set_custom_npc_flag		},
			{ "set_custom_npc_flag2",	npc_set_custom_npc_flag2	},
			{ "exist",					npc_exist	},
			{ "say",					npc_say	},
			{ "say_me",					npc_say_me	},
			{ "go_to",					npc_goto},
			{ "select",					npc_select},
			{ "get_point",				npc_get_point},
			{ "point_change",			npc_point_change},
			{ "update_packet",			npc_update_packet},
			{ "get_x",					npc_get_x},
			{ "get_y",					npc_get_y},
			{ "distance_to_me",			npc_distance_to_me},
			{ "distance_to_point",		npc_distance_to_point},
			{ "get_drop_item",			npc_get_drop_item},
			{ "get_level",				npc_get_level},
			{ "get_rank",			npc_get_rank},
			{ "is_miniboss",			npc_is_miniboss},
			{ NULL,				NULL			    	}
		};

		CQuestManager::instance().AddLuaFunctionTable("npc", npc_functions);
	}
};
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
