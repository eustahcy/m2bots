#include "stdafx.h"
#include "config.h"
#include "utils.h"
#include "constants.h"
#include "locale_service.h"
#include "text_file_loader.h"
#include "SpecialSpawnManager.h"
#include "special_spawn.h"
#include "sectree_manager.h"
#include "db.h"
#include <random>

SpecialSpawnManager::SpecialSpawnManager()
{
}

SpecialSpawnManager::~SpecialSpawnManager()
{
	Destroy();
}

void SpecialSpawnManager::Setup()
{
	const int FILE_NAME_LEN = 256;
	char szSpecialSpawnFileName[FILE_NAME_LEN];
	snprintf(szSpecialSpawnFileName, sizeof(szSpecialSpawnFileName),
		"%s/special_spawns.txt", LocaleService_GetBasePath().c_str());

	if (!LoadSpecialSpawn(szSpecialSpawnFileName))
	{
		sys_err("cannot load %s", szSpecialSpawnFileName);
		thecore_shutdown();
	}
}

void SpecialSpawnManager::Destroy()
{
	auto it = m_map_specialSpawns.begin();
	while (it != m_map_specialSpawns.end())
	{
		it->second->Destroy();
		M2_DELETE(it->second);
		++it;
	}

	m_map_specialSpawns.clear();
}

bool SpecialSpawnManager::LoadSpecialSpawn(const char* c_pszFileName)
{
	std::unique_ptr<SQLMsg> pkMsg(DBManager::instance().DirectQuery("SELECT vnum, channel, UNIX_TIMESTAMP() - UNIX_TIMESTAMP(kill_time) AS last_kill_delta FROM special_spawn"));
	SQLResult* pRes = pkMsg->Get();

	std::map<DWORD, DWORD> killTimeMap;
	if (pRes->uiNumRows > 0)
	{
		MYSQL_ROW data;
		int col;
		while ((data = mysql_fetch_row(pRes->pSQLResult)))
		{
			col = 0;
			int iVnum;
			str_to_number(iVnum, data[col++]);

			int iChannel;
			str_to_number(iChannel, data[col++]);

			if (iChannel != g_bChannel)
				continue;

			int kill_time;
			str_to_number(kill_time, data[col++]);

			killTimeMap[iVnum] = kill_time;
		}
	}

	CTextFileLoader loader;
	if (!loader.Load(c_pszFileName))
		return false;

	std::string stName("");
	DWORD nodeCount = loader.GetChildNodeCount();
	sys_log(0, "SPECIALSPAWN START nodeCount (%d)", nodeCount);
	for (DWORD i = 0; i < nodeCount; ++i)
	{
		loader.SetChildNode(i);
		loader.GetCurrentNodeName(&stName);
		
		int iVnum;
		if (!loader.GetTokenInteger("vnum", &iVnum))
		{
			sys_err("LoadSpecialSpawn : Syntax error %s : no vnum, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		int iSpawnVnum;
		if (!loader.GetTokenInteger("spawn_vnum", &iSpawnVnum))
		{
			sys_err("LoadSpecialSpawn : Syntax error %s : no spawn vnum, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		std::string sSpawnType("");
		if (!loader.GetTokenString("spawn_type", &sSpawnType))
		{
			sys_err("LoadSpecialSpawn : Syntax error %s : no spawnType, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		DWORD spawnType;
		if (sSpawnType == "mob")
			spawnType = SPECIAL_SPAWN_MOB;
		else if (sSpawnType == "group")
			spawnType = SPECIAL_SPAWN_GROUP;
		else {
			sys_err("LoadSpecialSpawn : Syntax error %s : undefined spawn type %s, node %s", sSpawnType.c_str(), c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		int iChannel;
		if (!loader.GetTokenInteger("channel", &iChannel))
		{
			iChannel = 0;
		}

		if (iChannel > 0 && iChannel != g_bChannel)
		{
			sys_log(0, "LoadSpecialSpawn : Spawn not available for current channel (cur %d, dest %d) node %s", g_bChannel, iChannel, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		int iMapIndex;
		if (!loader.GetTokenInteger("map_index", &iMapIndex))
		{
			sys_err("LoadSpecialSpawn : Syntax error %s : no mapIndex, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		if (!map_allow_find(iMapIndex))
		{
			sys_log(0, "SpecialSpawn: this core does not allow map %d. Skipping (%d)", iMapIndex, iVnum);
			loader.SetParentNode();
			continue;
		}

		int iTimeStart;
		int iTimeEnd;
		if (!loader.GetTokenRange("time", &iTimeStart, &iTimeEnd))
		{
			sys_err("LoadSpecialSpawn : Syntax error %s : no time, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		std::string sTimeType("");
		if (!loader.GetTokenString("time_type", &sTimeType))
		{
			sys_err("LoadSpecialSpawn : Syntax error %s : no timeType, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		DWORD timeType;
		if (sTimeType == "normal")
			timeType = SPECIAL_SPAWN_TIME_NORMAL;
		else if (sTimeType == "hour")
			timeType = SPECIAL_SPAWN_TIME_HOUR;
		else {
			sys_err("LoadSpecialSpawn : Syntax error %s : undefined time type %s, node %s", sTimeType.c_str(), c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		int spawnCount;
		if (!loader.GetTokenInteger("count", &spawnCount))
		{
			sys_err("LoadSpecialSpawn : Syntax error %s : no spawnCount, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		BOOL isSaveDestroyTime = 0;
		loader.GetTokenBoolean("save_kill", &isSaveDestroyTime);

		int iInitialMinSpawnDelay = 0;
		int iInitialMaxSpawnDelay = 0;
		loader.GetTokenRange("initial_delay", &iInitialMinSpawnDelay, &iInitialMaxSpawnDelay);
		int iInitialSpawnDelay = number(iInitialMinSpawnDelay, iInitialMaxSpawnDelay);

		LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(iMapIndex);
		if (pkSectreeMap == NULL) {
			sys_err("SPECIAL_SPAWN: SECTREE_MAP not found for #%ld", iMapIndex);
			loader.SetParentNode();
			continue;
		}

		std::string sEventFlag("");
		loader.GetTokenString("event_flag", &sEventFlag);

		int iNotifyLevel = 0;
		loader.GetTokenInteger("notify_level", &iNotifyLevel);

		sys_log(0, "SPECIAL_SPAWN REGISTERED: vnum %d spawnVnum %d (count %d) spawnType %s channel %d mapIndex %d time %d-%d timeType %s ",
			iVnum, iSpawnVnum, spawnCount, sSpawnType.c_str(), iChannel, iMapIndex, iTimeStart, iTimeEnd, sTimeType.c_str());

		LPSPECIAL_SPAWN pkSpecialSpawn = M2_NEW SpecialSpawn;
		pkSpecialSpawn->unique_vnum = iVnum;
		pkSpecialSpawn->vnum = iSpawnVnum;
		pkSpecialSpawn->spawnType = spawnType;
		pkSpecialSpawn->timeType = timeType;
		pkSpecialSpawn->channel = iChannel;
		pkSpecialSpawn->mapIndex = iMapIndex;
		pkSpecialSpawn->minSpawnTimeRange = iTimeStart;
		pkSpecialSpawn->maxSpawnTimeRange = iTimeEnd;
		pkSpecialSpawn->initialSpawnDelay = iInitialSpawnDelay == 0 ? number(3,6) : iInitialSpawnDelay;
		pkSpecialSpawn->maxSpawnCount = spawnCount;
		pkSpecialSpawn->isSaveDestroyTime = isSaveDestroyTime;
		pkSpecialSpawn->reqEventFlag = sEventFlag;
		pkSpecialSpawn->notifyLevel = iNotifyLevel;

		TTokenVector* pTok = NULL;
		for (int k = 1; k < 256; ++k)
		{
			char buf[4];
			snprintf(buf, sizeof(buf), "%d", k);
			if (loader.GetTokenVector(buf, &pTok))
			{
				int x = 0;
				int y = 0;
				int z = 0;
				str_to_number(x, pTok->at(0).c_str());
				str_to_number(y, pTok->at(1).c_str());
				str_to_number(z, pTok->at(2).c_str());
				sys_log(0, "possible spawnPoint %d %d rot %d", x, y, z);
				SpecialSpawnPosition pos{};
				pos.spawnVID = 0;
				pos.pos = PIXEL_POSITION{ pkSectreeMap->m_setting.iBaseX + x * 100, pkSectreeMap->m_setting.iBaseY + y * 100, z };
				pkSpecialSpawn->m_vec_spawnPositions.push_back(pos);
				continue;
			}

			break;
		}

		loader.SetParentNode();

		if (pkSpecialSpawn->m_vec_spawnPositions.size() < 1)
		{
			sys_err("LoadSpecialSpawn spawnPositions below 1");
			M2_DELETE(pkSpecialSpawn);
			continue;
		}

		if (spawnCount > pkSpecialSpawn->m_vec_spawnPositions.size())
		{
			sys_err("LoadSpecialSpawn spawn count is greater than spawn positions (%d).", iVnum);
			M2_DELETE(pkSpecialSpawn);
			return false;
		}

		DWORD last_kill_delta = -1;
		auto it = killTimeMap.find(iVnum);
		if (it != killTimeMap.end())
			last_kill_delta = it->second;

		pkSpecialSpawn->OnBoot(last_kill_delta);
		m_map_specialSpawns[iVnum] = pkSpecialSpawn;
	}

	return true;
}
// Files shared by GameCore.top
