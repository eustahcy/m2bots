#ifndef __INC_METIN_II_GAME_DESC_MANAGER_H__
#define __INC_METIN_II_GAME_DESC_MANAGER_H__

#include <boost/unordered_map.hpp>


#include "../../common/stl.h"
#include "../../common/length.h"
#include "IFileMonitor.h"

class CLoginKey;
class CClientPackageCryptInfo;
typedef struct SPacketGGHandshakeValidate TPacketGGHandshakeValidate;

class DESC_MANAGER : public singleton<DESC_MANAGER>
{
	public:
		typedef TR1_NS::unordered_set<LPDESC>			DESC_SET;
		typedef TR1_NS::unordered_set<LPCLIENT_DESC>	CLIENT_DESC_SET;
		typedef std::map<int, LPDESC>					DESC_HANDLE_MAP;
		typedef std::map<DWORD, LPDESC>					DESC_HANDSHAKE_MAP;
		typedef std::map<DWORD, LPDESC>					DESC_ACCOUNTID_MAP;
		typedef boost::unordered_map<std::string, LPDESC>	DESC_LOGINNAME_MAP;
		typedef std::map<DWORD, DWORD>					DESC_HANDLE_RANDOM_KEY_MAP;

	public:
		DESC_MANAGER();
		virtual ~DESC_MANAGER();

		void			Initialize();
		void			Destroy();

		LPDESC			AcceptDesc(LPFDWATCH fdw, socket_t s);
		LPDESC			AcceptP2PDesc(LPFDWATCH fdw, socket_t s);
		LPDESC			CreateBotDesc(BYTE bEmpire);
		void			DestroyLoginKey(const std::string_view msg) const;
		void			DestroyLoginKey(const LPDESC d) const;
		void			DestroyDesc(LPDESC d, bool erase_from_set = true);

		DWORD			CreateHandshake();

		LPCLIENT_DESC		CreateConnectionDesc(LPFDWATCH fdw, const char * host, WORD port, int iPhaseWhenSucceed, bool bRetryWhenClosed);
		void			TryConnect();

		LPDESC			FindByHandle(DWORD handle);
		LPDESC			FindByHandshake(DWORD dwHandshake);

		void			RemoveClientDesc(LPCLIENT_DESC d);

		LPDESC			FindByCharacterName(const char* name);
		LPDESC			FindByLoginName(const std::string& login);
		void			ConnectAccount(const std::string& login, LPDESC d);
		void			DisconnectAccount(const std::string& login);

		void			DestroyClosed();

		void			UpdateLocalUserCount();
		DWORD			GetLocalUserCount() { return m_iLocalUserCount; }
		void			GetUserCount(int & iTotal, int ** paiEmpireUserCount, int & iLocalCount);

		const DESC_SET &	GetClientSet();

		DWORD			MakeRandomKey(DWORD dwHandle);
		bool			GetRandomKey(DWORD dwHandle, DWORD* prandom_key);

		DWORD			CreateLoginKey(LPDESC d);
		LPDESC			FindByLoginKey(DWORD dwKey);
		void			ProcessExpiredLoginKey();

		bool			IsDisconnectInvalidCRC() { return m_bDisconnectInvalidCRC; }
		void			SetDisconnectInvalidCRCMode(bool bMode) { m_bDisconnectInvalidCRC = bMode; }

		bool			IsP2PDescExist(const char * szHost, WORD wPort);

		// for C/S hybrid crypt
		bool			LoadClientPackageCryptInfo(const char* pDirName);
		void			SendClientPackageCryptKey( LPDESC desc );
		void			SendClientPackageSDBToLoadMap( LPDESC desc, const char* pMapName );
#ifdef __FreeBSD__
		static void		NotifyClientPackageFileChanged( const std::string& fileName, eFileUpdatedOptions eUpdateOption );
#endif

	private:
		bool				m_bDisconnectInvalidCRC;

		DESC_HANDLE_RANDOM_KEY_MAP	m_map_handle_random_key;

		CLIENT_DESC_SET		m_set_pkClientDesc;
		DESC_SET			m_set_pkDesc;

		DESC_HANDLE_MAP			m_map_handle;
		DESC_HANDSHAKE_MAP		m_map_handshake;
		//DESC_ACCOUNTID_MAP		m_AccountIDMap;
		DESC_LOGINNAME_MAP		m_map_loginName;
		std::map<DWORD, CLoginKey *>	m_map_pkLoginKey;

		int				m_iSocketsConnected;

		int				m_iHandleCount;

		int				m_iLocalUserCount;
		int				m_aiEmpireUserCount[EMPIRE_MAX_NUM];

		bool			m_bDestroyed;

		CClientPackageCryptInfo*	m_pPackageCrypt;
		
private:
	LPEVENT	m_pkDescManagerGarbageCollector;
	std::unordered_map<std::string, std::tuple<DWORD, int, bool>> m_connection_mapper;
	TR1_NS::unordered_set<std::string> s_handshake_whitelist;

	bool	GetHostHandshake(const struct sockaddr_in& c_rSockAddr);
	int	GetHostConnectionCount(const struct sockaddr_in& c_rSockAddr);
	void	RegisterInstrusiveConnection(const std::string& sHost);
	bool	IsIntrusiveConnection(const std::string& sHost);
	bool	IsOnHandshakeWhitelist(const struct sockaddr_in& c_rSockAddr);

public:
	void	AddToHandshakeWhiteList(const TPacketGGHandshakeValidate* pack);
	void	ConnectionCollector();
};

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
