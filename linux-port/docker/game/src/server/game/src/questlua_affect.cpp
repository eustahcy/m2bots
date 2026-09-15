#include "stdafx.h"
#include "config.h"
#include "questmanager.h"
#include "sectree_manager.h"
#include "char.h"
#include "affect.h"
#include "db.h"

namespace quest
{
	// "affect" Lua functions

	ALUA(affect_add)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager & q = CQuestManager::instance();

		BYTE applyOn = (BYTE) lua_tonumber(L, 1);

		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (applyOn >= POINT_MAX_NUM || applyOn < 1)
		{
			sys_err("apply is out of range : %d", applyOn);
			return 0;
		}

		if (ch->FindAffect(AFFECT_QUEST_START_IDX, applyOn))
			return 0;

		long value = (long) lua_tonumber(L, 2);
		long duration = (long) lua_tonumber(L, 3);

		ch->AddAffect(AFFECT_QUEST_START_IDX, applyOn, value, 0, duration, 0, false);

		return 0;
	}

	ALUA(affect_add_new)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();

		DWORD affectType = (DWORD)lua_tonumber(L, 1);

		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		BYTE pointType = (long)lua_tonumber(L, 2);

		if (pointType >= POINT_MAX_NUM)
		{
			sys_err("point is out of range : %d", pointType);
			return 0;
		}

		long value = (long)lua_tonumber(L, 3);
		long duration = (long)lua_tonumber(L, 4);
		DWORD flag = lua_isnumber(L, 5) ? (DWORD)lua_tonumber(L, 5) : AFF_NONE;
		bool bOverride = lua_isboolean(L, 6) ? (bool)lua_toboolean(L, 6) : false;
		bool bCube = lua_isboolean(L, 7) ? (bool)lua_toboolean(L, 7) : false;
		DWORD itemVnum = lua_isnumber(L, 8) ? (DWORD)lua_tonumber(L, 8) : 0;

		ch->AddAffect(affectType, pointType, value, flag, duration, 0, bOverride, bCube, itemVnum);

		return 0;
	}

	ALUA(affect_remove)
	{
		CQuestManager & q = CQuestManager::instance();
		int iType;

		if (lua_isnumber(L, 1))
		{
			iType = (int) lua_tonumber(L, 1);

			if (iType == 0)
				iType = q.GetCurrentPC()->GetCurrentQuestIndex() + AFFECT_QUEST_START_IDX;
		}
		else
			iType = q.GetCurrentPC()->GetCurrentQuestIndex() + AFFECT_QUEST_START_IDX;

		q.GetCurrentCharacterPtr()->RemoveAffect(iType);

		return 0;
	}

	ALUA(affect_remove_new)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		if (!lua_isnumber(L, 1))
		{
			return 0;
		}

		DWORD dwType = (DWORD)lua_tonumber(L, 1);
		BYTE bPoint = lua_isnumber(L, 2) ? (BYTE)lua_tonumber(L, 2) : POINT_NONE;
		CAffect* pkAff = ch->FindAffect(dwType, bPoint);
		if (pkAff)
			ch->RemoveAffect(pkAff);

		return 0;
	}

	ALUA(affect_remove_bad)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->RemoveBadAffect();
		return 0;
	}

	ALUA(affect_remove_good)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->RemoveGoodAffect();
		return 0;
	}

	// usage :	applyOn = affect.get_apply(AFFECT_TYPE)
	ALUA(affect_get_apply_on)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			return 0;
		}

		DWORD affectType = lua_tonumber(L, 1);

		CAffect* pkAff = ch->FindAffect(affectType);

		if ( pkAff != NULL )
			lua_pushnumber(L, pkAff->bApplyOn);
		else
			lua_pushnil(L);

		return 1;

	}

	ALUA(affect_add_collect)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager & q = CQuestManager::instance();

		BYTE point_type = (BYTE) lua_tonumber(L, 1);

		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (point_type >= POINT_MAX_NUM || point_type < 1)
		{
			sys_err("point is out of range : %d", point_type);
			return 0;
		}

		long value = (long) lua_tonumber(L, 2);

		CAffect* pkAff = ch->FindAffect(AFFECT_COLLECT, point_type);
		if (pkAff)
		{
			value += pkAff->lApplyValue;
		}

		ch->AddAffect(AFFECT_COLLECT, point_type, value, 0, INFINITE_AFFECT_DURATION, 0, true, true);

		return 0;
	}

	// playerbot/panel: affect.remove_collect (playerbotify.py).
	//
	// This engine binds remove_all_collect and not this one, so web_admin.quest
	// - which takes its own speed effects off by name before adding a new one,
	// rather than stripping everything the player earned - died on a nil
	// function, never wrote its result, and left the queue row holding the
	// quest's claim stamp for ever.
	//
	// affect_add_collect above sums into one affect per point type
	// (FindAffect(AFFECT_COLLECT, point_type)), so there is exactly one to take
	// off. r40250 matches on the value because there every call makes its own
	// affect; here that would never match an accumulated one, so the value is
	// accepted and ignored.
	ALUA(affect_remove_collect)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch == NULL)
			return 0;

		BYTE point_type = (BYTE) lua_tonumber(L, 1);

		if (point_type >= POINT_MAX_NUM || point_type < 1)
			return 0;

		CAffect* pkAff = ch->FindAffect(AFFECT_COLLECT, point_type);

		if (pkAff)
			ch->RemoveAffect(pkAff);

		return 0;
	}

	ALUA(affect_remove_all_collect)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_COLLECT);
		}

		return 0;
	}

	ALUA(affect_get_count)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("affect_get_count invalid argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch == NULL)
			return 0;

		DWORD dwAffectType = (DWORD)lua_tonumber(L, 1);
		lua_pushnumber(L, ch->CountAffect(dwAffectType));

		return 1;
	}

	ALUA(affect_get_duration)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("affect_get_duration invalid argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch == NULL)
			return 0;

		DWORD dwAffectType = (DWORD)lua_tonumber(L, 1);
		CAffect* pkAff = ch->FindAffect(dwAffectType);
		if (!pkAff)
		{
			lua_pushnumber(L, 0);
			return 1;
		}
		lua_pushnumber(L, pkAff->lDuration);
		return 1;
	}

	ALUA(affect_is_loaded)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		return ch->IsLoadedAffect();
	}

	void RegisterAffectFunctionTable()
	{
		luaL_reg affect_functions[] =
		{
			{ "add",				affect_add					},
			{ "add_new",			affect_add_new				},
			{ "remove",				affect_remove				},
			{ "remove_new",			affect_remove_new			},
			{ "remove_bad",			affect_remove_bad			},
			{ "remove_good",		affect_remove_good			},
			{ "add_collect",		affect_add_collect			},
			{ "remove_collect",		affect_remove_collect		},
			{ "remove_all_collect",	affect_remove_all_collect	},
			{ "get_apply_on",		affect_get_apply_on			},
			{ "get_count",			affect_get_count			},
			{ "get_duration",		affect_get_duration			},
			{ "is_loaded",			affect_is_loaded			},

			{ NULL,		NULL			}
		};

		CQuestManager::instance().AddLuaFunctionTable("affect", affect_functions);
	}
};
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
