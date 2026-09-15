// vim:ts=8 sw=4
#ifndef __INC_METIN_II_PLAYERHB_H__
#define __INC_METIN_II_PLAYERHB_H__

class PlayerHB : public singleton<PlayerHB>
{
    public:
	PlayerHB();
	virtual ~PlayerHB();

	bool	Initialize();

	void	Put(DWORD id);

    private:
	bool	Query(DWORD id);

	std::map<DWORD, time_t> m_map_data;
	std::string		m_stCreateTableQuery;
	std::string		m_stTableName;
	int			m_iExpireTime;
};

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
