// questlua_command.cpp
/*
	26.03.2015r. Evor
	Mened¿er nowego eventu
	when write begin
		write.GetCommand()
	end
*/
#include "stdafx.h"
#include "questlua.h"
#include "questmanager.h"
#include <string>
int baseComandEndIndex = 0;
// sys_err redefine
#undef sys_err
#ifndef __WIN32__
#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

namespace quest
{
	ALUA(getMessage)
	{
		PC* curPC = CQuestManager::instance().GetCurrentPC();
		if (!curPC) { sys_err("QuestluaCommand:: UNKNOWN PC!"); return 1; }

		std::string fullCommand = curPC->GetCommand(), baseCommand = "";

		for (int i = 0; i < (int)fullCommand.length(); i++)
		{
			if (fullCommand[i] != ' ') { baseCommand += fullCommand[i]; }
			else {
				baseComandEndIndex = i + 1; // pomijamy spacje
				break;
			}
		}

		lua_pushstring(L, baseCommand.c_str());

		baseCommand.clear();
		fullCommand.clear();
		return 1;
	}

	ALUA(getFullMessage)
	{
		PC* curPC = CQuestManager::instance().GetCurrentPC();
		if (!curPC) { sys_err("QuestluaCommand:: UNKNOWN PC!"); return 1; }
		lua_pushstring(L, curPC->GetCommand());
		return 1;
	}

	ALUA(getArguments)
	{
		PC* curPC = CQuestManager::instance().GetCurrentPC();
		if (!curPC) { sys_err("QuestluaCommand:: UNKNOWN PC!"); return 1; }

		lua_newtable(L);

		int idx = 1;
		std::string fullCommand = curPC->GetCommand(), argument = "";

		for (int i = baseComandEndIndex; i < (int)fullCommand.length(); i++) {
			if (fullCommand[i] == ' ') { // znaleziono spacje, czyli kolejny argument
				lua_pushstring(L, argument.c_str());
				lua_rawseti(L, -2, idx++);
				argument.clear();
			}
			else {
				argument += fullCommand[i];
			}
		}

		lua_pushstring(L, argument.c_str());
		lua_rawseti(L, -2, idx++);
		argument.clear();

		return 1;
	}

	void RegisterCommandFunctionTable()
	{

		luaL_reg CommandFunctions[] =
		{
			{"get_message", getMessage},
			{"getmessage", getMessage},

			{"getarguments", getArguments},
			{"get_arguments", getArguments},

			{"get_full_message", getFullMessage},
			{"getfullmessage", getFullMessage},

			{ NULL, NULL }
		};

		CQuestManager::instance().AddLuaFunctionTable("mes", CommandFunctions);
	}

}
// Files shared by GameCore.top
