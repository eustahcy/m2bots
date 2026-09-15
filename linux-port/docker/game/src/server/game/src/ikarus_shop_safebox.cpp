#include "stdafx.h"
#include "../../common/tables.h"
#include "packet.h"
#include "item.h"
#include "char.h"
#include "item_manager.h"
#include "desc.h"
#include "char_manager.h"

#include "ikarus_shop.h"
#include "ikarus_shop_manager.h"

#ifdef ENABLE_IKASHOP_RENEWAL


namespace ikashop
{
	CShopSafebox::CShopSafebox(LPCHARACTER chOwner)
	{
		m_owner = chOwner;
	}

	void CShopSafebox::SetOwner(LPCHARACTER ch)
	{
		m_owner = ch;
	}

	void CShopSafebox::SetValuteAmount(TValutesInfo val)
	{
		m_valutes = val;
	}

	bool CShopSafebox::AddItem(const TShopPlayerItem& item)
	{
		// making item info
		TShopItem info{item};
		info.price = {};

		// making shop item
		auto shopItem = std::make_shared<CShopItem>();
		shopItem->SetInfo(info);

		// registering shop item
		m_items[item.id] = shopItem;

		// registering to grid
		auto pos = _RegisterGridPosition(shopItem);
		shopItem->SetCell(pos);
		return true;
	}

	bool CShopSafebox::RemoveItem(DWORD itemid)
	{
		// searching item
		auto iter = m_items.find(itemid);
		if(iter == m_items.end())
			return false;

		// removing from grid
		_UnregisterGridPosition(iter->second);

		// removing from map
		m_items.erase(iter);

		return true;
	}

	void CShopSafebox::AddValute(TValutesInfo val)
	{
		m_valutes+=(val);
		//RefreshValuteToOwner();
	}

	bool CShopSafebox::RemoveValute(TValutesInfo val)
	{
		if(m_valutes.yang < val.yang)
			return false;

		m_valutes -= val;
		//RefreshValuteToOwner();
		return true;
	}

	const CShopSafebox::ITEM_MAP& CShopSafebox::GetItems() const
	{
		return m_items;
	}

	const DWORD CShopSafebox::GetLeftSpace() const
	{
		int left = OFFLINESHOP_MAX_SAFEBOX_SIZE;
		for (auto i : m_items)
		{
			left -= i.second->GetTable()->bSize;
		}
		return MAX(0, left);
	}

	const TValutesInfo& CShopSafebox::GetValutes() const
	{
		return m_valutes;
	}

	CShopSafebox::ITEM_HANDLE CShopSafebox::GetItem(DWORD itemid) const
	{
		auto it = m_items.find(itemid);
		return it != m_items.end() ? it->second : nullptr;
	}

	LPCHARACTER CShopSafebox::GetOwner() const
	{
		return m_owner;
	}


	//if nullptr auto find via charmanager
	bool CShopSafebox::RefreshToOwner(LPCHARACTER ch)
	{
		ch = ch ? ch : m_owner;
		if(!ch)
			return false;
		
		TValutesInfo valute;
		valute.yang = m_valutes.yang;

		// making items vector
		std::vector<ITEM_HANDLE> items;
		items.reserve(m_items.size());

		for (auto& [id, item] : m_items)
			items.emplace_back(item);
		GetManager().SendShopSafeboxRefreshClientPacket(ch , valute, items);
		return true;
	}

	int CShopSafebox::_RegisterGridPosition(ITEM_HANDLE item)
	{
		// getting item size
		auto table = item->GetTable();
		if(!table)
			return 0;

		// searching inside existing grids
		int pageIndex = 0;
		for(auto& grid : m_grids){
			// searching space
			if(auto pos = grid.FindSpace(table->bSize)){
				grid.RegisterItem(*pos, table->bSize);
				return pageIndex * grid.GetCellCount() + *pos;
			}
				
			// incrementing page index
			pageIndex++;
		}

		// adding a new page
		auto& grid = m_grids.emplace_back();
		const auto pos = *grid.FindSpace(table->bSize);
		grid.RegisterItem(pos, table->bSize);
		return pos + pageIndex * grid.GetCellCount();
	}

	void CShopSafebox::_UnregisterGridPosition(ITEM_HANDLE item)
	{
		// getting item size
		auto table = item->GetTable();
		if(!table)
			return;

		const auto page = item->GetInfo().pos / (GRID_WIDTH * GRID_HEIGHT);
		const auto pos = item->GetInfo().pos % (GRID_WIDTH * GRID_HEIGHT);

		if(static_cast<size_t>(page) < m_grids.size())
			m_grids[page].UnregisterItem(pos, table->bSize);
	}
}



#endif





// Files shared by GameCore.top
