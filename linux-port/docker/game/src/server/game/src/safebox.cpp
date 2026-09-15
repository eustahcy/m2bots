#include "stdafx.h"
#include "../../libgame/include/grid.h"
#include "constants.h"
#include "safebox.h"
#include "packet.h"
#include "char.h"
#include "desc_client.h"
#include "item.h"
#include "item_manager.h"
#include "config.h"

CSafebox::CSafebox(LPCHARACTER pkChrOwner, int iSize, YANG dwGold) : m_pkChrOwner(pkChrOwner), m_iSize(iSize), m_lGold(dwGold)
{
	assert(m_pkChrOwner != NULL);
	memset(m_pkItems, 0, sizeof(m_pkItems));

	if (m_iSize)
		m_pkGrid = std::make_unique<CGrid>(SAFEBOX_PAGE_WIDTH, m_iSize * SAFEBOX_PAGE_HEIGHT); // @fixme191

	m_bWindowMode = SAFEBOX;
}

CSafebox::~CSafebox()
{
	__Destroy();
}

void CSafebox::SetWindowMode(BYTE bMode)
{
	m_bWindowMode = bMode;
}

void CSafebox::__Destroy()
{
	for (int i = 0; i < SAFEBOX_MAX_NUM; ++i)
	{
		if (m_pkItems[i])
		{
			m_pkItems[i]->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(m_pkItems[i]);

			M2_DESTROY_ITEM(m_pkItems[i]->RemoveFromCharacter());
			m_pkItems[i] = NULL;
		}
	}

	if (m_pkGrid.get()) // @fixme191
		m_pkGrid.release();
}

bool CSafebox::Add(DWORD dwPos, LPITEM pkItem)
{
	if (!IsValidPosition(dwPos))
	{
		sys_err("SAFEBOX: item on wrong position at %d (size of grid = %d)", dwPos, m_pkGrid->GetSize());
		return false;
	}

	if (!m_pkChrOwner)
		return false;

	if (m_pkChrOwner->IsBusy(BUSY_SAFEBOX))
		return false;

	pkItem->SetWindow(m_bWindowMode);
	pkItem->SetCell(m_pkChrOwner, dwPos);
	pkItem->Save();
	ITEM_MANAGER::instance().FlushDelayedSave(pkItem);

	m_pkGrid->Put(dwPos, 1, pkItem->GetSize());
	m_pkItems[dwPos] = pkItem;

	TPacketGCItemSet pack;

	pack.header	= m_bWindowMode == SAFEBOX ? HEADER_GC_SAFEBOX_SET : HEADER_GC_MALL_SET;
	pack.Cell	= TItemPos(m_bWindowMode, dwPos);
	pack.vnum	= pkItem->GetVnum();
	pack.count	= pkItem->GetCount();
	pack.flags	= pkItem->GetFlag();
	pack.anti_flags	= pkItem->GetAntiFlag();
	thecore_memcpy(pack.alSockets, pkItem->GetSockets(), sizeof(pack.alSockets));
	thecore_memcpy(pack.aAttr, pkItem->GetAttributes(), sizeof(pack.aAttr));

	m_pkChrOwner->GetDesc()->Packet(&pack, sizeof(pack));
	sys_log(1, "SAFEBOX: ADD %s %s count %d", m_pkChrOwner->GetName(), pkItem->GetName(), pkItem->GetCount());
	return true;
}

LPITEM CSafebox::Get(DWORD dwPos)
{
	if (dwPos >= m_pkGrid->GetSize())
		return NULL;

	return m_pkItems[dwPos];
}

LPITEM CSafebox::Remove(DWORD dwPos)
{
	LPITEM pkItem = Get(dwPos);

	if (!pkItem)
		return NULL;

	if (!m_pkChrOwner)
		return NULL;

	if (m_pkChrOwner->IsBusy(BUSY_SAFEBOX))
		return NULL;

	if (!m_pkGrid)
		sys_err("Safebox::Remove : nil grid");
	else
		m_pkGrid->Get(dwPos, 1, pkItem->GetSize());

	pkItem->RemoveFromCharacter();

	m_pkItems[dwPos] = NULL;

	TPacketGCItemDel pack;

	pack.header	= m_bWindowMode == SAFEBOX ? HEADER_GC_SAFEBOX_DEL : HEADER_GC_MALL_DEL;
	pack.pos	= dwPos;

	m_pkChrOwner->GetDesc()->Packet(&pack, sizeof(pack));
	sys_log(1, "SAFEBOX: REMOVE %s %s count %d", m_pkChrOwner->GetName(), pkItem->GetName(), pkItem->GetCount());
	return pkItem;
}


void CSafebox::Refresh(DWORD dwPos, bool only_refresh)
{
	LPITEM pkItem = GetItem(dwPos);

	if (!pkItem)
		return;

	if (!m_pkChrOwner || !m_pkChrOwner->GetDesc())
		return;

	// Remove
	TPacketGCItemDel pack2;

	if (!only_refresh)
	{
		pack2.header = m_bWindowMode == SAFEBOX ? HEADER_GC_SAFEBOX_DEL : HEADER_GC_MALL_DEL;
		pack2.pos = dwPos;

		m_pkChrOwner->GetDesc()->Packet(&pack2, sizeof(pack2));
	}

	// Add
	TPacketGCItemSet pack;

	pack.header = m_bWindowMode == SAFEBOX ? HEADER_GC_SAFEBOX_SET : HEADER_GC_MALL_SET;
	pack.Cell = TItemPos(m_bWindowMode, dwPos);
	pack.vnum = pkItem->GetVnum();
	pack.count = pkItem->GetCount();
	pack.flags = pkItem->GetFlag();
	pack.anti_flags = pkItem->GetAntiFlag();
	thecore_memcpy(pack.alSockets, pkItem->GetSockets(), sizeof(pack.alSockets));
	thecore_memcpy(pack.aAttr, pkItem->GetAttributes(), sizeof(pack.aAttr));

	m_pkChrOwner->GetDesc()->Packet(&pack, sizeof(pack));
}

void CSafebox::Save()
{
	TSafeboxTable t;

	memset(&t, 0, sizeof(TSafeboxTable));

	t.dwID = m_pkChrOwner->GetDesc()->GetAccountTable().id;
	t.dwGold = m_lGold;

	db_clientdesc->DBPacket(HEADER_GD_SAFEBOX_SAVE, 0, &t, sizeof(TSafeboxTable));
	sys_log(1, "SAFEBOX: SAVE %s", m_pkChrOwner->GetName());
}

bool CSafebox::IsEmpty(DWORD dwPos, BYTE bSize)
{
	if (!m_pkGrid)
		return false;

	return m_pkGrid->IsEmpty(dwPos, 1, bSize);
}

void CSafebox::ChangeSize(int iSize)
{
	if (m_iSize >= iSize)
		return;

	m_iSize = iSize;

	std::unique_ptr<CGrid> pkOldGrid = std::move(m_pkGrid); // @fixme191

	if (m_pkGrid.get())
		m_pkGrid = std::make_unique<CGrid>(pkOldGrid.get(), SAFEBOX_PAGE_WIDTH, m_iSize * SAFEBOX_PAGE_HEIGHT);
	else
		m_pkGrid = std::make_unique<CGrid>(SAFEBOX_PAGE_WIDTH, m_iSize * SAFEBOX_PAGE_HEIGHT);
}

LPITEM CSafebox::GetItem(BYTE bCell)
{
	if (bCell >= SAFEBOX_PAGE_WIDTH * m_iSize * SAFEBOX_PAGE_HEIGHT)
	{
		sys_err("CHARACTER::GetItem: invalid item cell %d", bCell);
		return NULL;
	}

	return m_pkItems[bCell];
}

bool CSafebox::MoveItem(BYTE bCell, BYTE bDestCell)
{
	if (bCell == bDestCell) // @fixme196
		return false;

	if (bCell >= SAFEBOX_MAX_NUM || bDestCell >= SAFEBOX_MAX_NUM)
		return false;

	LPITEM item{};
	if (!(item = GetItem(bCell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (item->GetCount() < 1)
		return false;

	{
#ifndef ENABLE_MT2009_DISABLE_SAFEBOX_STACK
		LPITEM item2{};
		if ((item2 = GetItem(bDestCell)) && item != item2 && item2->IsStackable() &&
				!IS_SET(item2->GetAntiFlag(), ITEM_ANTIFLAG_STACK) &&
				item2->GetVnum() == item->GetVnum())
		{
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				if (item2->GetSocket(i) != item->GetSocket(i))
					return false;

			DWORD max_stack = item2->GetMaxStack();
			if (item2->GetCount() >= max_stack)
				return false;

			int count = MIN(max_stack - item2->GetCount(), item->GetCount());
			if (item->GetCount() <= count)
			{
				Remove(bCell);
				M2_DESTROY_ITEM(item);
			}
			else {
				item->SetCount(item->GetCount() - count);
				Refresh(bCell, true);
			}

			item2->SetCount(item2->GetCount() + count);
			Refresh(bDestCell, true);
			sys_log(1, "SAFEBOX: STACK %s %d -> %d %s count %d", m_pkChrOwner->GetName(), bCell, bDestCell, item2->GetName(), item2->GetCount());
			return true;
		}
#endif

		if (!IsEmpty(bDestCell, item->GetSize()))
			return false;

		m_pkGrid->Get(bCell, 1, item->GetSize());

		if (!m_pkGrid->Put(bDestCell, 1, item->GetSize()))
		{
			m_pkGrid->Put(bCell, 1, item->GetSize());
			return false;
		}
		else
		{
			m_pkGrid->Get(bDestCell, 1, item->GetSize());
			m_pkGrid->Put(bCell, 1, item->GetSize());
		}

		sys_log(1, "SAFEBOX: MOVE %s %d -> %d %s count %d", m_pkChrOwner->GetName(), bCell, bDestCell, item->GetName(), item->GetCount());

		Remove(bCell);
		Add(bDestCell, item);
	}

	return true;
}

bool CSafebox::IsValidPosition(DWORD dwPos)
{
	if (!m_pkGrid)
		return false;

	if (dwPos >= m_pkGrid->GetSize())
		return false;

	return true;
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
