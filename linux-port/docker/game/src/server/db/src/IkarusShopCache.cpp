#include "stdafx.h"

#ifdef ENABLE_IKASHOP_RENEWAL
#include "Main.h"
#include "Config.h"
#include "DBManager.h"
#include "QID.h"
#include "Peer.h"
#include "ClientManager.h"
#include "IkarusShopCache.h"


namespace ikashop
{
	template<class T>
	bool IsItemExpired(const auto now, const T& item){
		switch (static_cast<ikashop::ExpirationType>(item.expiration)) {
		case ikashop::ExpirationType::EXPIRE_REAL_TIME_FIRST_USE:
			// checking activation only for real time first use
			if (item.alSockets[1] == 0)
				return false;
		case ikashop::ExpirationType::EXPIRE_REAL_TIME:
			// checking timestamp on socket 0 for both timers
			if (now < item.alSockets[0])
				return false;
			// inserting into vector
			return true;
		default:
			break;
		}
		return false;
	}

	//SHOPS
	std::string CreateShopCacheUpdateItemQuery(const TShopItem& rItemPrice);
	std::string CreateShopCacheDeleteShopQuery(DWORD dwOwner);
	std::string CreateShopCacheDeleteShopItemQuery(DWORD dwOwner);
	std::string CreateShopCacheInsertShopQuery(DWORD dwOwnerID, DWORD dwDuration, const char* name
#ifdef ENABLE_IKASHOP_ENTITIES
		, const TShopSpawn& spawn
#endif
		, BYTE unlockCount, bool isPremium
	);
	std::string CreateShopCacheUpdateDurationQuery(DWORD dwOwnerID, DWORD dwDuration);
	std::string CreateShopCacheDeleteItemQuery(DWORD dwOwnerID, DWORD dwItemID);

	//SAFEBOX
	std::string CreateSafeboxCacheDeleteItemQuery(DWORD dwItem);
	std::string CreateSafeboxCacheInsertItemQuery(DWORD dwOwner, DWORD dwItemID, const TShopPlayerItem& item);
	std::string CreateSafeboxCacheUpdateValutes(DWORD dwOwner, const TValutesInfo& val);
	std::string CreateSafeboxCacheInsertSafeboxValutesQuery(DWORD dwOwnerID);
	std::string CreateSafeboxCacheUpdateValutesByAdding(DWORD dwOwner, const TValutesInfo& val);
	std::string CreateSafeboxCacheLoadItemsQuery(DWORD dwOwnerID);
	std::string CreateSafeboxCacheLoadValutesQuery(DWORD dwOwnerID);

	//OFFERS
	std::string CreateOfferCacheInsertOfferQuery(const TOfferInfo& rOffer);
	std::string CreateOfferCacheUpdateNotifiedQuery(DWORD dwOfferID);
	std::string CreateOfferCacheUpdateAcceptedQuery(DWORD dwOfferID);
	std::string CreateOfferCacheRemoveOfferByShopOwner(DWORD dwShopOwner);
	std::string CreateOfferCacheDeleteOfferQuery(DWORD dwOfferID);

	//AUCTION

	std::string CreateAuctionCacheAddAuctionQuery(const TAuctionInfo& auction);
	std::string CreateAuctionCacheAddOfferQuery(const TAuctionOfferInfo& auctionOffer);
	//std::string CreateAuctionCacheDeleteAuction(DWORD dwOwnerID);
	std::string CreateAuctionCacheDeleteAuctionOffers(DWORD dwOwnerID);
	std::string CreateAuctionCacheUpdateDurationQuery(const TAuctionInfo& info);

	/*
			CSHOPCACHE
	*/

	CShopCache::CShopCache()
	{
	}

	CShopCache::~CShopCache()
	{
	}

	CShopCache::SHOP_HANDLE CShopCache::Get(DWORD dwOwnerID) const
	{
		// searching shop cache
		auto it = m_shopsMap.find(dwOwnerID);
		if (it == m_shopsMap.end())
			return nullptr; // not found
		return it->second;
	}

	bool CShopCache::AddItem(const TShopCacheItemInfo& item, const int queryType)
	{
		// searching shop cache
		auto cache = Get(item.owner);
		if(!cache)
			return false;

		// refreshing table datas
		std::string query = CreateShopCacheUpdateItemQuery(item);
		CDBManager::instance().ReturnQuery(query.c_str(), queryType, 0, new SQueryInfoAddItem(item));
		return true;
	}

	bool CShopCache::RemoveItem(DWORD dwOwnerID, DWORD dwItemID)
	{
		// searching for shop cache
		auto cache = Get(dwOwnerID);
		if(!cache)
			return false;

		// searching for item cache
		auto& itemsmap = cache->itemsmap;
		auto it = itemsmap.find(dwItemID);
		if (it == itemsmap.end())
			return false;

		// skipping locked items
		if(it->second->lock)
			return false;

		// removing item from cache
		itemsmap.erase(it);
		// removing item from db
		std::string query = CreateShopCacheDeleteItemQuery(dwOwnerID, dwItemID);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_REMOVE_ITEM, 0, NULL);

		return true;
	}

	void CShopCache::DeleteItem(DWORD owner, DWORD itemid)
	{
		// removing from table
		auto query = fmt::format("DELETE FROM `item` WHERE `id`={}", itemid);
		CDBManager::instance().AsyncQuery(query.c_str());

		// removing from cache
		if (auto shop = Get(owner))
			shop->itemsmap.erase(itemid);
	}

	bool CShopCache::SellItem(DWORD dwOwnerID, DWORD dwItemID, bool updateQuery)
	{
		// searching for shop cache
		auto cache = Get(dwOwnerID);
		if(!cache)
			return false;

		// searching for item cache
		auto& itemsmap = cache->itemsmap;
		auto it = itemsmap.find(dwItemID);
		if (it == itemsmap.end())
			return false; // not found

		// skipping unlocked items
		if(!it->second->lock)
			return false;

		// removing item from cache
		itemsmap.erase(it);
		// updating ikashop_data
		if (updateQuery)
		{
			std::string query = CreateShopCacheDeleteItemQuery(dwOwnerID, dwItemID);
			CDBManager::instance().DirectQuery(query.c_str());
		}

		return true;
	}

	bool CShopCache::LockSellItem(DWORD dwOwnerID, DWORD dwItemID, long long TotalPriceSeen) //patch seen price check
	{
		// searching shop cache
		auto cache = Get(dwOwnerID);
		if(!cache)
			return false;

		// searching item cache
		auto& itemsmap = cache->itemsmap;
		auto it = itemsmap.find(dwItemID);
		if (it == itemsmap.end())
			return false; // not found

		// skipping locked items
		if (it->second->lock)
			return false;

		// skipping items with a changed price
		if (it->second->price.GetTotalYangAmount() != TotalPriceSeen)
			return false;

		// locking item
		it->second->lock = true;
		return true;
	}

	bool CShopCache::UnlockSellItem(DWORD dwOwnerID, DWORD dwItemID)//topatch
	{
		// searching shop cache
		auto cache = Get(dwOwnerID);
		if(!cache)
			return false;

		// searching item cache
		auto& itemsmap = cache->itemsmap;
		auto it = itemsmap.find(dwItemID);
		if (it == itemsmap.end())
			return false; // not found

		// skipping unlocked items
		if (!it->second->lock)
			return false;

		// unlocking item
		it->second->lock = false;
		return true;
	}

	bool CShopCache::EditItem(DWORD dwOwnerID, DWORD dwItemID, const TPriceInfo& price)
	{
		// searching shop cache
		auto cache = Get(dwOwnerID);
		if(!cache)
			return false;

		// searching item cache
		auto& itemsmap = cache->itemsmap;
		auto it = itemsmap.find(dwItemID);
		if (it == itemsmap.end())
			return false;

		YANG total_val = 0;
		for (auto item : itemsmap)
			total_val += item.second->price.GetTotalYangAmount();

		if (total_val - it->second->price.GetTotalYangAmount() + price.GetTotalYangAmount() > GOLD_MAX)
			return false;

		// skipping locked items
		if(it->second->lock)
			return false;

		// updating price on cache
		it->second->price = price;
		// updating price on database
		std::string query = CreateShopCacheUpdateItemQuery(*it->second);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_EDIT_ITEM, 0, NULL);
		return true;
	}

	bool CShopCache::CloseShop(DWORD dwOwnerID)
	{
		// searching shop cache
		auto it = m_shopsMap.find(dwOwnerID);
		if (it == m_shopsMap.end())
			return false;

		// removing shop cache
		m_shopsMap.erase(it);
		// removing shop from db
		const auto query = CreateShopCacheDeleteShopQuery(dwOwnerID);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_DELETE_SHOP, 0, NULL);
		return true;
	}

	bool CShopCache::CreateShop(DWORD dwOwnerID, DWORD dwDuration, const char* szShopName, const char* szOwnerName
#ifdef ENABLE_IKASHOP_ENTITIES
		, const TShopSpawn& spawn, const TShopItem * items
#endif
		, BYTE unlockCount, bool isPremium
	)
	{
		// searching if shop already exists
		auto cache = Get(dwOwnerID);
		if (cache && cache->duration != 0)
		{
			return false; // it already exists
		}
		
		// making query create shop info
		auto* qi = new SQueryInfoCreateShop;
		qi->owner = dwOwnerID;
		qi->duration = dwDuration;
		qi->name = szShopName;
		qi->ownerName = szOwnerName;
		qi->unlockCount = unlockCount;
		qi->isPremium = isPremium;
#ifdef ENABLE_IKASHOP_ENTITIES
		qi->spawn = spawn;
		std::memcpy(qi->items, items, sizeof(TShopItem) * SHOP_PLAYER_HOST_ITEM_MAX_NUM);
#endif

		// sending query
		std::string query = CreateShopCacheInsertShopQuery(dwOwnerID, dwDuration, szShopName
#ifdef ENABLE_IKASHOP_ENTITIES
			, spawn
#endif
			, unlockCount, isPremium
		);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_CREATE_SHOP, 0, qi);

		
		return true;
	}

	bool CShopCache::ChangeShopName(DWORD dwOwnerID, const char* szName)
	{
		// searching shop cache
		auto cache = Get(dwOwnerID);
		if(!cache)
			return false;

		// updating cached name
		cache->name = szName;

		return true;
	}

	bool CShopCache::PutItem(DWORD dwOwnerID, DWORD dwItemID, const TShopCacheItemInfo& item)
	{
		// searching shop cache
		auto cache = Get(dwOwnerID);
		if(!cache)
			return false;

		// checking item already exists
		auto& itemsmap = cache->itemsmap;
		if(itemsmap.contains(dwItemID))
			return false; // already exists

		// inserting the item
		itemsmap[dwItemID] = std::make_shared<TShopCacheItemInfo>(item);
		return true;
	}

	bool CShopCache::PutShop(DWORD dwOwnerID, DWORD dwDuration, const char* szShopName, const char* szOwnerName
#ifdef ENABLE_IKASHOP_ENTITIES
		, const TShopSpawn& spawn
#endif
		, BYTE unlockCount, bool isPremium
	)
	{
		CShopCache::SHOP_HANDLE shop;
		if (m_shopsMap.contains(dwOwnerID))
			shop = m_shopsMap[dwOwnerID];
		else
			shop = m_shopsMap[dwOwnerID] = std::make_shared<TShopCacheInfo>();

		shop->duration = dwDuration;
		shop->name = szShopName;
		shop->ownerName = szOwnerName;
#ifdef ENABLE_IKASHOP_ENTITIES
		shop->spawn = spawn;
#endif
		shop->unlockCount = unlockCount;
		shop->isPremium = isPremium;
		return true;
	}

	void CShopCache::EncodeCache(CPeer* peer) const
	{		
		for(auto& [owner, shop] : m_shopsMap)
		{
			static TShopInfo pack{};
			pack.duration = shop->duration;
			pack.ownerid = owner;
			pack.count = shop->itemsmap.size();
			str_to_cstring(pack.name, shop->name.c_str());
			str_to_cstring(pack.ownerName, shop->ownerName.c_str());
#ifdef ENABLE_IKASHOP_ENTITIES
			pack.spawn = shop->spawn;
#endif
			pack.unlockCount = shop->unlockCount;
			pack.isPremium = shop->isPremium;
			peer->Encode(&pack, sizeof(pack));
			for (auto& [id, item] : shop->itemsmap)
				peer->Encode(static_cast<TShopItem*>(item.get()), sizeof(TShopItem));
		}
	}


	DWORD CShopCache::GetItemCount() const
	{
		return std::accumulate(m_shopsMap.begin(), m_shopsMap.end(), 0UL, 
			[](const auto& val, const auto& iter){ return val + iter.second->itemsmap.size(); });
	}

	void CShopCache::ShopDurationProcess()
	{
		//expired check
		std::set<DWORD> expiredShops;

		for (auto& [owner, shop] : m_shopsMap)
		{
			// checking for expiring shop
			if(shop->duration == 1)
				expiredShops.emplace(owner);

			if (shop->duration != 0 && --shop->duration != 0 && shop->duration % 5 == 0)
				UpdateDurationQuery(owner, *shop);
		}

		//item expired check
		std::vector<std::pair<DWORD, DWORD>> item_vec;
		const time_t now = std::time(nullptr);

		for (auto& [owner, shop] : m_shopsMap)
		{
			// skipping expired shops
			if(expiredShops.contains(owner))
				continue;

			// searching for expired items
			for(auto& [id, item] : shop->itemsmap)
				if(IsItemExpired(now, *item))
					item_vec.emplace_back(std::make_pair(owner, id));
		}

		// removing expired shops
		for (auto owner : expiredShops){
#ifdef ENABLE_IKASHOP_LOGS
			CClientManager::instance().IkarusShopLog(owner, 0, "SHOP_DURATION_EXPIRED", owner);
#endif
			CClientManager::instance().IkarusShopExpiredShop(owner);
		}

		// removing expired items
		for(auto& [owner,id] : item_vec){
#ifdef ENABLE_IKASHOP_LOGS
			auto item = GetItem(owner, id);
			CClientManager::instance().IkarusShopLog(owner, id, "SHOP_ITEM_EXPIRED", owner, "", item->vnum, item->count);
#endif
			CClientManager::Instance().SendIkarusShopRemoveItemPacket(owner, id);
			CClientManager::Instance().RemoveOfferOnShopItem(owner, id);
			DeleteItem(owner, id);
		}
	}

	void CShopCache::UpdateDurationQuery(DWORD dwOwnerID, const TShopCacheInfo& rShop)
	{
		std::string query = CreateShopCacheUpdateDurationQuery(dwOwnerID, rShop.duration);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_SHOP_UPDATE_DURATION, 0 , NULL);
	}

	CShopCache::ITEM_CACHE_HANDLE CShopCache::GetItem(DWORD owner, DWORD item)
	{
		if(auto shop = Get(owner))
			if(auto iter = shop->itemsmap.find(item); iter != shop->itemsmap.end())
				return iter->second;
		return nullptr;
	}

#ifdef ENABLE_IKASHOP_ENTITIES
#ifdef EXTEND_IKASHOP_PRO
	bool CShopCache::MoveShopEntity(DWORD owner, const TShopSpawn& spawn)
	{
		auto shop = Get(owner);
		if(!shop)
			return false;

		// update table
		auto query = fmt::format("UPDATE `ikashop_offlineshop` SET `map` = {}, `x` = {}, `y` = {} WHERE `owner` = {}", spawn.map, spawn.x, spawn.y, owner);
		CDBManager::instance().AsyncQuery(query.c_str());

		// update cache
		shop->spawn = spawn;

		return true;
	}
#endif
#endif

	bool CShopCache::SetLockedSlotState(DWORD owner, BYTE unlockCount, bool isPremium)
	{
		auto shop = Get(owner);
		if (!shop)
			return false;

		shop->unlockCount = unlockCount;
		shop->isPremium = isPremium;
		return true;
	}


	//SAFEBOX CHACHE
	CSafeboxCache::CSafeboxCache()
	{
	}

	CSafeboxCache::~CSafeboxCache()
	{
	}

	CSafeboxCache::SAFEBOX_HANDLE CSafeboxCache::Get(DWORD dwOwnerID) const
	{
		auto it = m_safeboxMap.find(dwOwnerID);
		if(it == m_safeboxMap.end())
			return nullptr;
		return it->second;
	}

	bool CSafeboxCache::PutSafebox(DWORD dwOwnerID, const TSafeboxCacheInfo& safebox)
	{
		// searching for duplicates
		if(m_safeboxMap.contains(dwOwnerID))
			return false;
		// inserting into map
		m_safeboxMap[dwOwnerID] = std::make_shared<TSafeboxCacheInfo>(safebox);
		return true;
	}

	bool CSafeboxCache::PutItem(DWORD dwOwnerID, DWORD dwItem, const TShopPlayerItem& item)
	{
		// searching safebox cache
		auto cache = Get(dwOwnerID);
		if(!cache)
			return false;

		// searching safebox item cache
		auto& itemsmap = cache->itemsmap;
		if(itemsmap.contains(dwItem))
			return false;

		// inserting into map
		itemsmap[dwItem] = item;
		return true;
	}

	bool CSafeboxCache::DeleteItem(DWORD dwOwnerID, DWORD dwItemID)
	{
		// searching safebox cache
		auto cache = Get(dwOwnerID);
		if(!cache)
			return false;

		// searching safebox item cache
		auto& itemsmap = cache->itemsmap;
		auto it = itemsmap.find(dwItemID);
		if (it == itemsmap.end())
			return false;

		// removing item from cache
		itemsmap.erase(it);

		// removing item from table
		std::string query = CreateSafeboxCacheDeleteItemQuery(dwItemID);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_SAFEBOX_DELETE_ITEM, 0, NULL);
		return true;
	}

	bool CSafeboxCache::LockItem(DWORD dwOwner, DWORD dwItemID)
	{
		// searching safebox cache
		auto cache = Get(dwOwner);
		if (!cache)
			return false;

		// searching safebox item cache
		auto& itemsmap = cache->itemsmap;
		auto it = itemsmap.find(dwItemID);
		if (it == itemsmap.end())
			return false;

		// adding item to locked map
		auto& lockmap = cache->lockedmap;
		lockmap[dwItemID] = it->second;

		// removing item from cache
		itemsmap.erase(it);
		return true;
	}

	bool CSafeboxCache::RemoveLockedItem(DWORD dwOwner, DWORD dwItemID)
	{
		// searching safebox cache
		auto cache = Get(dwOwner);
		if (!cache)
			return false;

		// searching into lockmap
		auto& lockmap = cache->lockedmap;
		auto lockiter = lockmap.find(dwItemID);
		if(lockiter == lockmap.end())
			return false;

		// removing from map
		lockmap.erase(lockiter);

		// blanking ikashop_data on db
		auto query = fmt::format("UPDATE `item` SET `ikashop_data` = '' WHERE `id` = {}", dwItemID);
		CDBManager::instance().DirectQuery(query.c_str());

		return true;
	}

	bool CSafeboxCache::UnlockItem(DWORD dwOwner, DWORD dwItemID)
	{
		// searching safebox cache
		auto cache = Get(dwOwner);
		if (!cache)
			return false;

		// searching locked item
		auto& lockmap = cache->lockedmap;
		auto lockiter = lockmap.find(dwItemID);
		if(lockiter == lockmap.end())
			return false;

		// adding item to itemsmap
		auto& itemsmap = cache->itemsmap;
		itemsmap[dwItemID] = lockiter->second;

		// removing from locked items
		lockmap.erase(lockiter);
		return true;
	}

	bool CSafeboxCache::AddItem(DWORD dwOwnerID, DWORD dwItemID, const TShopPlayerItem& item)
	{
		// saving item into query info
		auto queryItem = new SQueryInfoSafeboxAddItem(item);
		queryItem->owner = dwOwnerID; // updating owner

		// pushing query
		std::string query = CreateSafeboxCacheInsertItemQuery(dwOwnerID, dwItemID, item);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_SAFEBOX_ADD_ITEM, 0, queryItem);
		query = fmt::format("UPDATE `item` SET `window` = 'IKASHOP_SAFEBOX', `owner_id` = {}  WHERE `id` = {} ", dwOwnerID, dwItemID);
		CDBManager::Instance().DirectQuery(query.c_str());
		return true;
	}

	bool CSafeboxCache::AddValutes(DWORD dwOwnerID, const TValutesInfo& val)
	{
		// searching safebox cache
		auto cache = Get(dwOwnerID);
		if(!cache)
		{
			// adding as query -> it wasn't loaded yet on cache
			AddValutesAsQuery(dwOwnerID, val);
			return true;
		}

		// updating values on cache
		cache->valutes += val;
		// updating values into db
		std::string query = CreateSafeboxCacheUpdateValutes(dwOwnerID, cache->valutes);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_SAFEBOX_UPDATE_VALUTES, 0, NULL);
		return true;
	}

	void CSafeboxCache::AddValutesAsQuery(DWORD dwOwnerID, const TValutesInfo& val)
	{
		std::string query=CreateSafeboxCacheUpdateValutesByAdding(dwOwnerID,val);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_SAFEBOX_UPDATE_VALUTES_ADDING, 0, NULL);
	}

	bool CSafeboxCache::RemoveValutes(DWORD dwOwnerID, const TValutesInfo& val)
	{
		// searching cache
		auto cache = Get(dwOwnerID);
		if(!cache)
			return false;

		// updating values into cache
		cache->valutes -= val;
		// updating values into db
		std::string query = CreateSafeboxCacheUpdateValutes(dwOwnerID, cache->valutes);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_SAFEBOX_UPDATE_VALUTES, 0, NULL);
		return true;
	}

	CSafeboxCache::SAFEBOX_HANDLE CSafeboxCache::CreateSafebox(DWORD dwOwnerID)
	{
		// inserting into db an empty safebox
		std::string query = CreateSafeboxCacheInsertSafeboxValutesQuery(dwOwnerID);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_SAFEBOX_INSERT_VALUTES, 0, NULL);

		// inserting into cache
		return (m_safeboxMap[dwOwnerID] = std::make_shared<TSafeboxCacheInfo>());
	}

	CSafeboxCache::SAFEBOX_HANDLE CSafeboxCache::LoadSafebox(DWORD dwPID)
	{
		// searching inside cache
		auto cache = Get(dwPID);
		if(cache)
			return cache;

		// making query
		const auto query = CreateSafeboxCacheLoadValutesQuery(dwPID);
		auto msg = CDBManager::instance().DirectQuery(query.c_str());

		// checking for errors
		if(msg->uiSQLErrno != 0)
			return nullptr;

		// no row so creating it from 0
		if (msg->Get()->uiAffectedRows == 0)
			return CreateSafebox(dwPID);

		// inserting new empty cache
		auto& safebox = m_safeboxMap[dwPID] = std::make_shared<TSafeboxCacheInfo>();

		// extracting data
		auto row = mysql_fetch_row(msg->Get()->pSQLResult);
		str_to_number(safebox->valutes.yang, row[0]);

		// making query selecting items
		const auto itemquery = CreateSafeboxCacheLoadItemsQuery(dwPID);
		msg = CDBManager::instance().DirectQuery(itemquery.c_str());

		// extracting items
		auto& itemsmap = safebox->itemsmap;
		auto item = TShopPlayerItem{};
		while ((row = mysql_fetch_row(msg->Get()->pSQLResult)))
		{
			int col = 0;
			str_to_number(item.id, row[col++]);
			str_to_number(item.vnum, row[col++]);
			str_to_number(item.count, row[col++]);

			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
				str_to_number(item.alSockets[i], row[col++]);

			for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
			{
				str_to_number(item.aAttr[i].bType, row[col++]);
				str_to_number(item.aAttr[i].sValue, row[col++]);
			}

#ifdef ENABLE_CHANGELOOK_SYSTEM
			str_to_number(item.dwTransmutation, row[col++]);
#endif
			auto result = str_to_json(row[col++]);
			if(!result){
				sys_err("CANNOT EXTRACT JSON FROM SHOP SAFEBOX ITEM : pid(%u) item_id(%u)", dwPID, item.id);
				continue;
			}

			auto expirationResult = jsonHelper::getValue<int>(result.value(), "expiration");
			if(!expirationResult){
				sys_err("CANNOT EXTRACT JSON EXPIRATION FROM SHOP SAFEBOX ITEM : pid(%u) item_id(%u)", dwPID, item.id);
				continue;
			}

			item.owner = dwPID;
			item.expiration = static_cast<ikashop::ExpirationType>(expirationResult.value());
			itemsmap[item.id] = item;
		}

		return safebox;
	}

	DWORD CSafeboxCache::GetItemCount() const
	{
		return std::accumulate(m_safeboxMap.begin(), m_safeboxMap.end(), 0UL,
			[](const auto& value, const auto& iter){ return value + iter.second->itemsmap.size(); });
	}

	//patch 08-03-2020
	void CSafeboxCache::ItemExpirationProcess() {
		// making temp vector
		std::vector<std::pair<DWORD, DWORD>> vec;
		const auto now = std::time(nullptr);

		// iterating checking item expiration
		for (auto& [owner, safebox] : m_safeboxMap){
			for (auto& [id,item] : safebox->itemsmap) {
				if(IsItemExpired(now, item)){
#ifdef ENABLE_IKASHOP_LOGS
					CClientManager::instance().IkarusShopLog(owner, id, "SAFEBOX_ITEM_EXPIRED", owner, "", item.vnum, item.count);
#endif
					vec.emplace_back(std::make_pair(owner, id));
				}
			}
		}

		// removing expired items
		for (const auto& [owner, id] : vec) 
		{
			DeleteItem(owner, id);
			CClientManager::instance().SendIkarusShopSafeboxExpiredItem(owner, id);
		}
	}

	//OFFER CACHE
	COfferCache::COfferCache()
	{
	}

	COfferCache::~COfferCache()
	{
	}

	bool COfferCache::Puts(const TOfferInfo& offer)
	{
		// checking for duplicates
		if(m_mapOffersByBuyer.contains(offer.offerid))
			return false;

		// inserting into offers cache
		auto& handle = m_mapOffersByBuyer[offer.offerid] = std::make_shared<TOfferInfo>(offer);

		// inserting into offers by shop
		auto& vec = m_findOffersByShop[offer.ownerid];
		vec.emplace_back(handle);

		return true;
	}

	COfferCache::OFFER_HANDLE COfferCache::Get(DWORD dwOfferID) const
	{
		auto it = m_mapOffersByBuyer.find(dwOfferID);
		if(it == m_mapOffersByBuyer.end())
			return nullptr;
		return it->second;
	}

	bool COfferCache::AddOffer(const TOfferInfo& offer)
	{
		const auto query = CreateOfferCacheInsertOfferQuery(offer);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_OFFER_ADD, 0, new SQueryInfoOfferAdd(offer));
		return true;
	}

	bool COfferCache::NotifiedOffer(DWORD dwOfferID)
	{
		// searching offer cache
		auto cache = Get(dwOfferID);
		if(!cache)
			return false;

		// updating value into cache
		cache->notified = true;
		// updating value into db
		std::string query = CreateOfferCacheUpdateNotifiedQuery(dwOfferID);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_OFFER_UPDATE_NOTIFIED, 0, NULL);
		return true;
	}

	bool COfferCache::CancelOffer(DWORD dwOfferID, DWORD dwOwnerID) //ikashop-updated 04/08/19 reminder
	{
		// searching offer cache
		auto cache = Get(dwOfferID);
		if(!cache)
			return false;

		// searching offer into shop offers vector
		auto it = m_findOffersByShop.find(dwOwnerID);
		// removing from vector
		if(it != m_findOffersByShop.end())
			std::erase(it->second, cache);

		// checking if vector is empty
		if(it->second.empty())
			m_findOffersByShop.erase(it);

		// removing offer from map
		m_mapOffersByBuyer.erase(dwOfferID);

		// removing offer from db
		std::string query = CreateOfferCacheDeleteOfferQuery(dwOfferID);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_OFFER_DELETE, 0, NULL);
		return true;
	}

	bool COfferCache::AcceptOffer(DWORD dwOfferID)
	{
		// searching offer cache
		auto cache = Get(dwOfferID);
		if (!cache)
			return false;

		return CancelOffer(dwOfferID, cache->ownerid);
	}

	bool COfferCache::RemoveOffersByShopOwner(DWORD dwShopOwnerID)
	{
		// searching offer vector
		auto it = m_findOffersByShop.find(dwShopOwnerID);
		if(it == m_findOffersByShop.end())
			return false;

		// removing all offers from cache
		for(auto& handle : it->second)
			m_mapOffersByBuyer.erase(handle->offerid);

		// removing vector from cache
		m_findOffersByShop.erase(it);

		// removing all offers from db
		std::string query = CreateOfferCacheRemoveOfferByShopOwner(dwShopOwnerID);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_OFFER_DELETE_BY_SHOP, 0, NULL);
		return true;
	}

	void COfferCache::EncodeCache(CPeer* peer) const
	{
		for(auto& [id, offer] : m_mapOffersByBuyer)
			peer->Encode(offer.get(), sizeof(*offer));
	}

	std::vector<COfferCache::OFFER_HANDLE> COfferCache::GetOffersByShopOwner(DWORD dwOwnerID) const
	{
		auto it = m_findOffersByShop.find(dwOwnerID);
		if(it == m_findOffersByShop.end())
			return {};
		return it->second;
	}

	std::vector<COfferCache::OFFER_HANDLE> COfferCache::GetOffersByItemID(DWORD dwOwnerID, DWORD dwItemID) const
	{
		// searching offers vector
		auto it = m_findOffersByShop.find(dwOwnerID);
		if (it == m_findOffersByShop.end())
			return {};

		// making filter lambda
		auto filterFunc = [&](auto&& handle) { return handle->itemid == dwItemID; };
		
		// making filtered vector
		std::vector<OFFER_HANDLE> ret;
		std::copy_if(it->second.begin(), it->second.end(), std::back_inserter(ret), filterFunc);
		return ret;
	}

	//AUCTION
	CAuctionCache::CAuctionCache()
	{
	}

	CAuctionCache::~CAuctionCache()
	{
	}

	bool CAuctionCache::PutsAuction(const TAuctionInfo& auction)
	{
		auto handle = std::make_shared<TAuctionInfo>(auction);
		m_mapAuction[auction.owner]	= handle;
		m_mapAuctionOffer[auction.id] = {};
		return true;
	}

	bool CAuctionCache::PutsAuctionOffer(const TAuctionOfferInfo& auctionOffer)
	{
		auto& vec = m_mapAuctionOffer[auctionOffer.ownerid];
		vec.emplace_back(std::make_shared<TAuctionOfferInfo>(auctionOffer));
		return true;
	}

	bool CAuctionCache::AddAuction(const TAuctionInfo& auction)
	{
		

		std::string query = CreateAuctionCacheAddAuctionQuery(auction);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_AUCTION_INSERT, 0, NULL);

		

		PutsAuction(auction);
		return true;
	}

	bool CAuctionCache::AddOffer(const TAuctionOfferInfo& auctionOffer, bool quering)
	{
		if (quering)
		{
			std::string query = CreateAuctionCacheAddOfferQuery(auctionOffer);
			CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_AUCTION_INSERT_OFFER, 0 , NULL);

			//adding 1 minute time when new offer is done in the last minute +
			if(auto cache = Get(auctionOffer.ownerid))
				if(cache->duration == 0)
					cache->duration = 1;
		}


		PutsAuctionOffer(auctionOffer);
		return true;
	}

	bool CAuctionCache::ExpiredAuction(DWORD dwOwnerID)
	{
		// removing from cache
		m_mapAuction.erase(dwOwnerID);
		m_mapAuctionOffer.erase(dwOwnerID);

		// - disabling since ikashop_data will be update by adding it on shop safebox
		//CDBManager::instance().ReturnQuery(CreateAuctionCacheDeleteAuction(dwOwnerID).c_str(), QID_OFFLINESHOP_AUCTION_DELETE, 0, NULL);

		// removing from db
		const auto query = CreateAuctionCacheDeleteAuctionOffers(dwOwnerID);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_AUCTION_DELETE_OFFERS, 0 , NULL);
		return true;
	}

	//patch 08-03-2020
	bool CAuctionCache::AuctionDurationProcess()
	{
		if(m_mapAuction.empty())
			return false;

		std::vector<DWORD> expired_auctions;
		std::vector<DWORD> expired_items;

		const time_t now = time(0);

		for(auto&[owner, auction] : m_mapAuction)
		{
			//check about the item expiration
			if(IsItemExpired(now, *auction))
				expired_items.emplace_back(owner);

			// check about auction expiration
			else if (auction->duration <= 1)
				expired_auctions.emplace_back(owner);

			// check about duration update on db
			else if(--auction->duration % 5 == 0)
				_UpdateDuration(*auction);
		}

		// expired auctions
		for(auto& id : expired_auctions)
			CClientManager::instance().IkarusShopExpiredAuction(id);

		// expired auction items
		for (auto& id : expired_items)
			CClientManager::instance().IkarusShopExpiredAuctionItem(id);

		return true;
	}

	void CAuctionCache::_UpdateDuration(const TAuctionInfo & auction)
	{
		std::string query = CreateAuctionCacheUpdateDurationQuery(auction);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_AUCTION_UPDATE_DURATION, 0 , NULL);
	}

	CAuctionCache::AUCTION_OFFER_HANDLE CAuctionCache::GetBestBuyerOffer(DWORD dwOwnerID)
	{
		// searching vector
		auto it = m_mapAuctionOffer.find(dwOwnerID);
		if(it == m_mapAuctionOffer.end() || it->second.empty())
			return nullptr;

		// obtaining max element
		auto& offerlist = it->second;
		auto itbest = std::max_element(offerlist.begin(), offerlist.end(), [](auto&& first, auto&& second) { return first->price < second->price; });
		return itbest != offerlist.end() ? *itbest : nullptr;
	}

	CAuctionCache::AUCTION_HANDLE CAuctionCache::Get(DWORD dwOwnerID)
	{
		auto it = m_mapAuction.find(dwOwnerID);
		if(it == m_mapAuction.end())
			return nullptr;
		return it->second;
	}

	std::vector<CAuctionCache::AUCTION_OFFER_HANDLE> CAuctionCache::GetOffers(DWORD dwOwnerID)
	{
		auto it = m_mapAuctionOffer.find(dwOwnerID);
		if(it == m_mapAuctionOffer.end())
			return {};
		return it->second;
	}

	DWORD CAuctionCache::GetCount()
	{
		return m_mapAuction.size();
	}

	DWORD CAuctionCache::GetOffersCount()
	{
		DWORD dwCount =0;
		for(itertype(m_mapAuctionOffer) it = m_mapAuctionOffer.begin(); it != m_mapAuctionOffer.end(); it++)
			dwCount += it->second.size();

		return dwCount;
	}

	void CAuctionCache::EncodeCache(CPeer* peer)
	{
		for(auto& [id, auction] : m_mapAuction)
			peer->Encode(auction.get(), sizeof(*auction));

		for(auto& [id, offers] : m_mapAuctionOffer)
			for(auto& offer : offers)
				peer->Encode(offer.get(), sizeof(*offer));
	}


	// QUERY MAKE
	std::string CreateShopCacheUpdateItemQuery(const TShopItem& item)
	{
		// making json object
		rapidjson::Document value;
		value.SetObject();
		// inserting values
		auto& allocator = value.GetAllocator();
		jsonHelper::insertValue(value, "yang", item.price.yang, allocator);
#ifdef ENABLE_CHEQUE_SYSTEM
		jsonHelper::insertValue(value, "cheque", item.price.cheque, allocator);
#endif
		jsonHelper::insertValue(value, "expiration", static_cast<int>(item.expiration), allocator);
		// making json string
		rapidjson::StringBuffer buffer;
		buffer.Clear();
		rapidjson::Writer<rapidjson::StringBuffer> StringWriter(buffer);
		value.Accept(StringWriter);
		// escaping json string
		static char escape[1024];
		CDBManager::instance().EscapeString(escape, buffer.GetString(), buffer.GetSize());
		// making query
		return fmt::format("UPDATE `item` SET `ikashop_data`='{}' WHERE `id`={}", escape, item.id);
	}

	std::string CreateShopCacheDeleteShopQuery(DWORD dwOwner)
	{
		return fmt::format("DELETE FROM `ikashop_offlineshop` WHERE `owner` = {}", dwOwner);
	}

	std::string CreateShopCacheDeleteShopItemQuery(DWORD dwOwner)
	{
		return fmt::format("UPDATE `item` SET `ikashop_data` = '' WHERE `window` = 'IKASHOP_OFFLINESHOP' AND `owner_id` = {}", dwOwner);
	}

	std::string CreateShopCacheInsertShopQuery(DWORD dwOwnerID, DWORD dwDuration, const char* name
#ifdef ENABLE_IKASHOP_ENTITIES
		, const TShopSpawn& spawn
#endif
		, BYTE unlockCount, bool isPremium
	)
	{
		return fmt::format("REPLACE INTO `ikashop_offlineshop` (`owner`, `duration`"
#ifdef ENABLE_IKASHOP_ENTITIES
			", `map`, `x`, `y`, `channel`, `name`"
#endif
			", `unlock_count`, `is_premium`"
			") VALUES({}, {}"
#ifdef ENABLE_IKASHOP_ENTITIES
			", {}, {}, {}, {}, '{}'"
#endif
			", {}, {}"
			");",
			dwOwnerID, dwDuration
#ifdef ENABLE_IKASHOP_ENTITIES
			, spawn.map, spawn.x, spawn.y, spawn.channel, name
#endif
			, unlockCount, isPremium ? 1 : 0
		);
	}

	std::string CreateShopCacheUpdateDurationQuery(DWORD dwOwnerID, DWORD dwDuration)
	{
		return fmt::format("UPDATE `ikashop_offlineshop` SET `duration` = '{}' WHERE `owner` = {};", dwDuration, dwOwnerID);
	}

	std::string CreateSafeboxCacheDeleteItemQuery(DWORD dwItem)
	{
		return fmt::format("DELETE FROM `item` WHERE `id` = {}", dwItem);
	}

	std::string CreateSafeboxCacheInsertItemQuery(DWORD dwOwner, DWORD dwItemID, const TShopPlayerItem& item)
	{
		// making json value
		rapidjson::Document jsonValue;
		jsonValue.SetObject();

		// encoding data
		auto& allocator = jsonValue.GetAllocator();
		jsonHelper::insertValue(jsonValue, "expiration", static_cast<int>(item.expiration), allocator);
		
		// making json string
		rapidjson::StringBuffer buffer;
		buffer.Clear();
		rapidjson::Writer<rapidjson::StringBuffer> StringWriter(buffer);
		jsonValue.Accept(StringWriter);
		
		// escaping json string
		static char escape[1024];
		CDBManager::instance().EscapeString(escape, buffer.GetString(), buffer.GetSize());
		return fmt::format("UPDATE `item` SET `ikashop_data` = '{}' WHERE `id` = {}", escape, item.id);
	}

	std::string CreateSafeboxCacheUpdateValutes(DWORD dwOwner, const TValutesInfo& val)
	{
		return fmt::format("UPDATE `ikashop_safebox` SET `gold` = {} WHERE `owner` = {};", val.yang, dwOwner);
	}

	std::string CreateSafeboxCacheUpdateValutesByAdding(DWORD dwOwner, const TValutesInfo& val)
	{
		return fmt::format("UPDATE `ikashop_safebox` SET `gold` = `gold`+{}  WHERE `owner` = {};", val.yang, dwOwner);
	}

	std::string CreateSafeboxCacheInsertSafeboxValutesQuery(DWORD dwOwnerID)
	{
		return fmt::format("INSERT INTO `ikashop_safebox` (`owner`,`gold`,`cheque`) VALUES({} , 0,0);", dwOwnerID);
	}

	std::string CreateSafeboxCacheLoadValutesQuery(DWORD dwOwnerID)
	{
		return fmt::format("SELECT `gold` from `ikashop_safebox` WHERE `owner` = {};", dwOwnerID);
	}

	std::string CreateSafeboxCacheLoadItemsQuery(DWORD dwOwnerID)
	{
		std::string query = "SELECT `id`, `vnum`, `count` ";
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
			query += fmt::format(",`socket{}` ", i);
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
			query += fmt::format(",`attrtype{}` , `attrvalue{}` ", i, i);
#ifdef ENABLE_CHANGELOOK_SYSTEM
		query += " , `trans` ";
#endif
		query += fmt::format(", `ikashop_data` FROM `item` WHERE `owner_id` = {} AND `window` = 'IKASHOP_SAFEBOX'", dwOwnerID);
		return query;
	}

	//OFFERCHACHE
	std::string CreateOfferCacheInsertOfferQuery(const TOfferInfo& rOffer)
	{
		//ikashop-updated 03/08/19
		static char szEscapeString[CHARACTER_NAME_MAX_LEN + 32];
		CDBManager::instance().EscapeString(szEscapeString, rOffer.buyername, strnlen(rOffer.buyername, sizeof(rOffer.buyername)));
		// offer_id,owner_id,offerer_id,price_yang,price_cheque,is_notified,is_accept

		return fmt::format(
			"INSERT INTO `ikashop_private_offer` (`id`,`seller`,`buyer`, `item`, `price`, `notified`,`accepted`, `buyer_name`) VALUES (0, {}, {}, {}, {}, {}, {}, '{}')",
				rOffer.ownerid, rOffer.buyerid, rOffer.itemid, rOffer.price.yang, rOffer.notified?1:0, rOffer.accepted?1:0, szEscapeString);
	}

	std::string CreateOfferCacheUpdateNotifiedQuery(DWORD dwOfferID)
	{
		return fmt::format("UPDATE `ikashop_private_offer` SET `notified` = 1 WHERE `buyer` = {};", dwOfferID);
	}

	std::string CreateOfferCacheUpdateAcceptedQuery(DWORD dwOfferID)
	{
		return fmt::format("DELETE FROM `ikashop_private_offer` WHERE `id` = {};", dwOfferID);
	}

	std::string CreateOfferCacheRemoveOfferByShopOwner(DWORD dwShopOwner)
	{
		return fmt::format("DELETE from `ikashop_private_offer`  WHERE `seller` = {};", dwShopOwner);
	}

	std::string CreateOfferCacheDeleteOfferQuery(DWORD dwOfferID)
	{
		return fmt::format("DELETE from `ikashop_private_offer`  WHERE `id` = {};", dwOfferID);
	}

	std::string CreateShopCacheDeleteItemQuery(DWORD dwOwnerID, DWORD dwItemID)
	{
		return fmt::format("UPDATE `item` SET `ikashop_data` = '' WHERE `id` = {}", dwItemID);
	}

	std::string CreateAuctionCacheAddAuctionQuery(const TAuctionInfo& auction)
	{
		// making json object
		rapidjson::Document jsonValue;
		jsonValue.SetObject();
		auto& allocator = jsonValue.GetAllocator();

		// encoding values
		jsonHelper::insertValue(jsonValue, "expiration", static_cast<int>(auction.expiration), allocator);
		jsonHelper::insertValue(jsonValue, "duration", static_cast<int>(auction.duration), allocator);
		jsonHelper::insertValue(jsonValue, "price", static_cast<int64_t>(auction.price.yang), allocator);

		// constructing json string
		rapidjson::StringBuffer buffer;
		buffer.Clear();
		rapidjson::Writer<rapidjson::StringBuffer> StringWriter(buffer);
		jsonValue.Accept(StringWriter);

		// escaping json string
		static char escape[500];
		CDBManager::instance().EscapeString(escape, buffer.GetString(), buffer.GetSize());

		// making query
		return fmt::format("UPDATE `item` SET `ikashop_data` = '{}' WHERE `id` = {}", escape, auction.id);
	}

	std::string CreateAuctionCacheAddOfferQuery(const TAuctionOfferInfo& auctionOffer)
	{
		static char szEscapeName[CHARACTER_NAME_MAX_LEN+32];
		CDBManager::instance().EscapeString(szEscapeName, auctionOffer.buyername, strnlen(auctionOffer.buyername , sizeof(auctionOffer.buyername)));
		return fmt::format("INSERT INTO `ikashop_auction_offer` (`seller`, `buyer`, `buyer_name` , `price`, `seen`) "
			"VALUES( {}, {}, '{}', {}, 0 );",
			auctionOffer.ownerid, auctionOffer.buyerid, szEscapeName, auctionOffer.price.yang);
	}

	std::string CreateAuctionCacheDeleteAuctionOffers(DWORD dwOwnerID)
	{
		return fmt::format("DELETE FROM `ikashop_auction_offer` WHERE `seller` = {};", dwOwnerID);
	}

	std::string CreateAuctionCacheUpdateDurationQuery(const TAuctionInfo& info)
	{
		return CreateAuctionCacheAddAuctionQuery(info);
	}
}


#endif



// Files shared by GameCore.top
