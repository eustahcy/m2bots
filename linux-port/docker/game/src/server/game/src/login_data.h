#ifndef __INC_METIN_II_LOGIN_DATA_H__
#define __INC_METIN_II_LOGIN_DATA_H__

class CLoginData
{
	public:
		CLoginData();

		void            SetClientKey(const DWORD * c_pdwClientKey);

		const DWORD *   GetClientKey();
		void            SetKey(DWORD dwKey);
		DWORD           GetKey();

		void			SetLogin(const char * c_pszLogin);
		const char *	GetLogin();

		void			SetConnectedPeerHandle(DWORD dwHandle);
		DWORD			GetConnectedPeerHandle();

		void			SetLogonTime();
		DWORD			GetLogonTime();

		void			SetIP(const char * c_pszIP);
		const char *	GetIP();

		void		SetDeleted(bool bSet);
		bool		IsDeleted();

		void		SetPremium(int * paiPremiumTimes);
		int			GetPremium(BYTE type);
		int *		GetPremiumPtr();

		void		SetHWID(const char* c_pszHwid) { strlcpy(m_szHwid, c_pszHwid, sizeof(m_szHwid)); }
		const char* GetHWID() { return m_szHwid; }

	private:
		DWORD           m_dwKey;
		DWORD           m_adwClientKey[4];
		DWORD           m_dwConnectedPeerHandle;
		DWORD           m_dwLogonTime;
		char			m_szIP[MAX_HOST_LENGTH+1];
		char			m_szHwid[HWID_MAX_LEN+1];
		bool			m_bDeleted;
		std::string		m_stLogin;
		int				m_aiPremiumTimes[PREMIUM_MAX_NUM];
};

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
