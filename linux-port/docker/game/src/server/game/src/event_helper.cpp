#include "stdafx.h"
#include "constants.h"
#include "event_helper.h"
#include "config.h"
#include "sectree_manager.h"
#include "char_manager.h"

CEventHelperManager::CEventHelperManager()
{
	m_map_EventNpcSpawn.clear();
	m_map_EventNpcSpawn["beta_server"] = BETA_SPAWN;
	m_map_EventNpcSpawn["valentine_event"] = VALENTINE_SPAWN;
}

bool CEventHelperManager::Initialize()
{
	return true;
}

void CEventHelperManager::SpawnEventNpc(SEventNpcSpawn data)
{
	for (const auto& spawnPosition : data.spawnPositions) {
		if (map_allow_find(spawnPosition.lMapIndex))
		{
			PIXEL_POSITION posBase;
			if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(spawnPosition.lMapIndex, posBase))
			{
				sys_err("cannot get map base position %d", spawnPosition.lMapIndex);
				continue;
			}

			int iRot = spawnPosition.bDirection > 0 ? (spawnPosition.bDirection - 1) * 45 : -1;

			CHARACTER_MANAGER::instance().SpawnMob(
				data.dwMobRace, spawnPosition.lMapIndex,
				posBase.x + spawnPosition.lLocalX * 100,
				posBase.y + spawnPosition.lLocalY * 100, 0, false, iRot);
		}
	}
}

void CEventHelperManager::DespawnEventNpc(SEventNpcSpawn spawnData)
{
	CharacterVectorInteractor i;

	if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(spawnData.dwMobRace, i))
	{
		CharacterVectorInteractor::iterator it = i.begin();

		while (it != i.end())
			M2_DESTROY_CHARACTER(*it++);
	}
}

void CEventHelperManager::ProcessEventFlag(const std::string name, int prev_value, int value)
{
	if (name == "beta_server" && g_dwClientVersion >= 1000000)
		return;

	auto it = m_map_EventNpcSpawn.find(name);
	if (it == m_map_EventNpcSpawn.end())
		return;

	if (value && !prev_value)
	{
		SpawnEventNpc(it->second);
	}
	else if (!value && prev_value)
	{
		DespawnEventNpc(it->second);
	}
}

// Files shared by GameCore.top
