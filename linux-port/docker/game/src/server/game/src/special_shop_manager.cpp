#include "stdafx.h"
#include "special_shop_manager.h"
#include "char.h"
#include "char_manager.h"
#include "packet.h"
#include "utils.h"
#include "desc.h"
#include "desc_client.h"
#include "item_manager.h"
#include "questmanager.h"
#include "item.h"
#include "log.h"
#include "buffer_manager.h"

CSpecialShopManager::CSpecialShopManager()
{
	m_map_SpecialShopGuestMap.clear();
}

CSpecialShopManager::~CSpecialShopManager()
{
}

bool CSpecialShopManager::InitializeItems(TSpecialShopItem* table, int size)
{
	m_map_SpecialShopItems.clear();
	for (int i = 0; i < size; ++i, ++table)
	{
		sys_log(0, "SPECIAL_SHOP ITEM vnum %d item_vnum %d count %d ", table->vnum, table->itemVnum, table->count);
		m_map_SpecialShopItems.insert(std::make_pair(table->vnum, *table));
	}

	sys_log(0, "SPECIAL_SHOP ITEMS: COUNT %d", m_map_SpecialShopItems.size());
	return true;
}

bool CSpecialShopManager::InitializeShops(TSpecialShopTable* table, int size)
{
	for (auto it : m_map_SpecialShopGuestMap)
		RemoveAllGuests(it.first);

	m_map_SpecialShopGuestMap.clear();
	m_map_SpecialShop.clear();
	m_map_SpecialShopMetaData.clear();

	for (int i = 0; i < size; ++i, ++table)
	{
		std::vector<DWORD> vec = {};
		for (auto item : table->items)
		{
			if (item > 0)
			{
				sys_log(0, "SPECIAL_SHOP ADD SHOP ITEM %d item %d ", table->vnum, item);
				vec.push_back(item);
			}
		}

		GUEST_SET set{};
		m_map_SpecialShopGuestMap.insert(std::make_pair(table->vnum, set));
		m_map_SpecialShop.insert(std::make_pair(table->vnum, vec));
	}

	sys_log(0, "SPECIAL_SHOP SHOPS: COUNT %d", m_map_SpecialShop.size());
	return true;
}

const TSpecialShopItem* CSpecialShopManager::GetSpecialShopItem(DWORD vnum)
{
	if (vnum == 0)
		return nullptr;

	itertype(m_map_SpecialShopItems) it = m_map_SpecialShopItems.find(vnum);

	if (it == m_map_SpecialShopItems.end())
	{
		return nullptr;
	}

	return &it->second;
}

CSpecialShopManager::TSpecialShopItemList CSpecialShopManager::GetSpecialShopItemList(DWORD shop_vnum)
{
	auto it = m_map_SpecialShop.find(shop_vnum);
	if (it != m_map_SpecialShop.end())
		return it->second;
	else
	{
		return {};
	}
}

std::vector<TRefineMaterial> CSpecialShopManager::GetRandomItems(DWORD seed, const TSpecialShopItem* item)
{
	std::vector<TRefineMaterial> items;
	items.clear();

	if (item->randomItemCount < 1 || item->randomItemGroup < 1)
		return items;

	auto specialItemGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(item->randomItemGroup);
	if (!specialItemGroup)
		return items;

	auto indices = generate_unique_numbers(item->randomItemCount, specialItemGroup->GetGroupSize(), seed);
	for (int i = 0; i < item->randomItemCount; i++)
	{
		if (specialItemGroup->IsSpecial(indices[i]))
			continue;

		auto reqItem = TRefineMaterial{};
		reqItem.vnum = specialItemGroup->GetVnum(indices[i]);
		reqItem.count = specialItemGroup->GetCount(indices[i]);
		items.emplace_back(reqItem);
	}
	return items;
}

TSpecialShopMetaData CSpecialShopManager::GetShopMetaData(DWORD shop_vnum)
{
	auto it = m_map_SpecialShopMetaData.find(shop_vnum);
	if (it == m_map_SpecialShopMetaData.end())
		return {};

	return it->second;
}

void CSpecialShopManager::SetShopSeed(DWORD shop_vnum, DWORD seed)
{
	auto meta = GetShopMetaData(shop_vnum);
	if (meta.seed == seed)
		return;

	meta.seed = seed;
	meta.lastUpdate = get_global_time();
	SetShopMeta(shop_vnum, meta, true);
}

void CSpecialShopManager::SetShopMeta(DWORD shop_vnum, TSpecialShopMetaData meta, bool send_db)
{
	sys_log(0, "specialshopVnum %d, meta seed: %d update: %d", shop_vnum, meta.seed, meta.lastUpdate);
	m_map_SpecialShopMetaData[shop_vnum] = meta;
	UpdateAllGuests(shop_vnum);
	if (send_db)
	{
		TPacketGDSpecialShopSeed p{};
		p.shopVnum = shop_vnum;
		p.meta = meta;

		db_clientdesc->DBPacket(HEADER_GD_SPECIAL_SHOP_SEED, 0, &p, sizeof(p));
	}
}

bool CSpecialShopManager::CheckShopOfferChange(DWORD shop_vnum)
{
	const auto meta = GetShopMetaData(shop_vnum);
	switch (shop_vnum)
	{
		case 9006: // STARSZA PANI
		{
			int delta_time = get_global_time() - meta.lastUpdate;
			if (delta_time > 3600 * 12)
			{
				SetShopSeed(shop_vnum, number(1, 9999999));
				return true;
			}
		}
		break;
		case 101: // DEVILTOWER
		{
			auto left_time = get_time_until(5, 0, meta.lastUpdate);
			if (left_time == 0)
			{
				SetShopSeed(shop_vnum, number(1, 9999999));
				return true;
			}
		}
		break;
		default:
			return false;
	}
	return false;
}

bool CSpecialShopManager::IsSpecialShopExist(DWORD shop_vnum)
{
	auto it = m_map_SpecialShop.find(shop_vnum);
	return it != m_map_SpecialShop.end();
}

bool CSpecialShopManager::IsItemInSpecialShop(DWORD shop_vnum, DWORD item_vnum)
{
	auto itemList = GetSpecialShopItemList(shop_vnum);
	auto it = std::find(itemList.begin(), itemList.end(), item_vnum);
	return it != itemList.end();
}

TSpecialShopLimit CSpecialShopManager::GetLimit(DWORD vnum, BYTE limitType)
{
	auto item = GetSpecialShopItem(vnum);
	if (!item)
		return {};

	for (auto limit : item->limits)
	{
		if (limit.type == limitType)
			return limit;
	}
	return {};
}

void CSpecialShopManager::SendShopInfo(LPCHARACTER ch, DWORD shop_vnum, LPCHARACTER seller)
{
	if (!ch || !ch->GetDesc())
		return;

	auto itemList =  GetSpecialShopItemList(shop_vnum);

	TPacketCGSpecialShop pack;
	pack.header = HEADER_GC_SPECIAL_SHOP;
	pack.subheader = SUBHEADER_GC_SPECIAL_SHOP_OPEN;
	pack.size = sizeof(TPacketCGSpecialShop) + sizeof(TSubPacketCGSpecialShopOpen) + itemList.size() * sizeof(TSpecialShopItemClient);

	TSubPacketCGSpecialShopOpen subpack;
	subpack.count = itemList.size();
	subpack.fraction = seller ? seller->GetFraction() : 0;

	TEMP_BUFFER buff;
	buff.write(&pack, sizeof(pack));
	buff.write(&subpack, sizeof(subpack));
	
	for (auto vnum : itemList)
	{
		auto item = GetSpecialShopItem(vnum);

		if (item)
		{
			TSpecialShopItemClient sendItem{};
			sendItem.vnum = item->vnum;
			sendItem.itemVnum = item->itemVnum;
			sendItem.count = item->count;
			sendItem.rarePct = item->rarePct;
			sendItem.priceType = item->priceType;
			sendItem.price = item->price;

			memcpy(&sendItem.items, item->items, sizeof(sendItem.items));

			if (item->randomItemCount > 0 && item->randomItemGroup > 0)
			{
				int last_items_index = 0;
				for (int i = 0; i < SPECIAL_SHOP_MAX_ITEM_COUNT; i++)
					if (sendItem.items[i].vnum == 0)
					{
						last_items_index = i;
						break;
					}

				const auto seed = GetShopMetaData(shop_vnum).seed + item->vnum;
				auto randomItems = GetRandomItems(seed, item);
				for (int i = 0; i < randomItems.size(); i++)
				{
					sendItem.items[last_items_index + i] = randomItems[i];
				}
			}

			memcpy(&sendItem.limits, item->limits, sizeof(sendItem.limits));
			ch->ChatDebug("sending itemvnum %d", sendItem.itemVnum);
			buff.write(&sendItem, sizeof(TSpecialShopItemClient));
		}
	}

	ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	ch->ChatDebug("special shop (vnum %d) %d items sent", shop_vnum, itemList.size());
}

void CSpecialShopManager::SendBuyInfo(LPCHARACTER ch, DWORD vnum)
{
	if (!ch || !ch->GetDesc())
		return;

	TPacketCGSpecialShop pack;
	pack.header = HEADER_GC_SPECIAL_SHOP;
	pack.subheader = SUBHEADER_GC_SPECIAL_SHOP_BUY;
	pack.size = sizeof(TPacketCGSpecialShop) + sizeof(TSubPacketCGSpecialShopBuy);

	TSubPacketCGSpecialShopBuy subpack;
	subpack.vnum = vnum;

	TEMP_BUFFER buff;
	buff.write(&pack, sizeof(pack));
	buff.write(&subpack, sizeof(subpack));

	ch->GetDesc()->Packet(buff.read_peek(), buff.size());
}

void CSpecialShopManager::BuyItem(LPCHARACTER ch, DWORD vnum)
{
	if (!ch || !ch->HasPlayerData())
		return;

	if (quest::CQuestManager::instance().GetEventFlag("block_special_shop") > 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This feature is temporarily unavailable."));
		return;
	}

	if (ch->IsBusy(BUSY_SPECIAL_SHOP))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade in the shop while another window is open."));
		return;
	}

	if (!ch->playerData->IsSpecialShopGuest())
	{
		ch->ChatDebug("you are not a guest in the special shop");
		return;
	}

	auto shopGuest = ch->playerData->GetSpecialShopGuest();

	if (!IsItemInSpecialShop(shopGuest.shop_vnum, vnum))
	{
		ch->ChatDebug("this item is not available in this special shop");
		return;
	}

	LPCHARACTER seller = CHARACTER_MANAGER::instance().Find(shopGuest.vid);
	if (!seller)
		return;

	if (ch->DistanceTo(seller) > 1200)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are too far away."));
		
		return;
	}

	auto item = GetSpecialShopItem(vnum);
	if (!item)
		return;


	if (!ch->HasSlotForItem(item->itemVnum))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Not enough inventory space."));
		return;
	}

	if (CheckShopOfferChange(shopGuest.shop_vnum))
		return;

	// check limits
	for (int i = 0; i < SPECIAL_SHOP_MAX_LIMIT; i++)
	{
		auto limitType = item->limits[i].type;
		auto limitValue = item->limits[i].value;

		if (limitType == SPECIAL_SHOP_LIMIT_NONE)
			continue;

		if (limitType == SPECIAL_SHOP_LIMIT_LEVEL)
		{
			if (ch->GetLevel() < limitValue)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need a minimum level of %d to buy this item."), limitValue);
				return;
			}
		}
		else if (limitType == SPECIAL_SHOP_LIMIT_REPUTATION)
		{
			if (ch->GetReputation(seller->GetFraction()) < limitValue)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SPECIAL_SHOP_LIMIT_REPUTATION"));
				return;
			}
		}
		else if (limitType == SPECIAL_SHOP_LIMIT_BUY_PER_TIME)
		{
			std::string flagName = "special_shop_buy" + std::to_string(item->vnum);
			int delta = ch->GetSpecialFlag(flagName) - get_global_time();
			if (delta > 0)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SPECIAL_SHOP_TIME_DELAY"), seconds_to_smart_time(delta));
				return;
			}
		}
		else if (limitType == SPECIAL_SHOP_LIMIT_ACCOUNT)
		{
			std::string flagName = "special_shop_buy" + std::to_string(item->vnum);
			const int buyCount = ch->GetAccountFlag(flagName);
			if (buyCount >= limitValue)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SPECIAL_SHOP_ACCOUNT_LIMIT"));
				return;
			}
		}
	}

	// combine items into single vector
	auto randomItems = GetRandomItems(GetShopMetaData(shopGuest.shop_vnum).seed + item->vnum, item);
	std::vector<TRefineMaterial> items;
	for (int i = 0; i < SPECIAL_SHOP_MAX_ITEM_COUNT; i++)
	{
		auto reqItem = item->items[i];
		if (reqItem.vnum == 0)
			break;

		items.emplace_back(reqItem);
	}

	if (randomItems.size() > 0)
		items.insert(items.end(), randomItems.begin(), randomItems.end());

	// check required items
	bool hasRequiredItems = true;
	for (auto reqItem : items) {
		if (ch->CountSpecifyItem(reqItem.vnum) < reqItem.count)
		{
			hasRequiredItems = false;
			break;
		}
	}

	if (!hasRequiredItems)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need certain items to buy this item."));
		return;
	}

	// check price
	if (item->priceType == SPECIAL_SHOP_PRICE_GOLD)
	{
		if (ch->GetGold() < item->price)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have enough Yang."));
			return;
		}

		ch->ChangeGold(-item->price);
	}
	else if (item->priceType == SPECIAL_SHOP_PRICE_REPUTATION)
	{
		int playerReputation = ch->GetReputation(seller->GetFraction());
		if (playerReputation < item->price)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SPECIAL_SHOP_PRICE_NO_REPUTATION"));
			return;
		}

		auto reputationLimit = GetLimit(vnum, SPECIAL_SHOP_LIMIT_REPUTATION);
		if (reputationLimit.type != SPECIAL_SHOP_LIMIT_NONE)
		{
			int destRep = playerReputation - item->price;
			if (destRep < reputationLimit.value)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SPECIAL_SHOP_PRICE_NO_REPUTATION"));
				return;
			}
		}
		ch->SetReputation(seller->GetFraction(), playerReputation - item->price);
	}

	for (auto reqItem : items) {
		if (reqItem.vnum == 0)
			break;

		ch->RemoveSpecifyItem(reqItem.vnum, reqItem.count);
	}

	auto timeLimit = GetLimit(vnum, SPECIAL_SHOP_LIMIT_BUY_PER_TIME);
	if (timeLimit.type != SPECIAL_SHOP_LIMIT_NONE)
	{
		std::string flagName = "special_shop_buy" + std::to_string(item->vnum);
		ch->SetSpecialFlag(flagName, get_global_time() + timeLimit.value, false);
	}

	auto accountLimit = GetLimit(vnum, SPECIAL_SHOP_LIMIT_ACCOUNT);
	if (accountLimit.type != SPECIAL_SHOP_LIMIT_NONE)
	{
		std::string flagName = "special_shop_buy" + std::to_string(item->vnum);
		ch->SetAccountFlag(flagName, ch->GetAccountFlag(flagName) + 1, false);
	}

	int rarePct = item->rarePct > 0 ? item->rarePct : -1;
	auto pkItem = ch->AutoGiveItem(item->itemVnum, item->count, rarePct, true, true);
	ch->SetExchangeTime();
	SendBuyInfo(ch, item->vnum);

	if (pkItem)
	{
		auto meta = GetShopMetaData(shopGuest.shop_vnum);

		char buf[128 + 1];
		snprintf(buf, sizeof(buf), "%d %d %dx %s %d", shopGuest.shop_vnum, item->vnum, item->count, pkItem->GetName(), meta.seed);

		LogManager::instance().ItemLog(ch, pkItem, "SPECIAL_BUY", buf);
	}
}

void CSpecialShopManager::OpenShop(LPCHARACTER ch, LPCHARACTER seller, DWORD shop_vnum)
{
	if (!seller || !seller->IsNPC())
		return;

	if (!ch || !ch->HasPlayerData())
		return;

	if (ch->IsBusy())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade in the shop while another window is open."));
		return;
	}

	if (ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_WHILE_DEAD"));
		return;
	}

	if (!IsSpecialShopExist(shop_vnum))
	{
		ch->ChatDebug("shop vnum %d does not exist.", shop_vnum);
		return;
	}

	if (ch->playerData->IsSpecialShopGuest())
	{
		ch->ChatDebug("you are already a guest in some special shop");
		return;
	}

	CheckShopOfferChange(shop_vnum);

	ch->playerData->SetSpecialShopGuest(seller->GetVID(), shop_vnum);
	AddGuest(shop_vnum, ch->GetPlayerID());
	SendShopInfo(ch, shop_vnum, seller);
}

void CSpecialShopManager::CloseShop(LPCHARACTER ch)
{
	if (!ch || !ch->HasPlayerData())
		return;

	if (!ch->playerData->IsSpecialShopGuest())
		return;

	RemoveGuest(ch->playerData->GetSpecialShopGuest().shop_vnum, ch->GetPlayerID());
	ch->playerData->SetSpecialShopGuest(0, 0);
	ch->SetExchangeTime();
}

void CSpecialShopManager::AddGuest(DWORD shop_vnum, DWORD pid)
{
	auto& guests = GetGuestSet(shop_vnum);
	guests.insert(pid);
}

void CSpecialShopManager::UpdateAllGuests(DWORD shop_vnum)
{
	auto& guests = GetGuestSet(shop_vnum);
	for (auto it = guests.begin(); it != guests.end(); ) {
		auto pid = *it;

		auto ch = CHARACTER_MANAGER::instance().FindByPID(pid);
		if (ch && ch->HasPlayerData() && ch->playerData->IsSpecialShopGuest())
		{
			auto shop = ch->playerData->GetSpecialShopGuest();
			auto seller = CHARACTER_MANAGER::instance().Find(shop.vid);

			SendShopInfo(ch, shop_vnum, seller);
			ch->ChatPacket(CHAT_TYPE_COMMAND, "UpdateSpecialShop");
			++it;
		}
		else
		{
			it = guests.erase(it);
		}
	}
}

void CSpecialShopManager::RemoveAllGuests(DWORD shop_vnum)
{
	auto guests = GetGuestSet(shop_vnum);
	for (auto pid : guests) {
		auto ch = CHARACTER_MANAGER::instance().FindByPID(pid);
		if (ch && ch->HasPlayerData() && ch->playerData->IsSpecialShopGuest())
		{
			CloseShop(ch);
			ch->ChatPacket(CHAT_TYPE_COMMAND, "HideSpecialShop");
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SPECIAL_SHOP_OFFER_CHANGED"));
		}
	}
	m_map_SpecialShopGuestMap.clear();
}

void CSpecialShopManager::RemoveGuest(DWORD shop_vnum, DWORD pid)
{
	auto& guests = GetGuestSet(shop_vnum);
	guests.erase(pid);
}

bool CSpecialShopManager::IsPlayerGuest(DWORD shop_vnum, DWORD pid)
{
	auto guests = GetGuestSet(shop_vnum);
	return guests.find(pid) != guests.end();
}

CSpecialShopManager::GUEST_SET& CSpecialShopManager::GetGuestSet(DWORD shop_vnum)
{
	auto it = m_map_SpecialShopGuestMap.find(shop_vnum);
	if (it != m_map_SpecialShopGuestMap.end())
		return it->second;

	auto [newIt, inserted] = m_map_SpecialShopGuestMap.emplace(shop_vnum, CSpecialShopManager::GUEST_SET{});
	return newIt->second;
}

// Files shared by GameCore.top
