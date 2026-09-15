#include "stdafx.h"

#ifdef ENABLE_IKASHOP_RENEWAL
#include "Main.h"
#include "Config.h"
#include "DBManager.h"
#include "QID.h"
#include "Peer.h"
#include "IkarusShopCache.h"
#include "ClientManager.h"


std::string CreateIkarusShopSelectShopItemsQuery(DWORD owner=0);
std::string CreateIkarusShopSelectAuctionsQuery();

template <class T>
const T& Decode(const char*& data){
	auto obj = reinterpret_cast<const T*>(data);
	data += sizeof(T);
	return *obj;
}


bool CClientManager::InitializeIkarusShopTable(DWORD owner)
{
	if(!InitializeIkarusShopShopTable())
		return false;

	if(!InitializeIkarusShopShopItemsTable())
		return false;

	//if(!InitializeIkarusShopOfferTable())
	//	return false;

	//if(!InitializeIkarusShopAuctionTable())
	//	return false;

	//if(!InitializeIkarusShopAuctionOfferTable())
	//	return false;
	return true;
}

bool CClientManager::InitializeIkarusShopShopTable(DWORD owner)
{
	// executing select query on shops table
	auto msg = CDBManager::instance().DirectQuery("SELECT s.`owner` , s.`duration`, s.`name`, p.`name`"
#ifdef ENABLE_IKASHOP_ENTITIES
		", s.`map`, s.`x`, s.`y`, s.`channel`"
#endif
		", s.`unlock_count`, s.`is_premium`"
		" FROM `ikashop_offlineshop` s INNER JOIN `player` p ON s.`owner` = p.`id`;");

	// handling sql errors
	if (msg->uiSQLErrno != 0 || !msg->Get()) {
		sys_err("CANNOT LOAD ikashop_offlineshop TABLE , errorcode %d sql->Get() = %p", msg->uiSQLErrno, msg->Get());
		return false;
	}

	// fetching results
	DWORD duration = 0;
	BYTE unlockCount = 0;
	BYTE isPremium = 0;

	for (uint32_t i = 0; i < msg->Get()->uiNumRows; i++)
	{
		auto row = mysql_fetch_row(msg->Get()->pSQLResult);
		int col = 0;

		str_to_number(owner, row[col++]);
		str_to_number(duration, row[col++]);
		auto name = row[col++];
		auto ownerName = row[col++];

#ifdef ENABLE_IKASHOP_ENTITIES
		ikashop::TShopSpawn spawn{};
		str_to_number(spawn.map, row[col++]);
		str_to_number(spawn.x, row[col++]);
		str_to_number(spawn.y, row[col++]);		
		str_to_number(spawn.channel, row[col++]);		
#endif
		str_to_number(unlockCount, row[col++]);
		str_to_number(isPremium, row[col++]);

		if (!m_offlineshopShopCache.PutShop(owner, duration, name, ownerName
#ifdef ENABLE_IKASHOP_ENTITIES
			, spawn
#endif
			, unlockCount, isPremium
		))
			sys_err("cannot execute putsShop -> double shop id?! %u ", owner);
	}

	return true;
}

bool CClientManager::InitializeIkarusShopShopItemsTable(DWORD owner)
{
	// executing query selecting items on window IKASHOP_OFFLINESHOP
	std::string query = CreateIkarusShopSelectShopItemsQuery(owner);
	auto msg = CDBManager::instance().DirectQuery(query.c_str());

	// handling sql errors
	if (msg->uiSQLErrno != 0 || !msg->Get()){
		sys_err("CANNOT LOAD SHOP ITEM TABLE , errorcode %d  sql %p", msg->uiSQLErrno, msg->Get());
		return false;
	}

	// fetching data
	for(uint32_t i=0; i < msg->Get()->uiNumRows; i++)
	{
		auto row = mysql_fetch_row(msg->Get()->pSQLResult);
		int col = 0;

		// fetching item info
		ikashop::CShopCache::TShopCacheItemInfo item{};
		str_to_number(item.id, row[col++]);
		str_to_number(item.owner, row[col++]);
		str_to_number(item.pos, row[col++]);
		str_to_number(item.vnum, row[col++]);
		str_to_number(item.count, row[col++]);
		// fetching sockets
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
			str_to_number(item.alSockets[i], row[col++]);
		// fetching attributes
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++) {
			str_to_number(item.aAttr[i].bType, row[col++]);
			str_to_number(item.aAttr[i].sValue, row[col++]);
		}

#ifdef ENABLE_CHANGELOOK_SYSTEM
		str_to_number(item.dwTransmutation, row[col++]);
#endif
		// fetching json
		std::string jsonStr = row[col++];
		auto jsonOptional = str_to_json(jsonStr.c_str());
		if (!jsonOptional) {
			sys_err("INVALID JSON ON SHOP ITEM %d (owner %d)  json value (%s)", item.id, item.owner, jsonStr.c_str());
			continue;
		}

		auto& json = jsonOptional.value();

		// fetching price yang
		auto priceYang = jsonHelper::getValue<int64_t>(json, "yang");
		if (!priceYang) {
			sys_err("INVALID JSON ON SHOP ITEM %d (owner %d) CANNOT EXTRACT PRICE YANG   json value (%s)", item.id, item.owner, jsonStr.c_str());
			continue;
		}
		item.price.yang = *priceYang;

		// fetching price cheque
#ifdef ENABLE_CHEQUE_SYSTEM
		auto priceCheque = jsonHelper::getValue<int>(json, "cheque");
		if (!priceCheque) {
			sys_err("INVALID JSON ON SHOP ITEM %d (owner %d) CANNOT EXTRACT PRICE CHEQUE   json value (%s)", item.id, item.owner, jsonStr.c_str());
			continue;
		}
		item.price.cheque = *priceCheque;
#endif

		// fetching expiration
		auto expiration = jsonHelper::getValue<int64_t>(json, "expiration");
		item.expiration = static_cast<ikashop::ExpirationType>(expiration ? *expiration : 0);

		auto cache = m_offlineshopShopCache.Get(item.owner);
		if (cache)
		{
			// avoid adding items from expired shops on DB initialize
			// those should be allocated only after the player logs
			// to prevent unecessary memory allocation
			if (owner == 0 && cache->duration < 1)
				continue; 

			if (!m_offlineshopShopCache.PutItem(item.owner, item.id, item/*, isSold*/))
				sys_err("cannot execute putitem !? owner %d , item %d", item.owner, item.id);
		}
		
	}

	return true;
}

bool CClientManager::InitializeIkarusShopOfferTable()
{
	// executing query selecting all offers inside ikashop_private_offer
	auto msg = CDBManager::Instance().DirectQuery("SELECT `id`, `seller`, `buyer`, `item`, `price`, `notified`, `accepted`, `buyer_name`, UNIX_TIMESTAMP(`datetime`) FROM `ikashop_private_offer`;");

	// sql errors handling
	if (msg->uiSQLErrno != 0 || !msg->Get())	{
		sys_err("CANNOT LOAD ikashop_private_offer TABLE , errorcode %d  sql %p", msg->uiSQLErrno, msg->Get());
		return false;
	}

	// fetching data
	for(uint32_t i = 0; i < msg->Get()->uiNumRows; i++){
		// fetching row
		auto row = mysql_fetch_row(msg->Get()->pSQLResult);
		int col = 0;

		// extracting data
		ikashop::TOfferInfo offer;
		str_to_number(offer.offerid, row[col++]);
		str_to_number(offer.ownerid, row[col++]);
		str_to_number(offer.buyerid, row[col++]);
		str_to_number(offer.itemid, row[col++]);
		str_to_number(offer.price.yang, row[col++]);
		str_to_number(offer.notified, row[col++]);
		str_to_number(offer.accepted, row[col++]);
		str_to_cstring(offer.buyername, row[col++]);
		str_to_number(offer.datetime, row[col++]);

		m_offlineshopOfferCache.Puts(offer);
	}

	return true;
}

bool CClientManager::InitializeIkarusShopAuctionTable()
{
	// executing query selecting all items in window IKASHOP_AUCTION
	std::string query = CreateIkarusShopSelectAuctionsQuery();
	auto msg = CDBManager::Instance().DirectQuery(query.c_str());

	// sql errors handlings
	if (msg->uiSQLErrno != 0 || !msg->Get())
	{
		sys_err("CANNOT LOAD offlineshop_auctions TABLE , errorcode %d ", msg->uiSQLErrno, msg->Get());
		return false;
	}

	// fetching data
	for(uint32_t i =0; i < msg->Get()->uiNumRows; i++){
		// fetching row
		auto row = mysql_fetch_row(msg->Get()->pSQLResult);
		int col = 0;

		// extracting data
		ikashop::TAuctionInfo info;
		str_to_number(info.id, row[col++]);
		str_to_number(info.vnum, row[col++]);
		str_to_number(info.count, row[col++]);
		str_to_number(info.owner, row[col++]);
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
			str_to_number(info.alSockets[i], row[col++]);
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
		{
			str_to_number(info.aAttr[i].bType, row[col++]);
			str_to_number(info.aAttr[i].sValue, row[col++]);
		}
#ifdef ENABLE_CHANGELOOK_SYSTEM
		str_to_number(info.dwTransmutation, row[col++]);
#endif
		auto jsonResult = str_to_json(row[col++]);
		str_to_cstring(info.ownername, row[col++]);

		if(!jsonResult){
			sys_err("FAILED FETCH JSON AUCTION : pid(%d) item(%d) playername(%s)", info.owner, info.id, info.ownername);
			continue;
		}

		// getting json value
		auto& jsonValue = jsonResult.value();

		// fetching json duration
		auto extractedDuration = jsonHelper::getValue<int>(jsonValue, "duration");
		if(!extractedDuration){
			sys_err("FAILED FETCH JSON AUCTION DURATION : pid(%d) item(%d) playername(%s)", info.owner, info.id, info.ownername);
			continue;
		}
		info.duration = *extractedDuration;

		// fetching json starting price
		auto extractedPrice = jsonHelper::getValue<int64_t>(jsonValue, "price");
		if(!extractedPrice){
			sys_err("FAILED FETCH JSON AUCTION STARTING PRICE : pid(%d) item(%d) playername(%s)", info.owner, info.id, info.ownername);
			continue;
		}
		info.price.yang = *extractedPrice;

		// fetching json expiration
		auto extractedExpiration = jsonHelper::getValue<int>(jsonValue, "expiration");
		if (!extractedExpiration) {
			sys_err("FAILED FETCH JSON AUCTION EXPIRATION : pid(%d) item(%d) playername(%s)", info.owner, info.id, info.ownername);
			continue;
		}
		info.expiration = static_cast<ikashop::ExpirationType>(*extractedExpiration);


		// registering auction
		m_offlineshopAuctionCache.PutsAuction(info);
	}

	return true;
}

bool CClientManager::InitializeIkarusShopAuctionOfferTable()
{
	// executing query selecting all auction offers
	auto msg = CDBManager::Instance().DirectQuery("SELECT `seller`, `buyer`, `buyer_name`, `price`, `seen`, UNIX_TIMESTAMP(`datetime`) FROM `ikashop_auction_offer`");

	// sql error handling
	if (msg->uiSQLErrno != 0 || !msg->Get())
	{
		sys_err("CANNOT LOAD offlineshop_auctions TABLE , errorcode %d  sql %p", msg->uiSQLErrno, msg->Get());
		return false;
	}

	// fetching result
	for(uint32_t i=0; i < msg->Get()->uiNumRows; i++){
		auto row = mysql_fetch_row(msg->Get()->pSQLResult);
		int col = 0;

		ikashop::TAuctionOfferInfo offer;
		str_to_number(offer.ownerid, row[col++]);
		str_to_number(offer.buyerid, row[col++]);
		str_to_cstring(offer.buyername, row[col++]);
		str_to_number(offer.price.yang, row[col++]);
		str_to_number(offer.seenflag, row[col++]);
		str_to_number(offer.datetime, row[col++]);
		m_offlineshopAuctionCache.AddOffer(offer, false);
	}

	return true;
}

void CClientManager::SendIkarusShopTable(CPeer* peer)
{
	peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0,
		sizeof(TPacketDGNewIkarusShop) + sizeof(ikashop::TSubPacketDGLoadTables) +
		sizeof(ikashop::TShopInfo)	* m_offlineshopShopCache.GetCount() + sizeof(ikashop::TShopItem) * m_offlineshopShopCache.GetItemCount()  +
		sizeof(ikashop::TOfferInfo)	* m_offlineshopOfferCache.GetCount() +
		sizeof(ikashop::TAuctionInfo) * m_offlineshopAuctionCache.GetCount() +
		sizeof(ikashop::TAuctionOfferInfo) * m_offlineshopAuctionCache.GetOffersCount()
	);


	TPacketDGNewIkarusShop pack{};
	pack.bSubHeader	= ikashop::SUBHEADER_DG_LOAD_TABLES;

	ikashop::TSubPacketDGLoadTables subPack{};
	subPack.shopcount = m_offlineshopShopCache.GetCount();
	subPack.offercount = m_offlineshopOfferCache.GetCount();
	subPack.auctioncount = m_offlineshopAuctionCache.GetCount();
	subPack.auctionoffercount = m_offlineshopAuctionCache.GetOffersCount();

	peer->Encode(&pack, sizeof(pack));
	peer->Encode(&subPack, sizeof(subPack));

	

	m_offlineshopShopCache.EncodeCache(peer);
	m_offlineshopOfferCache.EncodeCache(peer);
	m_offlineshopAuctionCache.EncodeCache(peer);

}

void CClientManager::RecvIkarusShopPacket(CPeer* peer,const char* data)
{
	const auto& pack = Decode<TPacketDGNewIkarusShop>(data);

	bool success=false;

	switch (pack.bSubHeader)
	{
	case ikashop::SUBHEADER_GD_BUY_ITEM:
		success=RecvIkarusShopBuyItemPacket(peer, data);
		break;
	case ikashop::SUBHEADER_GD_BUY_LOCK_ITEM:
		success=RecvIkarusShopLockBuyItem(peer,data);
		break;
	case ikashop::SUBHEADER_GD_EDIT_ITEM:
		success=RecvIkarusShopEditItemPacket(data);
		break;
	case ikashop::SUBHEADER_GD_REMOVE_ITEM:
		success=RecvIkarusShopRemoveItemPacket(peer, data);
		break;
	case ikashop::SUBHEADER_GD_ADD_ITEM:
		success=RecvIkarusShopAddItemPacket(data);
		break;

	case ikashop::SUBHEADER_GD_SHOP_FORCE_SOFT_CLOSE:
		success= RecvIkarusShopForceSoftClose(peer, data);
		break;
	case ikashop::SUBHEADER_GD_SHOP_FORCE_HARD_CLOSE:
		success = RecvIkarusShopForceHardClose(peer, data);
		break;

	case ikashop::SUBHEADER_GD_SHOP_CREATE_NEW:
		success=RecvIkarusShopCreateNew(data);
		break;
	case ikashop::SUBHEADER_GD_SHOP_CHANGE_NAME:
		success=RecvIkarusShopChangeName(data);
		break;

	case ikashop::SUBHEADER_GD_OFFER_CREATE:
		success=RecvIkarusShopOfferCreate(data);
		break;
	case ikashop::SUBHEADER_GD_OFFER_NOTIFIED:
		success=RecvIkarusShopOfferNotified(data);
		break;
	case ikashop::SUBHEADER_GD_OFFER_ACCEPT:
		success=RecvIkarusShopOfferAccepted(data);
		break;
	case ikashop::SUBHEADER_GD_OFFER_CANCEL:
		success=RecvIkarusShopOfferCancel(data);
		break;

	case ikashop::SUBHEADER_GD_SAFEBOX_GET_ITEM:
		success=RecvIkarusShopSafeboxGetItem(peer, data);
		break;
	case ikashop::SUBHEADER_GD_SAFEBOX_GET_VALUTES:
		success=RecvIkarusShopSafeboxGetValutes(data);
		break;
	case ikashop::SUBHEADER_GD_SAFEBOX_ADD_ITEM:
		success=RecvIkarusShopSafeboxAddItem(data);
		break;
	case ikashop::SUBHEADER_GD_SAFEBOX_GET_TIME_FEEDBACK:
		success=RecvIkarusShopSafeboxGetItemFeedback(data);
		break;

	//AUCTION
	case ikashop::SUBHEADER_GD_AUCTION_CREATE:
		success=RecvIkarusShopAuctionCreate(data);
		break;
	case ikashop::SUBHEADER_GD_AUCTION_ADD_OFFER:
		success=RecvIkarusShopAuctionAddOffer(data);
		break;
	case ikashop::SUBHEADER_GD_AUCTION_CANCEL:
		success=RecvIkarusShopAuctionCancel(data);
		break;
	case ikashop::SUBHEADER_GD_AUCTION_OFFER_SEEN:
		success=RecvIkarusShopAuctionOfferSeen(data);
		break;

#ifdef EXTEND_IKASHOP_PRO
	case ikashop::SUBHEADER_GD_NOTIFICATION_SEEN:
		success=RecvIkarusShopNotificationSeen(data);
		break;
	case ikashop::SUBHEADER_GD_SHOP_RESTORE_DURATION:
		success=RecvIkarusShopRestoreDuration(data);
		break;
#ifdef ENABLE_IKASHOP_ENTITIES
	case ikashop::SUBHEADER_GD_MOVE_SHOP_ENTITY:
		success=RecvIkarusShopMoveShopEntity(data);
		break;
#endif
#endif

	case ikashop::SUBHEADER_GD_OWNER_ONLINE:
		success= RecvIkarusShopOwnerOnline(data);
		break;

	case ikashop::SUBHEADER_GD_SHOP_LOCKED_SLOT_STATE:
		success = RecvIkarusShopLockedSlotState(data);
		break;

	case ikashop::SUBHEADER_GD_LOAD_EXPIRED_SHOP:
		success = RecvIkarusShopLoadExpiredShop(data);
		break;

	default:
		sys_err("UNKNOW NEW OFFLINESHOP SUBHEADER GD %d",pack.bSubHeader);
		break;
	}


	if(!success)
		sys_err("maybe some error during recv offline shop subheader %d ",pack.bSubHeader);
}

bool CClientManager::RecvIkarusShopLockBuyItem(CPeer* peer, const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDLockBuyItem>(data);
	
	// searching shop
	auto shop = m_offlineshopShopCache.Get(subpack.ownerid);
	if(!shop)
	{
		sys_err("cannot find buy target item %u (owner %u , buyer %u) ", subpack.itemid, subpack.ownerid, subpack.guestid);
		SendIkarusShopBuyLockedItemPacket(peer, 0, subpack.guestid, subpack.itemid);
		return false;
	}

	// searching item
	auto itemIter = shop->itemsmap.find(subpack.itemid);
	if(itemIter == shop->itemsmap.end())
	{
		sys_err("cannot find buy target item %u (owner %u , buyer %u) ", subpack.itemid, subpack.ownerid, subpack.guestid);
		SendIkarusShopBuyLockedItemPacket(peer, 0, subpack.guestid, subpack.itemid);
		return false;
	}

	auto item = itemIter->second;

	// finally selling it
	if (m_offlineshopShopCache.LockSellItem(subpack.ownerid, subpack.itemid, subpack.seenprice)){
#ifdef ENABLE_IKASHOP_LOGS
		IkarusShopLog(subpack.guestid, subpack.itemid, "LOCK_ITEM_TO_BUY", subpack.ownerid, "", item->vnum, item->count, item->price.yang
#ifdef ENABLE_CHEQUE_SYSTEM
			, item->price.cheque
#endif
		);
#endif

		SendIkarusShopBuyLockedItemPacket(peer, subpack.ownerid, subpack.guestid, subpack.itemid);
#ifdef EXTEND_IKASHOP_PRO
		std::string itemPrice = fmt::format("{}", item->price.yang);
#ifdef ENABLE_CHEQUE_SYSTEM
		itemPrice += fmt::format("|{}", item->price.cheque);
#endif
		AppendIkarusShopNotification(
			ikashop::ENotificationType::SELLER_SOLD_ITEM, subpack.ownerid, item->vnum, "", itemPrice);
#endif
	}

	else
	{
		SendIkarusShopBuyLockedItemPacket(peer, 0, subpack.guestid, subpack.itemid);
		sys_err("cannot find buy target item %u (owner %u , buyer %u) ", subpack.itemid, subpack.ownerid, subpack.guestid);
	}
	return true;
}

bool CClientManager::RecvIkarusShopBuyItemPacket(CPeer* peer, const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDBuyItem>(data);

	DWORD dwOwner = subpack.ownerid;
	DWORD dwItem  = subpack.itemid;
	bool success = subpack.success;	
	int tax_pct = MINMAX(0,subpack.tax,99);

	auto shopCache = m_offlineshopShopCache.Get(subpack.ownerid);
	if(!shopCache)
		return false;

	auto it = shopCache->itemsmap.find(dwItem);
	if(shopCache->itemsmap.end() == it)
		return false;

	// unlocking locked item
	if(!success){
		m_offlineshopShopCache.UnlockSellItem(dwOwner, dwItem);
		return true;
	}

	auto price = it->second->price;
	// mt2009: since cheque is not enabled the math is easy
	long long tax_price = price.yang * tax_pct / 100;
	price.yang -= tax_price;

	ikashop::TValutesInfo valute;
	valute.yang = price.GetTotalYangAmount();

	m_offlineshopSafeboxCache.AddValutes(subpack.ownerid, valute);
	SendIkarusShopSafeboxAddValutes(subpack.ownerid, valute);
	
	//check about offers
	RemoveOfferOnShopItem(dwOwner, dwItem);

	//ikashop-updated 04/08/19
	SendIkarusShopBuyItemPacket(dwOwner, subpack.guestid, dwItem, peer);

#ifdef ENABLE_IKASHOP_LOGS
	auto& item = it->second;
	IkarusShopLog(subpack.guestid, subpack.itemid, "BUY_ITEM", subpack.ownerid, fmt::format("TAX: {}", tax_price), item->vnum, item->count, item->price.yang
#ifdef ENABLE_CHEQUE_SYSTEM
		, item->price.cheque
#endif
	);
#endif

	if(!m_offlineshopShopCache.SellItem(dwOwner, dwItem))
		sys_err("some problem with sell : %u %u ",dwOwner, dwItem);

	return true;
}

bool CClientManager::RecvIkarusShopEditItemPacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDEditItem>(data);
	
	if(m_offlineshopShopCache.EditItem(subpack.ownerid, subpack.itemid, subpack.price)){
		SendIkarusShopEditItemPacket(subpack.ownerid, subpack.itemid, subpack.price);
#ifdef ENABLE_IKASHOP_LOGS
		auto item = m_offlineshopShopCache.GetItem(subpack.ownerid, subpack.itemid);
		IkarusShopLog(subpack.ownerid, subpack.itemid, "EDIT_PRICE", subpack.ownerid, "", item->vnum, item->count, item->price.yang
#ifdef ENABLE_CHEQUE_SYSTEM
			, item->price.cheque
#endif
		);
#endif
	}

	else
		sys_err("cannot edit item %u , shop %u ",subpack.itemid , subpack.ownerid);
	return true;
}

#define DEBUG_REMOVE_ITEM_PACKET
bool CClientManager::RecvIkarusShopRemoveItemPacket(CPeer* peer, const char* data) //patchme
{
	const auto& subpack = Decode<ikashop::TSubPacketGDRemoveItem>(data);
	
	// getting shop
	auto shopCache = m_offlineshopShopCache.Get(subpack.ownerid);
	if (!shopCache)
	{
#ifdef DEBUG_REMOVE_ITEM_PACKET
		sys_log(0, "DEBUG_REMOVE_ITEM_PACKET: NO SHOP CACHE owner %d itemid %d ", subpack.ownerid, subpack.itemid);
#endif
		return false;
	}

	// getting item cache
	auto it = shopCache->itemsmap.find(subpack.itemid);
	if (it == shopCache->itemsmap.end())
	{
#ifdef DEBUG_REMOVE_ITEM_PACKET
		sys_log(0, "DEBUG_REMOVE_ITEM_PACKET: NO ITEM CACHE owner %d itemid %d ", subpack.ownerid, subpack.itemid);
#endif
		return false;
	}

	RemoveOfferOnShopItem(subpack.ownerid, subpack.itemid);

#ifdef ENABLE_IKASHOP_LOGS
	auto item = it->second;
	IkarusShopLog(subpack.ownerid, subpack.itemid, "REMOVE_ITEM", subpack.ownerid, "", item->vnum, item->count, item->price.yang
#ifdef ENABLE_CHEQUE_SYSTEM
		, item->price.cheque
#endif
	);
#endif

	//topatch
	auto copy = it->second;
	if (m_offlineshopShopCache.RemoveItem(subpack.ownerid, subpack.itemid))
		SendIkarusShopRemoveItemPacket(subpack.ownerid, subpack.itemid, peer);

	else
		sys_err("cannot remove item %u shop %u item ?!", subpack.ownerid, subpack.itemid);

	return true;
}

bool CClientManager::RecvIkarusShopAddItemPacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDAddItem>(data);
	ikashop::CShopCache::TShopCacheItemInfo item{subpack.sitem};
	item.lock = false;

#ifdef ENABLE_IKASHOP_LOGS
	IkarusShopLog(subpack.ownerid, item.id, "ADD_ITEM", subpack.ownerid, "", item.vnum, item.count, item.price.yang
#ifdef ENABLE_CHEQUE_SYSTEM
		, item.price.cheque
#endif
	);
#endif

	m_offlineshopShopCache.AddItem(item, QID_OFFLINESHOP_ADD_ITEM);
	return true;
}

// soft close meaning the shop is still saved in the database with its content but shop's entity is not spawned
// so basically it has the same functionality as shop expire
bool CClientManager::RecvIkarusShopForceSoftClose(CPeer* peer, const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDShopForceClose>(data);

	auto shopCache = m_offlineshopShopCache.Get(subpack.ownerid);
	if(!shopCache)
		return false;

#ifdef ENABLE_IKASHOP_LOGS
	IkarusShopLog(subpack.ownerid, 0, subpack.gm ? "GM_CLOSE_SHOP" : "CLOSE_SHOP", subpack.ownerid);
#endif

	shopCache->duration = 0;
	m_offlineshopShopCache.UpdateDurationQuery(subpack.ownerid, *shopCache);
	m_offlineshopOfferCache.RemoveOffersByShopOwner(subpack.ownerid);
	SendIkarusShopShopExpired(subpack.ownerid);
	return true;
}

// hard close meaning removes the offlineshop from the DB
bool CClientManager::RecvIkarusShopForceHardClose(CPeer* peer, const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDShopForceClose>(data);
	auto shopCache = m_offlineshopShopCache.Get(subpack.ownerid);
	if (!shopCache)
		return false;

#ifdef ENABLE_MT2009_DISABLE_IKASHOP_DEFAULT_SAFEBOX
	if (shopCache->itemsmap.size() > 0)
	{
		sys_err("HARD CLOSE OFFLINESHOP WITH ITEM COUNT > 0 (%d) owner %d ", shopCache->itemsmap.size(), subpack.ownerid);
		return false;
	}
#else
	// caching item list
	auto items{ shopCache->itemsmap };
#endif

#ifdef ENABLE_IKASHOP_LOGS
	IkarusShopLog(subpack.ownerid, 0, "HARD_CLOSE_SHOP", subpack.ownerid);
#endif

	RemoveOfferOnShop(subpack.ownerid);
	m_offlineshopOfferCache.RemoveOffersByShopOwner(subpack.ownerid);
	m_offlineshopShopCache.CloseShop(subpack.ownerid);
	SendIkarusShopForceClose(subpack.ownerid);

#ifndef ENABLE_MT2009_DISABLE_IKASHOP_DEFAULT_SAFEBOX
	// putting item into safebox
	for (auto [id, item] : items)
		m_offlineshopSafeboxCache.AddItem(subpack.ownerid, id, *item);
#endif
	return true;
}

bool CClientManager::RecvIkarusShopCreateNew(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDShopCreateNew>(data);

	m_offlineshopShopCache.CreateShop(subpack.shop.ownerid , subpack.shop.duration , subpack.shop.name, subpack.shop.ownerName
#ifdef ENABLE_IKASHOP_ENTITIES
		, subpack.shop.spawn, subpack.items
#endif
		, subpack.shop.unlockCount, subpack.shop.isPremium);

#ifdef ENABLE_IKASHOP_LOGS
	IkarusShopLog(subpack.shop.ownerid, 0, "CREATE_SHOP", subpack.shop.ownerid);
#endif
	return true;
}

bool CClientManager::RecvIkarusShopChangeName(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDShopChangeName>(data);
	if (m_offlineshopShopCache.ChangeShopName(subpack.ownerid, subpack.name))
	{
		// making query
		auto query = fmt::format("UPDATE `ikashop_offlineshop` SET `name`='{}' WHERE owner={}",
			subpack.name, subpack.ownerid);

		// executing query
		auto msg = CDBManager::instance().DirectQuery(query.c_str());
		if (!msg || !msg->Get() || msg->uiSQLErrno != 0)
		{
			sys_err("CANNOT UPDATE SHOP NAME, SKIPPING: Owner (%u) name %s ", subpack.ownerid, subpack.name);
			return false;
		}

		SendIkarusShopChangeName(subpack.ownerid, subpack.name);
	}
	return true;
}

bool CClientManager::RecvIkarusShopOfferCreate(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDOfferCreate>(data);

	// searching shop
	auto item = m_offlineshopShopCache.GetItem(subpack.ownerid, subpack.itemid);
	if (!item)
	{
#ifdef ENABLE_IKASHOP_LOGS
		IkarusShopLog(subpack.offer.buyerid, subpack.offer.itemid, "PRIVATE_OFFER_CREATE_AND_DELETED", subpack.offer.ownerid, "", subpack.vnum, subpack.count, subpack.offer.price.yang
#ifdef ENABLE_CHEQUE_SYSTEM
			, subpack.offer.price.cheque
#endif
		);
#endif

#ifdef EXTEND_IKASHOP_PRO
		AppendIkarusShopNotification(ikashop::ENotificationType::BUYER_OFFER_DENIED, subpack.offer.buyerid, subpack.vnum, "", fmt::format("{}", subpack.offer.price.yang));
#endif

		ikashop::TValutesInfo valutes{};
		valutes.yang = subpack.offer.price.GetTotalYangAmount();
		m_offlineshopSafeboxCache.AddValutes(subpack.offer.buyerid, valutes);
		SendIkarusShopSafeboxAddValutes(subpack.offer.buyerid, valutes);
		return false;
	}

	ikashop::TOfferInfo info;
	info.accepted = false;
	info.notified = false;
	info.itemid = subpack.itemid;
	info.buyerid = subpack.offer.buyerid;
	info.ownerid = subpack.offer.ownerid;
	info.price = subpack.offer.price;
	info.datetime = static_cast<int64_t>(std::time(nullptr));
	strncpy(info.buyername, subpack.offer.buyername, sizeof(info.buyername));

	if(m_offlineshopOfferCache.AddOffer(info)){
#ifdef EXTEND_IKASHOP_PRO
		if(auto item = m_offlineshopShopCache.GetItem(subpack.offer.ownerid, subpack.offer.itemid))
			AppendIkarusShopNotification(
				ikashop::ENotificationType::SELLER_NEW_OFFER_ON_ITEM, subpack.offer.ownerid, item->vnum, info.buyername, fmt::format("{}", subpack.offer.price.yang));
#endif

#ifdef ENABLE_IKASHOP_LOGS
		IkarusShopLog(info.buyerid, info.itemid, "PRIVATE_OFFER_CREATE", info.ownerid, "", item->vnum, item->count, info.price.yang
#ifdef ENABLE_CHEQUE_SYSTEM
			, info.price.cheque
#endif
		);
#endif
		return true;
	}

	return false;
}

bool CClientManager::RecvIkarusShopOfferNotified(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDOfferNotified>(data);
	if(m_offlineshopOfferCache.NotifiedOffer(subpack.offerid))
		SendIkarusShopOfferNotified(subpack.ownerid, subpack.offerid);
	return true;
}

bool CClientManager::RecvIkarusShopOfferAccepted(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDOfferAccept>(data);

	auto offerCache = m_offlineshopOfferCache.Get(subpack.offerid);
	if(!offerCache)
		return true;

	if(offerCache->accepted)
		return true;

	// searching shop item cache
	auto item = m_offlineshopShopCache.GetItem(offerCache->ownerid, offerCache->itemid);
	if(!item)
	{
		sys_err("cannot find shop item shop(%u), item(%u), offer(%u) ", offerCache->ownerid, offerCache->itemid, subpack.offerid);
		return false;
	}

	// skipping locked items
	if(item->lock)
		return true;

	// editing shop price
	if (!m_offlineshopShopCache.EditItem(offerCache->ownerid, offerCache->itemid, offerCache->price)) {
		sys_err("cannot edit shop item shop(%u), item(%u), offer(%u) ", offerCache->ownerid, offerCache->itemid, subpack.offerid);
		return false;
	}

	// accepting offer
	if (m_offlineshopOfferCache.AcceptOffer(subpack.offerid))
		SendIkarusShopOfferAccepted(subpack.ownerid, subpack.offerid);

	// check about offers
	RemoveOfferOnShopItem(offerCache->ownerid, offerCache->itemid);

	// locking the item so that item can be sold
	item->lock = true;

	ikashop::TValutesInfo valute;
	valute.yang = offerCache->price.GetTotalYangAmount();

	// adding money to safebox
	m_offlineshopSafeboxCache.AddValutes(offerCache->ownerid, valute);
	SendIkarusShopSafeboxAddValutes(offerCache->ownerid, valute);

	// selling item
	if (!m_offlineshopShopCache.SellItem(offerCache->ownerid, offerCache->itemid, false))
		sys_err("cannot sell item %u, shop %u ", offerCache->itemid, offerCache->ownerid);

	m_offlineshopSafeboxCache.AddItem(offerCache->buyerid, item->id, *item);

#ifdef EXTEND_IKASHOP_PRO
	AppendIkarusShopNotification(
		ikashop::ENotificationType::BUYER_OFFER_ACCEPTED, offerCache->buyerid, item->vnum, "", fmt::format("{}", offerCache->price.yang));
#endif

#ifdef ENABLE_IKASHOP_LOGS
	IkarusShopLog(offerCache->buyerid, offerCache->itemid, "PRIVATE_OFFER_ACCEPTED", offerCache->ownerid, "", item->vnum, item->count, item->price.yang
#ifdef ENABLE_CHEQUE_SYSTEM
		, item->price.cheque
#endif
	);
#endif

	return true;
}

bool CClientManager::RecvIkarusShopOfferCancel(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDOfferCancel>(data);

	auto offer = m_offlineshopOfferCache.Get(subpack.offerid);
	if(!offer)
		return false;

	ikashop::TValutesInfo valute{};
	valute.yang = offer->price.GetTotalYangAmount();

	m_offlineshopSafeboxCache.AddValutes(offer->buyerid, valute);
	SendIkarusShopSafeboxAddValutes(offer->buyerid, valute);

	//ikashop-updated 05/08/19
	if(m_offlineshopOfferCache.CancelOffer(subpack.offerid, subpack.ownerid)){
		SendIkarusShopOfferCancel(subpack.ownerid, subpack.offerid, false);
		auto item = m_offlineshopShopCache.GetItem(offer->ownerid, offer->itemid);
#ifdef EXTEND_IKASHOP_PRO
		// getting the item
		if (item)
		{
			const auto type = subpack.requester == offer->buyerid ? ikashop::ENotificationType::SELLER_OFFER_CANCELLED : ikashop::ENotificationType::BUYER_OFFER_DENIED;
			const auto pid = subpack.requester == offer->buyerid ? subpack.ownerid : offer->buyerid;
			AppendIkarusShopNotification(type, pid, item->vnum, "", fmt::format("{}", offer->price.yang));
		}
#endif
#ifdef ENABLE_IKASHOP_LOGS
		IkarusShopLog(offer->buyerid, offer->itemid, "PRIVATE_OFFER_CANCEL", offer->ownerid, "", item ? item->vnum : 0, item ? item->count : 0, item ? item->price.yang : 0
#ifdef ENABLE_CHEQUE_SYSTEM
			, item ? item->price.cheque : 0
#endif
		);
#endif
	}
	else
		sys_err("some problems with cache cancel offer");

	return true;
}

bool CClientManager::RecvIkarusShopSafeboxGetItem(CPeer* peer, const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDSafeboxGetItem>(data);
	if(m_offlineshopSafeboxCache.LockItem(subpack.ownerid , subpack.itemid))
	{
#ifdef ENABLE_IKASHOP_LOGS
		auto safebox = m_offlineshopSafeboxCache.Get(subpack.ownerid);
		auto& item = safebox->lockedmap.find(subpack.itemid)->second;
		IkarusShopLog(subpack.ownerid, subpack.itemid, "SHOPPING_SAFEBOX_GET_ITEM", 0, "", item.vnum, item.count);
#endif
		SendIkarusShopSafeboxGetItemConfirm(peer, subpack.ownerid, subpack.itemid);
		return true;
	}
	return false;
}

bool CClientManager::RecvIkarusShopSafeboxGetValutes(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDSafeboxGetValutes>(data);
	if(m_offlineshopSafeboxCache.RemoveValutes(subpack.ownerid , subpack.valute)){
#ifdef ENABLE_IKASHOP_LOGS
		IkarusShopLog(subpack.ownerid, 0, "SHOPPING_SAFEBOX_GET_VALUTES", 0, "", 0, 0, subpack.valute.yang);
#endif
		return true;
	}
	return false;
}

bool CClientManager::RecvIkarusShopSafeboxAddItem(const char* data) {
	const auto& subpack = Decode<ikashop::TSubPacketGDSafeboxAddItem>(data);
	if (!m_offlineshopSafeboxCache.AddItem(subpack.ownerid, subpack.itemid, subpack.item))
		return false;
#ifdef ENABLE_IKASHOP_LOGS
	IkarusShopLog(subpack.ownerid, subpack.itemid, "SHOPPING_SAFEBOX_ADD_ITEM", 0, "", subpack.item.vnum, subpack.item.count);
#endif
	return true;
}

bool CClientManager::RecvIkarusShopSafeboxGetItemFeedback(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDSafeboxGetItemFeedback>(data);

#ifdef ENABLE_IKASHOP_LOGS
	ikashop::TShopPlayerItem* item = nullptr;
	if (auto safebox = m_offlineshopSafeboxCache.Get(subpack.ownerid))
		if (auto iter = safebox->itemsmap.find(subpack.itemid); iter != safebox->itemsmap.end())
			item = &iter->second;
	IkarusShopLog(subpack.ownerid, subpack.itemid, "SHOPPING_SAFEBOX_GET_ITEM_RESULT", 0, subpack.result ? "SUCCESS" : "FAILED", item ? item->vnum : 0, item ? item->count : 0);
#endif

	return subpack.result ?
		m_offlineshopSafeboxCache.RemoveLockedItem(subpack.ownerid, subpack.itemid) :
		m_offlineshopSafeboxCache.UnlockItem(subpack.ownerid, subpack.itemid);
}

//AUCTION
bool CClientManager::RecvIkarusShopAuctionCreate(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDAuctionCreate>(data);

	

	if(m_offlineshopAuctionCache.AddAuction(subpack.auction))
	{
#ifdef ENABLE_IKASHOP_LOGS
		IkarusShopLog(subpack.auction.owner, subpack.auction.id, "CREATE_AUCTION", subpack.auction.owner, "", subpack.auction.vnum, subpack.auction.count, subpack.auction.price.GetTotalYangAmount());
#endif
		SendIkarusShopAuctionCreate(subpack.auction);
	}
	else
		sys_err("cannot create auction: %u , %u , %s ",subpack.auction.owner , subpack.auction.duration, subpack.auction.ownername);

	return true;
}

bool CClientManager::RecvIkarusShopAuctionAddOffer(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDAuctionAddOffer>(data);
	auto auction = m_offlineshopAuctionCache.Get(subpack.offer.ownerid);

	// checking auction does exist 
	if (!auction)
	{
		// giving back money to buyer
		ikashop::TValutesInfo valute;
		valute.yang = subpack.offer.price.GetTotalYangAmount();

		m_offlineshopSafeboxCache.AddValutes(subpack.offer.buyerid, valute);
		SendIkarusShopSafeboxAddValutes(subpack.offer.buyerid, valute);

#ifdef ENABLE_IKASHOP_LOGS
		IkarusShopLog(subpack.offer.buyerid, 0, "REFUND_OFFER_AUCTION_ALREADY_EXPIRED", subpack.offer.ownerid, "", 0, 0, subpack.offer.price.GetTotalYangAmount());
#endif
		return true;
	}

	auto bestoffer = m_offlineshopAuctionCache.GetBestBuyerOffer(subpack.offer.ownerid);
	if (bestoffer)
	{
		ikashop::TValutesInfo valute;
		valute.yang = bestoffer->price.GetTotalYangAmount();

		m_offlineshopSafeboxCache.AddValutes(bestoffer->buyerid, valute);
		SendIkarusShopSafeboxAddValutes(bestoffer->buyerid, valute);

#ifdef ENABLE_IKASHOP_LOGS
		if (auction)
			IkarusShopLog(bestoffer->buyerid, auction->id, "REFUND_OFFER", auction->owner, "", auction->vnum, auction->count, bestoffer->price.GetTotalYangAmount());
#endif

#ifdef EXTEND_IKASHOP_PRO
		// getting the item
		if (auction)
			AppendIkarusShopNotification(
				ikashop::ENotificationType::BUYER_AUCTION_RAISE, 
					bestoffer->buyerid, auction->vnum, "", fmt::format("{}", bestoffer->price.yang));
#endif
	}

	if(m_offlineshopAuctionCache.AddOffer(subpack.offer)){
#ifdef EXTEND_IKASHOP_PRO
		// getting the item
		if (auction)
			AppendIkarusShopNotification(
				ikashop::ENotificationType::SELLER_AUCTION_NEW_OFFER, 
					subpack.offer.ownerid, auction->vnum, subpack.offer.buyername, fmt::format("{}", subpack.offer.price.yang));
#endif
#ifdef ENABLE_IKASHOP_LOGS
		if (auction)
			IkarusShopLog(subpack.offer.buyerid, auction->id, "NEW_AUCTION_OFFER", auction->owner, "", auction->vnum, auction->count, subpack.offer.price.GetTotalYangAmount());
#endif
		SendIkarusShopAuctionAddOffer(subpack.offer);
	}

	return true;
}

bool CClientManager::RecvIkarusShopAuctionCancel(const char* data)
{
	const auto& ownerid = Decode<DWORD>(data);
	// checking auction does exists and has no offers
	auto auction = m_offlineshopAuctionCache.Get(ownerid);
	if(auction && m_offlineshopAuctionCache.GetOffers(ownerid).empty()){
#ifdef ENABLE_IKASHOP_LOGS
		IkarusShopLog(ownerid, auction->id, "AUCTION_CANCEL", auction->owner, "", auction->vnum, auction->count);
#endif
		IkarusShopExpiredAuction(ownerid);
	}
	return true;
}

bool CClientManager::RecvIkarusShopAuctionOfferSeen(const char* data)
{
	const auto& inputSubpack = Decode<ikashop::TSubPacketGDAuctionOfferSeen>(data);
	// getting auction offers
	auto offers = m_offlineshopAuctionCache.GetOffers(inputSubpack.ownerid);
	for(auto& offer : offers){
		// skipping all non matching offers
		if(offer->seenflag)
			continue;
		if(offer->buyerid != inputSubpack.buyerid)
			continue;
		if(offer->price.yang != inputSubpack.price)
			continue;
		
		// moving the seenflag to true
		offer->seenflag = true;
		
		// updating table value
		auto query = fmt::format("UPDATE `ikashop_auction_offer` SET `seen` = 1 WHERE `seller` = {} AND `buyer` = {} AND `price`={}", inputSubpack.ownerid, inputSubpack.buyerid, inputSubpack.price);
		CDBManager::instance().AsyncQuery(query.c_str());
		
		// forwarding to cores
		TPacketDGNewIkarusShop pack{ ikashop::SUBHEADER_DG_AUCTION_OFFER_SEEN };
		ikashop::TSubPacketDGAuctionOfferSeen subpack;
		subpack.ownerid = inputSubpack.ownerid;
		subpack.buyerid = inputSubpack.buyerid;
		subpack.price = inputSubpack.price;
		for (auto& peer : m_peerList)
		{
			if(peer->GetChannel() != 0)
			{
				peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
				peer->Encode(pack);
				peer->Encode(subpack);
			}
		}
		break;
	}

	return true;
}

#ifdef EXTEND_IKASHOP_PRO
bool CClientManager::RecvIkarusShopNotificationSeen(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketGDNotificationSeen>(data);
	auto query = fmt::format("DELETE FROM `ikashop_notification` WHERE `id` = {}", subpack.id);
	CDBManager::instance().AsyncQuery(query.c_str());
	return true;
}

bool CClientManager::RecvIkarusShopRestoreDuration(const char* data)
{
	const auto& owner = Decode<DWORD>(data);
	auto shop = m_offlineshopShopCache.Get(owner);
	if(!shop 
		|| shop->duration != 0
		)
		return false;

	// updating on cache
	shop->duration = OFFLINESHOP_DURATION_MAX_MINUTES;
	
	// updating on table
	auto query = fmt::format("UPDATE `ikashop_offlineshop` SET `duration` = {} WHERE `owner` = {}", shop->duration, owner);
	CDBManager::instance().AsyncQuery(query.c_str());
	
	// forwarding to cores
	TPacketDGNewIkarusShop pack{};
	pack.bSubHeader = ikashop::SUBHEADER_DG_SHOP_RESTORE_DURATION;

#ifdef ENABLE_IKASHOP_LOGS
	IkarusShopLog(owner, 0, "SHOP_RESTORE_DURATION", owner, "");
#endif

	for(auto& peer : m_peerList)
	{
		if(peer->GetChannel() != 0)
		{
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(DWORD));
			peer->Encode(pack);
			peer->Encode(owner);
		}
	}
	return true;
}
#endif

#ifdef ENABLE_IKASHOP_ENTITIES
#ifdef EXTEND_IKASHOP_PRO
bool CClientManager::RecvIkarusShopMoveShopEntity(const char* data)
{
	const auto& info = Decode<ikashop::TSubPacketGDMoveShopEntity>(data);
	if(m_offlineshopShopCache.MoveShopEntity(info.owner, info.spawn))
	{
		TPacketDGNewIkarusShop pack{};
		pack.bSubHeader = ikashop::SUBHEADER_DG_MOVE_SHOP_ENTITY;

		ikashop::TSubPacketDGMoveShopEntity subpack{};
		subpack.owner = info.owner;
		subpack.spawn = info.spawn;

#ifdef ENABLE_IKASHOP_LOGS
		IkarusShopLog(info.owner, 0, "MOVE_SHOP_ENTITY", info.owner, fmt::format("NEW_POS: map({})  x({})  y({}) channel({})", subpack.spawn.map, subpack.spawn.x, subpack.spawn.y, subpack.spawn.channel));
#endif

		for(auto& peer : m_peerList)
		{
			if(peer->GetChannel() == subpack.spawn.channel)
			{
				peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
				peer->Encode(pack);
				peer->Encode(subpack);
			}
		}
	}
	return true;
}
#endif
#endif

bool CClientManager::RecvIkarusShopOwnerOnline(const char* data)
{
	const auto& info = Decode<ikashop::TSubPacketGDOwnerOnline>(data);

	TPacketDGNewIkarusShop pack{};
	pack.bSubHeader = ikashop::SUBHEADER_DG_SHOP_OWNER_ONLINE;

	ikashop::TSubPacketDGOwnerOnline subpack{};
	subpack.owner = info.ownerid;
	subpack.isOnline = info.isOnline;

	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0)
		{
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(pack);
			peer->Encode(subpack);
		}
	}
	return true;
}

bool CClientManager::RecvIkarusShopLockedSlotState(const char* data)
{
	const auto& info = Decode<ikashop::TSubPacketGDLockedSlotState>(data);

	if (m_offlineshopShopCache.SetLockedSlotState(info.ownerid, info.unlockCount, info.isPremium))
	{
		// updating table value
		auto query = fmt::format("UPDATE `ikashop_offlineshop` SET `unlock_count`={}, is_premium={} WHERE `owner`={}", info.unlockCount, info.isPremium ? 1 : 0, info.ownerid);
		CDBManager::instance().AsyncQuery(query.c_str());

		TPacketDGNewIkarusShop pack{};
		pack.bSubHeader = ikashop::SUBHEADER_DG_SHOP_LOCKED_SLOT_STATE;

		ikashop::TSubPacketDGLockedSlotState subpack{};
		subpack.owner = info.ownerid;
		subpack.unlockCount = info.unlockCount;
		subpack.isPremium = info.isPremium;

		for (auto& peer : m_peerList)
		{
			if (peer->GetChannel() != 0)
			{
				peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
				peer->Encode(pack);
				peer->Encode(subpack);
			}
		}
		return true;
	}
	return false;
}

bool CClientManager::RecvIkarusShopLoadExpiredShop(const char* data)
{
	const auto& info = Decode<ikashop::TSubPacketGDLoadExpiredShop>(data);

	auto cache = m_offlineshopShopCache.Get(info.owner);
	if (!cache)
		return false;

	InitializeIkarusShopShopItemsTable(info.owner);

	TPacketDGNewIkarusShop pack{};
	pack.bSubHeader = ikashop::SUBHEADER_DG_LOAD_EXPIRED_SHOP;

	ikashop::TSubPacketDGLoadExpiredShop subpack{};
	subpack.owner = info.owner;
	subpack.itemCount = cache->itemsmap.size();

	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0)
		{
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack) + sizeof(ikashop::TShopItem) * subpack.itemCount);
			peer->Encode(pack);
			peer->Encode(subpack);

			for (auto& [id, item] : cache->itemsmap)
				peer->Encode(static_cast<ikashop::TShopItem*>(item.get()), sizeof(ikashop::TShopItem));
		}
	}
	return true;
}


bool CClientManager::SendIkarusShopBuyLockedItemPacket(CPeer* peer, DWORD dwOwner, DWORD dwGuest, DWORD dwItem)
{
	TPacketDGNewIkarusShop pack;
	pack.bSubHeader	= ikashop::SUBHEADER_DG_LOCKED_BUY_ITEM;

	ikashop::TSubPacketDGLockedBuyItem subpack;
	subpack.ownerid	= dwOwner;
	subpack.buyerid	= dwGuest;
	subpack.itemid	= dwItem;

	peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack)+sizeof(subpack));
	peer->Encode(&pack, sizeof(pack));
	peer->Encode(&subpack, sizeof(subpack));

	
	return true;
}

bool CClientManager::SendIkarusShopBuyItemPacket(DWORD dwOwner, DWORD dwGuest, DWORD dwItem, CPeer* requester)
{
	TPacketDGNewIkarusShop pack;
	pack.bSubHeader	= ikashop::SUBHEADER_DG_BUY_ITEM;

	ikashop::TSubPacketDGBuyItem subpack;
	subpack.ownerid	= dwOwner;
	subpack.buyerid	= dwGuest;
	subpack.itemid	= dwItem;

	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			subpack.requester = requester == peer;

			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}

	
	return true;
}

bool CClientManager::SendIkarusShopEditItemPacket(DWORD dwOwner, DWORD dwItem, const ikashop::TPriceInfo& price)
{
	TPacketDGNewIkarusShop pack;
	pack.bSubHeader	= ikashop::SUBHEADER_DG_EDIT_ITEM;

	ikashop::TSubPacketDGEditItem subpack;
	subpack.ownerid = dwOwner;
	subpack.itemid = dwItem;
	subpack.price = price;

	

	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}

	return true;
}

bool CClientManager::SendIkarusShopRemoveItemPacket(DWORD dwOwner, DWORD dwItem, CPeer* requester)
{
	TPacketDGNewIkarusShop pack;
	pack.bSubHeader	= ikashop::SUBHEADER_DG_REMOVE_ITEM;

	ikashop::TSubPacketDGRemoveItem subpack;
	subpack.ownerid	= dwOwner;
	subpack.itemid	= dwItem;

	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			// updating requester flag
			subpack.requester = requester == peer;

			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}

	return true;
}

bool CClientManager::SendIkarusShopAddItemPacket(DWORD dwOwner, DWORD dwItemID, const ikashop::TShopItem& rInfo)
{
	TPacketDGNewIkarusShop pack;
	pack.bSubHeader	= ikashop::SUBHEADER_DG_ADD_ITEM;

	ikashop::TSubPacketDGAddItem subpack;
	subpack.ownerid = dwOwner;
	subpack.itemid = dwItemID;
	subpack.item = rInfo;

	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}

	return true;
}

bool CClientManager::SendIkarusShopForceClose(DWORD dwOwnerID)
{
	TPacketDGNewIkarusShop pack{};
	pack.bSubHeader	= ikashop::SUBHEADER_DG_SHOP_FORCE_CLOSE;

	ikashop::TSubPacketDGShopForceClose subpack{};
	subpack.ownerid	= dwOwnerID;

	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}

	return true;
}

bool CClientManager::SendIkarusShopCreateNew(const ikashop::TShopInfo& shop, const ikashop::TShopItem* items)
{
	TPacketDGNewIkarusShop pack{};
	pack.bSubHeader	= ikashop::SUBHEADER_DG_SHOP_CREATE_NEW;

	ikashop::TSubPacketDGShopCreateNew subpack{};
	subpack.shop = shop;
	std::memcpy(subpack.items, items, sizeof(ikashop::TShopItem) * SHOP_PLAYER_HOST_ITEM_MAX_NUM);

	for (auto& peer : m_peerList)
	{
		if(peer->GetChannel() != 0){
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}

	return true;
}

bool CClientManager::SendIkarusShopChangeName(DWORD dwOwnerID, const char* szName)
{
	TPacketDGNewIkarusShop pack;
	pack.bSubHeader	= ikashop::SUBHEADER_DG_SHOP_CHANGE_NAME;

	ikashop::TSubPacketDGShopChangeName subpack;
	subpack.ownerid	= dwOwnerID;
	strncpy(subpack.name, szName, sizeof(subpack.name));


	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack) );
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}

	return true;
}

bool CClientManager::SendIkarusShopShopExpired(DWORD dwOwnerID)
{
	TPacketDGNewIkarusShop pack;
	pack.bSubHeader	= ikashop::SUBHEADER_DG_SHOP_EXPIRED;

	ikashop::TSubPacketDGShopExpired subpack;
	subpack.ownerid	= dwOwnerID;

	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack) );
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}

	return true;
}

bool CClientManager::SendIkarusShopOfferCreate(const ikashop::TOfferInfo& offer)
{
	TPacketDGNewIkarusShop pack;
	pack.bSubHeader	= ikashop::SUBHEADER_DG_OFFER_CREATE;

	ikashop::TSubPacketDGOfferCreate subpack;
	subpack.itemid = offer.itemid;
	subpack.ownerid = offer.ownerid;
	subpack.offer = offer;


	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}

	return true;
}

bool CClientManager::SendIkarusShopOfferNotified(DWORD dwOwnerID, DWORD dwOfferID)
{
	TPacketDGNewIkarusShop pack;
	pack.bSubHeader	= ikashop::SUBHEADER_DG_OFFER_NOTIFIED;

	ikashop::TSubPacketDGOfferNotified subpack;
	subpack.offerid = dwOfferID;
	subpack.ownerid = dwOwnerID;


	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}

	return true;
}

bool CClientManager::SendIkarusShopOfferAccepted(DWORD dwOwnerID, DWORD dwOfferID)
{
	TPacketDGNewIkarusShop pack;
	pack.bSubHeader	= ikashop::SUBHEADER_DG_OFFER_ACCEPT;

	ikashop::TSubPacketDGOfferAccept subpack;
	subpack.offerid = dwOfferID;
	subpack.ownerid = dwOwnerID;

	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}

	return true;
}

//ikashop-updated 05/08/19
void CClientManager::SendIkarusShopOfferCancel(DWORD dwOwnerID, DWORD dwOfferID, bool isRemovingItem)
{
	TPacketDGNewIkarusShop pack;
	pack.bSubHeader	= ikashop::SUBHEADER_DG_OFFER_CANCEL;

	ikashop::TSubPacketDGOfferCancel subpack;
	subpack.offerid = dwOfferID;
	subpack.ownerid = dwOwnerID;
	subpack.removing = isRemovingItem;


	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}

	
}

void CClientManager::SendIkarusShopSafeboxAddItem(DWORD dwOwnerID, DWORD dwItem, const ikashop::TShopPlayerItem& item)
{
	TPacketDGNewIkarusShop pack{};
	pack.bSubHeader	= ikashop::SUBHEADER_DG_SAFEBOX_ADD_ITEM;

	ikashop::TSubPacketDGSafeboxAddItem subpack{};
	subpack.ownerid = dwOwnerID;
	subpack.itemid = dwItem;
	subpack.item = item;

	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}
}

void CClientManager::SendIkarusShopSafeboxAddValutes(DWORD dwOwnerID, const ikashop::TValutesInfo& valute)
{
	TPacketDGNewIkarusShop pack;
	pack.bSubHeader	= ikashop::SUBHEADER_DG_SAFEBOX_ADD_VALUTES;

	ikashop::TSubPacketDGSafeboxAddValutes subpack;
	subpack.ownerid = dwOwnerID;
	subpack.valute = valute;

	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}
}

//patch 08-03-2020
void CClientManager::SendIkarusShopSafeboxExpiredItem(DWORD dwOwnerID, DWORD itemID) {
	TPacketDGNewIkarusShop pack;
	pack.bSubHeader = ikashop::SUBHEADER_DG_SAFEBOX_EXPIRED_ITEM;

	ikashop::TSubPacketDGSafeboxExpiredItem subpack;
	subpack.ownerid = dwOwnerID;
	subpack.itemid = itemID;

	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}
}

void CClientManager::SendIkarusShopSafeboxGetItemConfirm(CPeer* peer, DWORD dwOwnerID, DWORD itemID)
{
	TPacketDGNewIkarusShop pack{};
	pack.bSubHeader = ikashop::SUBHEADER_DG_SAFEBOX_GET_ITEM_CONFIRM;

	ikashop::TSubPacketDGSafeboxGetItemConfirm subpack{};
	subpack.ownerid = dwOwnerID;
	subpack.itemid = itemID;

	peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
	peer->Encode(&pack, sizeof(pack));
	peer->Encode(&subpack, sizeof(subpack));
}

//AUCTION
void CClientManager::SendIkarusShopAuctionCreate(const ikashop::TAuctionInfo& auction)
{
	

	TPacketDGNewIkarusShop pack;
	pack.bSubHeader	= ikashop::SUBHEADER_DG_AUCTION_CREATE;

	ikashop::TSubPacketDGAuctionCreate subpack;
	subpack.auction = auction;

	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}
}

void CClientManager::SendIkarusShopAuctionAddOffer(const ikashop::TAuctionOfferInfo& offer)
{
	TPacketDGNewIkarusShop pack;
	pack.bSubHeader	= ikashop::SUBHEADER_DG_AUCTION_ADD_OFFER;

	ikashop::TSubPacketDGAuctionAddOffer subpack;
	subpack.offer = offer;

	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}
}

void CClientManager::SendIkarusShopAuctionExpired(DWORD dwOwnerID)
{
	TPacketDGNewIkarusShop pack;
	pack.bSubHeader	= ikashop::SUBHEADER_DG_AUCTION_EXPIRED;

	ikashop::TSubPacketDGAuctionExpired subpack;
	subpack.ownerid = dwOwnerID;

	for (auto& peer : m_peerList)
	{
		if (peer->GetChannel() != 0) {
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(&pack, sizeof(pack));
			peer->Encode(&subpack, sizeof(subpack));
		}
	}
}

void CClientManager::IkarusShopResultAddItemQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo, bool sendResponse)
{
	auto* qi = static_cast<ikashop::SQueryInfoAddItem*>(pQueryInfo->pvData);
	m_offlineshopShopCache.PutItem(qi->owner, qi->id, *qi);
	if (sendResponse)
		SendIkarusShopAddItemPacket(qi->owner, qi->id, *qi);
	delete(qi);
}

void CClientManager::IkarusShopResultCreateShopQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo)
{
	// trying to insert shop
	auto* qi = static_cast<ikashop::SQueryInfoCreateShop*>(pQueryInfo->pvData);
	m_offlineshopShopCache.PutShop(qi->owner, qi->duration, qi->name.c_str(), qi->ownerName.c_str()
#ifdef ENABLE_IKASHOP_ENTITIES
		, qi->spawn
#endif
		, qi->unlockCount, qi->isPremium
	);

	ikashop::TShopInfo shopInfo{};
	shopInfo.ownerid = qi->owner;
	shopInfo.duration = qi->duration;
	shopInfo.unlockCount = qi->unlockCount;
	shopInfo.isPremium = qi->isPremium;
	str_to_cstring(shopInfo.name, qi->name.c_str());
	str_to_cstring(shopInfo.ownerName, qi->ownerName.c_str());

#ifdef ENABLE_IKASHOP_ENTITIES
	shopInfo.spawn = qi->spawn;
#endif
	ikashop::TShopItem items[SHOP_PLAYER_HOST_ITEM_MAX_NUM];
	std::memcpy(items, qi->items, sizeof(qi->items));

	// create shop with items
	for (int i = 0; i < SHOP_PLAYER_HOST_ITEM_MAX_NUM; i++)
	{
		if (qi->items[i].id < 1)
			break;

		ikashop::CShopCache::TShopCacheItemInfo item{ qi->items[i] };
		item.lock = false;

#ifdef ENABLE_IKASHOP_LOGS
		IkarusShopLog(qi->owner, item.id, "CREATE_SHOP_ADD_ITEM", qi->owner, "", item.vnum, item.count, item.price.yang
#ifdef ENABLE_CHEQUE_SYSTEM
			, item.price.cheque
#endif
		);
#endif

		m_offlineshopShopCache.AddItem(item, QID_OFFLINESHOP_CREATE_SHOP_ADD_ITEM);
	}

	SendIkarusShopCreateNew(shopInfo, items);
	
	delete(qi);
}

void CClientManager::IkarusShopResultOfferAddQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo)
{
	auto* qi = static_cast<ikashop::SQueryInfoOfferAdd *>(pQueryInfo->pvData);
	qi->offerid = msg->Get()->uiInsertID;
	m_offlineshopOfferCache.Puts(*qi);
	SendIkarusShopOfferCreate(*qi);
	delete(qi);
}

void CClientManager::IkarusShopResultSafeboxAddItemQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo)
{
	auto* qi = static_cast<ikashop::SQueryInfoSafeboxAddItem *>(pQueryInfo->pvData);
	m_offlineshopSafeboxCache.PutItem(qi->owner , qi->id, *qi);
	SendIkarusShopSafeboxAddItem(qi->owner, qi->id, *qi);
	delete(qi);
}

void CClientManager::IkarusShopResultQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo)
{
	

	switch (pQueryInfo->iType)
	{
	case QID_OFFLINESHOP_ADD_ITEM:
		IkarusShopResultAddItemQuery(peer, msg, pQueryInfo);
		break;

	case QID_OFFLINESHOP_CREATE_SHOP_ADD_ITEM:
		IkarusShopResultAddItemQuery(peer, msg, pQueryInfo, false);
		break;

	case QID_OFFLINESHOP_CREATE_SHOP:
		IkarusShopResultCreateShopQuery(peer, msg, pQueryInfo);
		break;

	case QID_OFFLINESHOP_SAFEBOX_ADD_ITEM:
		IkarusShopResultSafeboxAddItemQuery(peer, msg, pQueryInfo);
		break;

	case QID_OFFLINESHOP_OFFER_ADD:
		IkarusShopResultOfferAddQuery(peer, msg, pQueryInfo);
		break;

	case QID_OFFLINESHOP_EDIT_ITEM:
	case QID_OFFLINESHOP_REMOVE_ITEM:
	case QID_OFFLINESHOP_DELETE_SHOP:
	case QID_OFFLINESHOP_DELETE_SHOP_ITEM:
	case QID_OFFLINESHOP_SHOP_CHANGE_NAME:
	case QID_OFFLINESHOP_SAFEBOX_DELETE_ITEM:
	case QID_OFFLINESHOP_SAFEBOX_UPDATE_VALUTES:
	case QID_OFFLINESHOP_SAFEBOX_INSERT_VALUTES:
	case QID_OFFLINESHOP_SAFEBOX_UPDATE_VALUTES_ADDING:
	case QID_OFFLINESHOP_OFFER_UPDATE_NOTIFIED:
	case QID_OFFLINESHOP_OFFER_UPDATE_ACCEPTED:
	case QID_OFFLINESHOP_OFFER_DELETE_BY_SHOP:
	case QID_OFFLINESHOP_UPDATE_SOLD_ITEM:
	case QID_OFFLINESHOP_SHOP_UPDATE_DURATION:
	case QID_OFFLINESHOP_AUCTION_INSERT:
	case QID_OFFLINESHOP_AUCTION_INSERT_OFFER:
	case QID_OFFLINESHOP_AUCTION_DELETE:
	case QID_OFFLINESHOP_AUCTION_DELETE_OFFERS:
	case QID_OFFLINESHOP_AUCTION_UPDATE_DURATION:
		break;

	default:
		sys_err("Unknown offshop query type %d ",pQueryInfo->iType);
		break;

	}
}

void CClientManager::IkarusShopDurationProcess()
{
	m_offlineshopShopCache.ShopDurationProcess();
	m_offlineshopAuctionCache.AuctionDurationProcess();
	//patch 08-03-2020
	m_offlineshopSafeboxCache.ItemExpirationProcess();
}

void CClientManager::IkarusShopExpiredShop(DWORD dwID
)
{
	auto cache = m_offlineshopShopCache.Get(dwID);
	if(!cache)
		return;

	// removing offers
	RemoveOfferOnShop(dwID);

#ifdef EXTEND_IKASHOP_PRO
	{
		AppendIkarusShopNotification(ikashop::ENotificationType::SELLER_SHOP_EXPIRED, dwID, 0, "", "");
	}
#endif

#ifdef EXTEND_IKASHOP_PRO
	cache->duration = 0;
	m_offlineshopShopCache.UpdateDurationQuery(dwID, *cache);
#else
	// caching item map
	auto items { cache->itemsmap };
	// closing the shop
	m_offlineshopShopCache.CloseShop(dwID);
	// inserting into safebox
	for (auto& [id, item] : items)
		m_offlineshopSafeboxCache.AddItem(dwID, id, *item);
#endif
	m_offlineshopOfferCache.RemoveOffersByShopOwner(dwID);

	SendIkarusShopShopExpired(dwID);
}

void CClientManager::IkarusShopExpiredAuction(DWORD ownerid)
{
	//searching auction into cache
	auto auction = m_offlineshopAuctionCache.Get(ownerid);
	if(!auction)
		return;

	//get best offer info
	const auto winoffer = m_offlineshopAuctionCache.GetBestBuyerOffer(ownerid);
	const auto winnerid = winoffer ? winoffer->buyerid : ownerid;

	//if best offer exists insert money in safebox
	if (winoffer)
	{
		ikashop::TValutesInfo valute;
		valute.yang = winoffer->price.GetTotalYangAmount();

		m_offlineshopSafeboxCache.AddValutes(ownerid, valute);
		SendIkarusShopSafeboxAddValutes(ownerid, valute);

#ifdef ENABLE_IKASHOP_LOGS
		IkarusShopLog(winoffer->buyerid, auction->id, "AUCTION_BUY", auction->owner, "", auction->vnum, auction->count, winoffer->price.GetTotalYangAmount());
#endif

#ifdef EXTEND_IKASHOP_PRO
		AppendIkarusShopNotification(ikashop::ENotificationType::SELLER_AUCTION_SOLD, ownerid, auction->vnum, winoffer->buyername, fmt::format("{}", winoffer->price.yang));
		
		std::set<DWORD> notifiedIds;
		AppendIkarusShopNotification(ikashop::ENotificationType::BUYER_AUCTION_BUY, winoffer->buyerid, auction->vnum, "", fmt::format("{}", winoffer->price.yang));
		notifiedIds.emplace(winoffer->buyerid);

		for(auto& offer : m_offlineshopAuctionCache.GetOffers(ownerid))
		{
			if(!notifiedIds.contains(offer->buyerid))
			{
				notifiedIds.emplace(offer->buyerid);
				AppendIkarusShopNotification(ikashop::ENotificationType::BUYER_AUCTION_LOST, offer->buyerid, auction->vnum, winoffer->buyername, fmt::format("{}", winoffer->price.yang));
			}
		}
#endif
	}

#ifdef ENABLE_OFFLINESHOP_DEBUG
	else
		
#endif

#ifdef EXTEND_IKASHOP_PRO
	if(!winoffer)
		AppendIkarusShopNotification(ikashop::ENotificationType::SELLER_AUCTION_EXPIRED, ownerid, auction->vnum, "", "");
#endif

#ifdef ENABLE_IKASHOP_LOGS
	if (!winoffer)
		IkarusShopLog(auction->owner, auction->id, "AUCTION_BACK_TO_OWNER", auction->owner, "", auction->vnum, auction->count);
#endif

	m_offlineshopAuctionCache.ExpiredAuction(ownerid);
	SendIkarusShopAuctionExpired(ownerid);

	//insert item into safebox
	m_offlineshopSafeboxCache.AddItem(winnerid, auction->id, *auction);
}

void CClientManager::IkarusShopLoadShopSafebox(CPeer* peer, DWORD dwID)
{
	// getting safebox
	auto safebox = m_offlineshopSafeboxCache.LoadSafebox(dwID);
	if (!safebox)
	{
		sys_err("cannot load shop safebox for pid %d ",dwID);
		return;
	}

#ifdef ENABLE_IKASHOP_LOGS
	//IkarusShopLog(dwID, 0, "SHOP_SAFEBOX_LOAD", 0, fmt::format("ITEM_COUNT: {}", safebox->itemsmap.size()));
#endif

	TPacketDGNewIkarusShop pack{};
	pack.bSubHeader = ikashop::SUBHEADER_DG_SAFEBOX_LOAD;

	ikashop::TSubPacketDGSafeboxLoad subpack;
	subpack.ownerid = dwID;
	subpack.itemcount = safebox->itemsmap.size();
	subpack.valute = safebox->valutes;

	const auto packetSize = sizeof(pack) + sizeof(subpack) + sizeof(ikashop::TShopPlayerItem) * safebox->itemsmap.size();
	peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, packetSize);
	peer->Encode(&pack, sizeof(pack));
	peer->Encode(&subpack, sizeof(subpack));

	for (auto& [id, item] : safebox->itemsmap)
		peer->Encode(&item, sizeof(item));
}

#ifdef EXTEND_IKASHOP_PRO
void CClientManager::IkarusShopLoadNotifications(CPeer* peer, DWORD owner)
{
	auto query = fmt::format("SELECT `id`, `type`, `who`, `what`, `format`, `seen`, UNIX_TIMESTAMP(`datetime`) FROM `ikashop_notification` WHERE `owner` = {} ORDER BY `datetime` ASC LIMIT 200", owner);
	auto msg = CDBManager::instance().DirectQuery(query.c_str());

	if(!msg || !msg->Get() || msg->uiSQLErrno != 0)
	{
		sys_err("CANNOT LOAD NOTIFICATION FOR OWNER %u, errorno %d", owner, (int) msg->uiSQLErrno);
		return;
	}

	std::vector<ikashop::TNotificationInfo> notifications;
	for(uint32_t i=0; i < msg->Get()->uiNumRows; i++)
	{
		if(auto row = mysql_fetch_row(msg->Get()->pSQLResult))
		{
			int col = 0;

			auto& notification = notifications.emplace_back();
			str_to_number(notification.id, row[col++]);
			str_to_enum(notification.type, row[col++]);
			str_to_cstring(notification.who, row[col++]);
			str_to_number(notification.what, row[col++]);
			str_to_cstring(notification.format, row[col++]);
			str_to_number(notification.seenflag, row[col++]);
			str_to_number(notification.datetime, row[col++]);
		}
	}

	if(!notifications.empty())
	{
		TPacketDGNewIkarusShop pack{};
		pack.bSubHeader = ikashop::SUBHEADER_DG_NOTIFICATION_LOAD;

		ikashop::TSubPacketDGNotificationLoad subpack{};
		subpack.ownerid = owner;
		subpack.count = notifications.size();

		peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack) + sizeof(ikashop::TNotificationInfo) * notifications.size());
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&subpack, sizeof(subpack));
		peer->Encode(notifications.data(), sizeof(ikashop::TNotificationInfo) * notifications.size());
	}
}

void CClientManager::AppendIkarusShopNotification(ikashop::ENotificationType type, DWORD owner, DWORD what, const std::string& who, const std::string& format)
{
	// escaping who
	static char escapeWho[500];
	CDBManager::instance().EscapeString(escapeWho, who.c_str(), who.size());
	// escaping format
	static char escapeFormat[500];
	CDBManager::instance().EscapeString(escapeFormat, format.c_str(), format.size());
	// making query
	auto query = fmt::format("INSERT INTO `ikashop_notification` (`owner`, `type`, `who`, `what`, `format`, `seen`) VALUES({}, {}, '{}', {}, '{}', {})",
								owner, (int)type, escapeWho, what, escapeFormat, 0);
	// executing query
	auto msg = CDBManager::instance().DirectQuery(query.c_str());
	if(!msg || !msg->Get() || msg->uiSQLErrno != 0)
	{
		sys_err("CANNOT INSERT NOTIFICATION INTO TABLE, SKIPPING: Owner (%u)  Type (%d)", owner, (int)type);
		return;
	}
	
	// forwarding notification
	TPacketDGNewIkarusShop pack{ ikashop::SUBHEADER_DG_NOTIFICATION_FORWARD };

	ikashop::TSubPacketDGNotificationForward subpack{};
	subpack.datetime = static_cast<int64_t>(std::time(nullptr));
	subpack.id = msg->Get()->uiInsertID;
	subpack.seenflag = false;
	subpack.type = type;
	subpack.what = what;
	subpack.ownerid = owner;
	
	str_to_cstring(subpack.format, format.c_str());
	str_to_cstring(subpack.who, who.c_str());

	for(auto& peer : m_peerList)
	{
		if(peer->GetChannel() != 0)
		{
			peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
			peer->Encode(pack);
			peer->Encode(subpack);
		}
	}
}
#endif

//patch 08-03-2020
void CClientManager::IkarusShopExpiredAuctionItem(DWORD dwOwnerID) {
	// searching auction into cache
	auto auction = m_offlineshopAuctionCache.Get(dwOwnerID);
	if(!auction)
		return;

	// giving money back to buyer
	if (auto winneroffer = m_offlineshopAuctionCache.GetBestBuyerOffer(dwOwnerID)){
		ikashop::TValutesInfo valute{};
		valute.yang = winneroffer->price.GetTotalYangAmount();		

		m_offlineshopSafeboxCache.AddValutes(winneroffer->buyerid, valute);
		SendIkarusShopSafeboxAddValutes(winneroffer->buyerid, valute);

#ifdef ENABLE_IKASHOP_LOGS
		IkarusShopLog(winneroffer->buyerid, auction->id, "AUCTION_REFUND_EXPIRED_ITEM", auction->owner, "", auction->vnum, auction->count, winneroffer->price.GetTotalYangAmount());
#endif
	}

#ifdef ENABLE_OFFLINESHOP_DEBUG
	else
		
#endif

#ifdef ENABLE_IKASHOP_LOGS
	IkarusShopLog(auction->owner, auction->id, "AUCTION_EXPIRED_ITEM", auction->owner, "", auction->vnum, auction->count);
#endif

	m_offlineshopAuctionCache.ExpiredAuction(dwOwnerID);
	SendIkarusShopAuctionExpired(dwOwnerID);
}

//updated 15-01-2020
bool CClientManager::IsUsingIkarusShopSystem(DWORD dwID)
{
	// searching shop
	if (m_offlineshopShopCache.Get(dwID))
		return true;

	// searching auction
	if(m_offlineshopAuctionCache.Get(dwID))
		return true;
	return false;
}

//patch offerlist loading fix
void CClientManager::RemoveOfferOnShopItem(DWORD dwOwnerID, DWORD dwItemID)
{
	for(auto& offer : m_offlineshopOfferCache.GetOffersByItemID(dwOwnerID, dwItemID))
	{
		if(offer->accepted)
			continue;

		ikashop::TValutesInfo valute;
		valute.yang = offer->price.GetTotalYangAmount();

		m_offlineshopSafeboxCache.AddValutes(offer->buyerid, valute);
		SendIkarusShopSafeboxAddValutes(offer->buyerid, valute);

#ifdef EXTEND_IKASHOP_PRO
		// getting the item
		if (auto item = m_offlineshopShopCache.GetItem(offer->ownerid, offer->itemid))
			AppendIkarusShopNotification(ikashop::ENotificationType::BUYER_OFFER_EXPIRED, offer->buyerid, item->vnum, "", fmt::format("{}", offer->price.yang));
#endif

		SendIkarusShopOfferCancel(offer->ownerid, offer->offerid, true);//ikashop-updated 05/08/19
		m_offlineshopOfferCache.CancelOffer(offer->offerid, offer->ownerid);
	}
}

void CClientManager::RemoveOfferOnShop(DWORD dwOwnerID)
{
	for(auto& offer : m_offlineshopOfferCache.GetOffersByShopOwner(dwOwnerID))
	{
		if (offer->accepted)
			continue;

		ikashop::TValutesInfo valute;
		valute.yang = offer->price.GetTotalYangAmount();

		m_offlineshopSafeboxCache.AddValutes(offer->buyerid, valute);
		SendIkarusShopSafeboxAddValutes(offer->buyerid, valute);

#ifdef EXTEND_IKASHOP_PRO
		if (auto item = m_offlineshopShopCache.GetItem(offer->ownerid, offer->itemid))
			AppendIkarusShopNotification(ikashop::ENotificationType::BUYER_OFFER_EXPIRED, offer->buyerid, item->vnum, "", fmt::format("{}", offer->price.yang));
#endif

		SendIkarusShopOfferCancel(offer->ownerid, offer->offerid, true);//ikashop-updated 05/08/19
		m_offlineshopOfferCache.CancelOffer(offer->itemid, offer->ownerid);
	}
}

#ifdef ENABLE_IKASHOP_LOGS
void CClientManager::IkarusShopLog(DWORD who, DWORD itemid, std::string_view what, DWORD shopOwner, const std::string& extra, DWORD vnum, DWORD count, int64_t yang, int cheque)
{
	//* making query
	std::string query = "INSERT INTO `ikarusshop_log` (";
	query += "`who`, `itemid`, `what`, `shop_owner`, `extra`, `vnum`, `count`, `yang`";
#ifdef ENABLE_CHEQUE_SYSTEM
	query += ", `cheque`";
#endif
	query += ") ";

	query += fmt::format("VALUES({}, {}, '{}', {}, '{}', {}, {}, {}",
		who, itemid, what, shopOwner, extra, vnum, count, yang);
#ifdef ENABLE_CHEQUE_SYSTEM
	query += fmt::format(", {}", cheque);
#endif
	query += ");";

	CDBManager::instance().AsyncQuery(query.c_str(), SQL_LOG);
}
#endif

std::string CreateIkarusShopSelectShopItemsQuery(DWORD owner)
{
	std::string query = "SELECT `id`, `owner_id`, `pos`, `vnum`, `count`, ";
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
		query += fmt::format("`socket{}`, ", i);

	for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
		query += fmt::format("`attrtype{}`, `attrvalue{}`, ", i, i);

#ifdef ENABLE_CHANGELOOK_SYSTEM
	query += "`transmutation`, ";
#endif
	//patch 08-03-2020
	query += "`ikashop_data` FROM `item` WHERE `window` = 'IKASHOP_OFFLINESHOP' ";
	if (owner > 0)
	{
		query += fmt::format(" AND `owner_id`={} ", owner);
	}
	return query;
}

std::string CreateIkarusShopSelectAuctionsQuery()
{
	std::string query = "SELECT i.`id`, i.`vnum`, i.`count`, i.`owner_id`, ";
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
		query += fmt::format("i.`socket{}`, ", i);
	for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
		query += fmt::format("i.`attrtype{}`, i.`attrvalue{}`, ", i, i);
#ifdef ENABLE_CHANGELOOK_SYSTEM
	query += "i.`trans`, ";
#endif
	query += "i.`ikashop_data`, p.`name` FROM `item` i INNER JOIN `player` p ON i.`owner_id` = p.`id` WHERE `window` = 'IKASHOP_AUCTION'";
	return query;
}

#endif
// Files shared by GameCore.top
