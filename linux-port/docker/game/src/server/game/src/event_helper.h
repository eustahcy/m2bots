class CEventHelperManager : public singleton<CEventHelperManager>
{
public:
	CEventHelperManager();

	bool Initialize();
	void ProcessEventFlag(const std::string name, int prev_value, int value);

protected:
	struct SEventNpcSpawnPosition
	{
		long	lMapIndex;
		long	lLocalX;
		long	lLocalY;
		BYTE	bDirection;
	};

	struct SEventNpcSpawn
	{
		DWORD dwMobRace;
		std::vector<SEventNpcSpawnPosition> spawnPositions;
	};

	std::map<std::string, SEventNpcSpawn> m_map_EventNpcSpawn;

	void SpawnEventNpc(SEventNpcSpawn spawnData);
	void DespawnEventNpc(SEventNpcSpawn spawnData);

	// SPAWN DATA
	SEventNpcSpawn BETA_SPAWN = {
		20999,
		{
			{ 1, 614, 562, 1 },
			{ 21, 613, 681, 4 },
			{ 41, 372, 702, 7 },
		}
	};

	SEventNpcSpawn VALENTINE_SPAWN = {
		9013,
		{
			{ 1, 609, 618, 4 },
			{ 21, 598, 665, 3 },
			{ 41, 318, 626, 2 },
		}
	};
};


// Files shared by GameCore.top
