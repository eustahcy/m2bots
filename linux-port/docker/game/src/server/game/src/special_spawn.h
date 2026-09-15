
enum SpecialSpawnSpawnTypes
{
	SPECIAL_SPAWN_MOB,
	SPECIAL_SPAWN_GROUP
};

enum SpecialSpawnTimeTypes
{
	SPECIAL_SPAWN_TIME_NORMAL,
	SPECIAL_SPAWN_TIME_HOUR
};

EVENTINFO(special_spawn_event_info)
{
	LPSPECIAL_SPAWN 	special_spawn;

	special_spawn_event_info()
		: special_spawn(0)
	{
	}
};

struct SpecialSpawnPosition
{
	DWORD spawnVID;
	PIXEL_POSITION pos;
};

class SpecialSpawn
{
public:
	SpecialSpawn();
	~SpecialSpawn();

	void Destroy();

	DWORD unique_vnum;
	DWORD vnum;
	BYTE spawnType;
	BYTE timeType;
	DWORD channel;
	DWORD mapIndex;
	DWORD minSpawnTimeRange;
	DWORD maxSpawnTimeRange;
	DWORD maxSpawnCount;
	DWORD initialSpawnDelay;
	bool isSaveDestroyTime;
	int notifyLevel{};
	std::vector<SpecialSpawnPosition> m_vec_spawnPositions;
	std::vector<LPCHARACTER> m_vec_spawnedChars;
	std::string reqEventFlag = "";

	LPEVENT	spawn_event;
	DWORD end_event_time;

	bool DoSpawn();
	void StartEvent(int delay = -1);
	DWORD GetSpawnDelay();
	DWORD GetTimeLeft();
	DWORD GetSpawnCount() { return m_vec_spawnedChars.size(); }
	DWORD GetSpawnRandomTime() { return number(minSpawnTimeRange, maxSpawnTimeRange); }

	bool IsSpawned();

	void OnBoot(DWORD last_kill_delta = -1);
	void OnSpawnedKill(LPCHARACTER ch);
	void OnSpawnedDestroy(LPCHARACTER ch);

	bool IsNotify() const { return notifyLevel > 0; }
};
// Files shared by GameCore.top
