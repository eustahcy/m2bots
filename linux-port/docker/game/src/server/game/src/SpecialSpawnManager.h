class SpecialSpawnManager : public singleton<SpecialSpawnManager>
{
public:
	SpecialSpawnManager();
	virtual ~SpecialSpawnManager();

	bool	Initialize();
	void Setup();
	void	Destroy();

	std::map<DWORD, LPSPECIAL_SPAWN> m_map_specialSpawns;
	std::map<DWORD, std::vector<LPSPECIAL_SPAWN>> m_map_groupedSpecialSpawns;

private:
	bool LoadSpecialSpawn(const char* c_pszFileName);
	//void SpecialSpawnDoSpawn(LPSPECIAL_SPAWN special_spawn);
};


// Files shared by GameCore.top
