#ifndef __INC_METIN_II_GAME_SAFEBOX_H__
#define __INC_METIN_II_GAME_SAFEBOX_H__
#include <memory>

class CHARACTER;
class CItem;
class CGrid;

class CSafebox
{
	public:
		CSafebox(LPCHARACTER pkChrOwner, int iSize, YANG dwGold);
		~CSafebox();

		bool		Add(DWORD dwPos, LPITEM pkItem);
		LPITEM		Get(DWORD dwPos);
		LPITEM		Remove(DWORD dwPos);
		void		Refresh(DWORD dwPos, bool only_refresh);
		void		ChangeSize(int iSize);

		bool		MoveItem(BYTE bCell, BYTE bDestCell);
		LPITEM		GetItem(BYTE bCell);

		void		Save();

		bool		IsEmpty(DWORD dwPos, BYTE bSize);
		bool		IsValidPosition(DWORD dwPos);

		void		SetWindowMode(BYTE bWindowMode);

	protected:
		void		__Destroy();

		LPCHARACTER	m_pkChrOwner;
		LPITEM		m_pkItems[SAFEBOX_MAX_NUM];
		std::unique_ptr<CGrid>		m_pkGrid; // @fixme191
		int		m_iSize;
		YANG		m_lGold;

		BYTE		m_bWindowMode;
};

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
