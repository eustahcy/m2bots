#include "stdafx.h"
#include "special_spawn.h"
#include "char.h"
#include "char_manager.h"
#include "config.h"
#include "utils.h"
#include "db.h"
#include "questmanager.h"

SpecialSpawn::SpecialSpawn()
	: unique_vnum(0), vnum(0), spawnType(0), timeType(0), channel(0), mapIndex(0), maxSpawnCount(0), isSaveDestroyTime(false), spawn_event(NULL)
{
	m_vec_spawnedChars.clear();
	minSpawnTimeRange = 0;
	maxSpawnTimeRange = 0;
	initialSpawnDelay = 0;
}

SpecialSpawn::~SpecialSpawn()
{
}

void SpecialSpawn::Destroy()
{
	spawn_event = NULL;
	m_vec_spawnedChars.clear();
}

bool SpecialSpawn::DoSpawn()
{
	if (reqEventFlag.length() > 0 && quest::CQuestManager::instance().GetEventFlag(reqEventFlag) < 1)
		return false;

	if (GetSpawnCount() >= maxSpawnCount)
		return false;
	
	int spawnIndex = -1;
	while (spawnIndex < 0)
	{
		int n = number(0, m_vec_spawnPositions.size() - 1);

		LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(m_vec_spawnPositions[n].spawnVID);
		if (!ch || ch->IsDead())
			spawnIndex = n;
	}

	auto& randomPosition = m_vec_spawnPositions[spawnIndex];
	LPCHARACTER ch = nullptr;
	if (spawnType == SPECIAL_SPAWN_MOB)
	{
		int iRot = randomPosition.pos.z > 0 ? (randomPosition.pos.z - 1) * 45 : -1;
		ch = CHARACTER_MANAGER::Instance().SpawnMob(vnum, mapIndex, randomPosition.pos.x, randomPosition.pos.y, 0, false, iRot);

		if (vnum == 2307) // TRENT WORLDBOSS
		{
			SendNoticeMap(LC_TEXT("WORLDBOSS_TRENT2_SPAWN"), mapIndex, true);
		}
	}
	else if (spawnType == SPECIAL_SPAWN_GROUP)
	{
		ch = CHARACTER_MANAGER::Instance().SpawnGroup(vnum, mapIndex, randomPosition.pos.x, randomPosition.pos.y, randomPosition.pos.x, randomPosition.pos.y, NULL, false);
	}

	if (ch) {

		if (IsNotify())
		{
			char buf[512];
			sprintf(buf, LC_TEXT("%s has spawned!"), ch->GetName());
			SendNoticeMap(buf, mapIndex, false, notifyLevel, true);
		}

		m_vec_spawnedChars.emplace_back(ch);
		ch->m_pkSpecialSpawn = this;
		randomPosition.spawnVID = ch->GetVID();
		return true;
	}

	return false;
}

EVENTFUNC(special_spawn_event)
{
	special_spawn_event_info* info = dynamic_cast<special_spawn_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("special_spawn_event> <Factor> Null pointer");
		return 0;
	}

	LPSPECIAL_SPAWN	special_spawn = info->special_spawn;
	if (special_spawn == NULL)
	{
		special_spawn = nullptr;
		return 0;
	}

	int delta = special_spawn->maxSpawnCount - special_spawn->GetSpawnCount();
	if (delta < 1)
	{
		special_spawn = nullptr;
		return 0;
	}

	for (int i=0; i < delta; i++)
		special_spawn->DoSpawn();

	if (special_spawn->GetSpawnCount() < special_spawn->maxSpawnCount) {
		return PASSES_PER_SEC(special_spawn->GetSpawnRandomTime());
	}
	special_spawn = nullptr;
	return 0;
}

DWORD SpecialSpawn::GetSpawnDelay()
{
	auto randomSpawnTime = GetSpawnRandomTime();

	if (timeType == SPECIAL_SPAWN_TIME_NORMAL) {
		return randomSpawnTime;
	}
	else if (timeType == SPECIAL_SPAWN_TIME_HOUR) {
		time_t currentTime = time(nullptr);
		struct tm* localTime = localtime(&currentTime);
		long long currentSeconds = localTime->tm_hour * 3600 + localTime->tm_min * 60 + localTime->tm_sec;
		long long remainingTime = randomSpawnTime - currentSeconds;

		if (remainingTime <= 0) {
			remainingTime += 86400;
		}

		return remainingTime;
	}

	return randomSpawnTime;
}

DWORD SpecialSpawn::GetTimeLeft()
{
	if (IsSpawned())
	{
		return 0;
	}

	const int delta = end_event_time - get_global_time();
	return MAX(delta, 0);
}

bool SpecialSpawn::IsSpawned()
{
	int spawnCount = GetSpawnCount();
	if (spawnCount < 1)
		return false;

	for (auto ch : m_vec_spawnedChars)
	{
		if (!ch)
			spawnCount--;
	}

	return spawnCount < maxSpawnCount;
}


void SpecialSpawn::StartEvent(int delay)
{
	if (spawn_event)
		spawn_event = NULL;


	special_spawn_event_info* info = AllocEventInfo<special_spawn_event_info>();
	info->special_spawn = this;

	if (timeType == SPECIAL_SPAWN_TIME_NORMAL) {
		if (delay < 1)
			delay = GetSpawnDelay();
	}
	else if (timeType == SPECIAL_SPAWN_TIME_HOUR) {
		delay = GetSpawnDelay();
	}
	
	end_event_time = get_global_time() + delay;
	spawn_event = event_create(special_spawn_event, info, PASSES_PER_SEC(delay));
}

void SpecialSpawn::OnBoot(DWORD last_kill_delta)
{
	int delay = initialSpawnDelay;
	if (timeType == SPECIAL_SPAWN_TIME_NORMAL && last_kill_delta >= 0) {
		auto spawnTime = GetSpawnRandomTime();
		if (last_kill_delta < spawnTime) {
			delay = spawnTime - last_kill_delta;
		}
	}
	StartEvent(delay);
}

void SpecialSpawn::OnSpawnedKill(LPCHARACTER ch)
{
	if (isSaveDestroyTime)
		DBManager::instance().Query("REPLACE INTO special_spawn VALUES (%d, %d, NOW())", unique_vnum, g_bChannel);
}

void SpecialSpawn::OnSpawnedDestroy(LPCHARACTER ch)
{
	if (ch)
		std::erase(m_vec_spawnedChars, ch);

	StartEvent();

	if (ch && IsNotify())
	{
		char buf[512];
		sprintf(buf, LC_TEXT("%s has been killed!"), ch->GetName());
		SendNoticeMap(buf, mapIndex, false, notifyLevel, true);
	}
}

// Files shared by GameCore.top
