#ifndef P2P_MANAGER_H_
#define P2P_MANAGER_H_

#include <boost/unordered_map.hpp>

#include "../../common/stl.h"
#include "input.h"

typedef struct _CCI
{
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	DWORD	dwPID;
	BYTE	bEmpire;
	long	lMapIndex;
	BYTE	bChannel;

	int	iPremium;

	LPDESC	pkDesc;
} CCI;

class P2P_MANAGER : public singleton<P2P_MANAGER>
{
	public:
		P2P_MANAGER();
		~P2P_MANAGER();

		void			RegisterAcceptor(LPDESC d);
		void			UnregisterAcceptor(LPDESC d);

		void			RegisterConnector(LPDESC d);
		void			UnregisterConnector(LPDESC d);

		void			EraseUserByDesc(LPDESC d);

		void			FlushOutput();

		void			Boot(LPDESC d);

		void			Send(const void * c_pvData, int iSize, LPDESC except = NULL);

		template<typename T, std::enable_if_t<utils::IsRawV<T>>* = nullptr>
		void Send(const T& c_pvData, LPDESC except = nullptr) {
			Send(&c_pvData, sizeof(T), except);
		}
		template<typename C, std::enable_if_t<utils::IsContiguousV<C>>* = nullptr>
		void Send(const C& v, LPDESC except = nullptr) {
			Send(v.data(), v.size() * sizeof(typename C::value_type), except);
		}

		void			Login(LPDESC d, const TPacketGGLogin * p);
		void			Logout(const char * c_pszName);

		CCI *			Find(const char * c_pszName);
		CCI *			FindByPID(DWORD pid);

		int				GetCount();
		int				GetEmpireUserCount(int idx);
		// How many of the characters logged in on the other cores of this
		// channel are playerbots (0 = every empire).
		int				CountPlayerBots(int empire) const;
		int				GetDescCount();
		void			GetP2PHostNames(std::string& hostNames);

	private:
		void			Logout(CCI * pkCCI);

		CInputProcessor *	m_pkInputProcessor;
		int			m_iHandleCount;

		typedef boost::unordered_map<std::string, CCI *, stringhash> TCCIMap;
		typedef boost::unordered_map<DWORD, CCI*> TPIDCCIMap;

		TR1_NS::unordered_set<LPDESC> m_set_pkPeers;
		TCCIMap			m_map_pkCCI;
		TPIDCCIMap		m_map_dwPID_pkCCI;
		int			m_aiEmpireUserCount[EMPIRE_MAX_NUM];
};

#endif /* P2P_MANAGER_H_ */
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
