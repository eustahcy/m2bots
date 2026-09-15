#ifndef __INC_MESSENGER_MANAGER_H
#define __INC_MESSENGER_MANAGER_H

#include "db.h"

class MessengerManager : public singleton<MessengerManager>
{
	public:
		typedef std::string keyT;
		typedef const std::string & keyA;

		MessengerManager();
		virtual ~MessengerManager();

	public:
		void	P2PLogin(keyA account);
		void	P2PLogout(keyA account);

		void	Login(keyA account);
		void	Logout(keyA account);

		void	RequestToAdd(LPCHARACTER ch, LPCHARACTER target);
		bool	AuthToAdd(keyA account, keyA companion, bool bDeny); // @fixme130 void -> bool

		void	__AddToList(keyA account, keyA companion);
		void	AddToList(keyA account, keyA companion);

		void	__RemoveFromList(keyA account, keyA companion);
		void	RemoveFromList(keyA account, keyA companion);

		void	RemoveAllList(keyA account);

		void	Initialize();

		bool	IsFriend(std::string account, std::string companion);
		DWORD	GetFriendCount(std::string account);

		// BLOCK MESSENGER
		void	LoadBlockList(LPCHARACTER who);
		void	__InsertBlockListP2P(DWORD pid, std::string name);

		void	Block(LPCHARACTER ch, std::string targetName);
		void	__BlockP2P(DWORD mePID, std::string targetName);

		void	Unblock(LPCHARACTER ch, std::string targetName);
		void	__UnblockP2P(DWORD mePID, std::string targetName);

		bool	IsBlock(DWORD whoPID, std::string targetName);
		DWORD	GetBlockCount(DWORD whoPID);

	private:
		void	SendBlockList(LPCHARACTER who);
		void	SendBlock(LPCHARACTER who, const char* name, bool isBlock);
		void	SendList(keyA account);
		void	SendLogin(keyA account, keyA companion);
		void	SendLogout(keyA account, keyA companion);

		void	LoadList(SQLMsg * pmsg);

		void	Destroy();

		std::set<keyT>			m_set_loginAccount;
		std::map<keyT, std::set<keyT> >	m_Relation;
		std::map<keyT, std::set<keyT> >	m_InverseRelation;
		std::set<DWORD>			m_set_requestToAdd;

		std::map<DWORD, std::set<std::string>>	m_BlockMap;
};

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
