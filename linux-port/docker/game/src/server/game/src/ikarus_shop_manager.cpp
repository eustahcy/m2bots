#include "stdafx.h"
#include "../../common/tables.h"
#include "packet.h"
#include "item.h"
#include "char.h"
#include "item_manager.h"
#include "desc.h"
#include "char_manager.h"
#include "banword.h"
#include "buffer_manager.h"
#include "desc_client.h"
#include "config.h"
#include "event.h"
#include "locale_service.h"
#include <fstream>
#include "questmanager.h"
#include "sectree_manager.h"
#include "sectree.h"
#include "config.h"
#include "db.h"
#include "p2p.h"
#include "utils.h"
#include <msl/random.h>
#include "../../common/PulseManager.h"

#ifdef ENABLE_IKASHOP_RENEWAL

#include "ikarus_shop.h"
#include "ikarus_shop_manager.h"
#include "playerbot_offline_policy.h"
#include "shop.h"

template<class DerivedType, class BaseType>
DerivedType DerivedFromBase(const BaseType& base) {
	static_assert(std::is_base_of<BaseType, DerivedType>::value); // DerivedType must be inherited by BaseType
	DerivedType temporary{};
	static_cast<BaseType&>(temporary) = base;
	return temporary;
}


// playerbot: a GM plays the single-player world, and Ikarus serves it
// like anybody (playerbotify.py, apply_gm_gameplay).
static bool CheckGMLevel(LPCHARACTER ch)
{
	return ch != nullptr;
}

bool MatchAttributes(const TPlayerItemAttribute* pAttributesFilter,const TPlayerItemAttribute* pAttributesItem)
{
	static auto findAttribute = [](auto type, auto val, auto attr){
		for (int j = 0; j < ITEM_ATTRIBUTE_NORM_NUM; j++)
			if (attr[j].bType == type)
				return attr[j].sValue >= val;
		return false;
	};

	auto matchFilterAttribute = [&](auto filter){
		return filter.bType == 0 || findAttribute(filter.bType, filter.sValue, pAttributesItem);
	};

	return std::all_of(pAttributesFilter, pAttributesFilter + ITEM_ATTRIBUTE_NORM_NUM, matchFilterAttribute);
}

std::string StringToLower(const char* name, size_t len)
{
	// std::tolower handles ints so transform reject it as operand
	static auto lower = [](auto&& ch){ return std::tolower(ch); };
	// filling string inserting lower characters
	std::string res = {name, len};
	std::transform(res.begin(), res.end(), res.begin(), lower);
	return res;
}

bool IsGoodSalePrice(const ikashop::TPriceInfo price) {
	if (price.yang >= GOLD_MAX || price.yang < 0)
		return false;
#ifdef ENABLE_CHEQUE_SYSTEM
	else if (price.cheque >= CHEQUE_MAX || price.cheque < 0)
		return false;
#endif
	return true;
}

bool MatchItemName(std::string stName, const char* table , const size_t tablelen)
{
	const auto lower = StringToLower(table, tablelen);
	return lower.find(stName) != std::string::npos;
}

bool CheckCharacterActions(LPCHARACTER ch)
{
	if(!ch || !ch->CanHandleItem(false, false, BUSY_CAN_HANDLE_ITEM_EXCLUDE | BUSY_SHOP_MANAGE | BUSY_SHOP))
		return false;

	if(ch->GetExchange())
		return false;

	if(ch->GetSafebox())
		return false;

	if(ch->GetShop())
		return false;

#if defined(ENABLE_ACCE_SYSTEM) || defined(ENABLE_ACCE_COSTUME_SYSTEM) || defined(ENABLE_SASH_SYSTEM) || defined(__SASH_SYSTEM) || defined(ACCE_SYSTEM) || defined(__ACCE_SYSTEM__)
	if(ch->IsAcceOpened())
		return false;
#endif

	if(quest::CQuestManager::Instance().GetPCForce(ch->GetPlayerID())->IsRunning())
		return false;

	return true;
}

bool MatchFilterType(const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, TItemTable* table)
{
	if(filter.type == 0)
		return true;

	static auto MatchWeapon = [](const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, TItemTable* table) -> bool {
		if(table->bType != ITEM_WEAPON)
			return false;
		if(filter.subtype == 0)
			return true;
		switch(filter.subtype)
		{
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_SWORD:
				return table->bSubType == WEAPON_SWORD
					&& IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_SHAMAN) 
					&& !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_WARRIOR) 
					&& !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_ASSASSIN) 
					&& !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_SURA);
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_BLADE:
				return table->bSubType == WEAPON_SWORD 
					&& IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_SHAMAN)
					&& IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_WARRIOR)
					&& IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_ASSASSIN)
					&& !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_SURA);
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_TWOHANDED:
				return table->bSubType == WEAPON_TWO_HANDED;
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_DAGGER:
				return table->bSubType == WEAPON_DAGGER;
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_BOW:
				return table->bSubType == WEAPON_BOW;
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_BELL:
				return table->bSubType == WEAPON_BELL;
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_FAN:
				return table->bSubType == WEAPON_FAN;
#ifdef ENABLE_WOLFMAN_CHARACTER
			case ikashop::SEARCH_SHOP_SUBTYPE_WEAPON_CLAW:
				return table->bSubType == WEAPON_CLAW;
#endif
		}
		return false;
	};

	static auto MatchArmor = [](const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, TItemTable* table) {
		if(table->bType != ITEM_ARMOR || table->bSubType != ARMOR_BODY)
			return false;
		if(filter.subtype == 0)
			return true;
		switch(filter.subtype)
		{
			case ikashop::SEARCH_SHOP_SUBTYPE_ARMOR_WARRIOR:
				return !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_WARRIOR);
			case ikashop::SEARCH_SHOP_SUBTYPE_ARMOR_SURA:
				return !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_SURA);
			case ikashop::SEARCH_SHOP_SUBTYPE_ARMOR_ASSASSIN:
				return !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_ASSASSIN);
			case ikashop::SEARCH_SHOP_SUBTYPE_ARMOR_SHAMAN:
				return !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_SHAMAN);
#ifdef ENABLE_WOLFMAN_CHARACTER
			case ikashop::SEARCH_SHOP_SUBTYPE_ARMOR_WOLFMAN:
				return !IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_WOLFMAN);
#endif
		}
		return false;
	};

	static auto MatchJewel = [](const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, TItemTable* table) {
		if (table->bType != ITEM_ARMOR || table->bSubType == ARMOR_BODY)
			return false;
		if (filter.subtype == 0)
			return true;
		switch(filter.subtype)
		{
			case ikashop::SEARCH_SHOP_SUBTYPE_JEWEL_HELM:
				return table->bSubType == ARMOR_HEAD;
			case ikashop::SEARCH_SHOP_SUBTYPE_JEWEL_SHIELD:
				return table->bSubType == ARMOR_SHIELD;
			case ikashop::SEARCH_SHOP_SUBTYPE_JEWEL_EAR:
				return table->bSubType == ARMOR_EAR;
			case ikashop::SEARCH_SHOP_SUBTYPE_JEWEL_NECK:
				return table->bSubType == ARMOR_NECK;
			case ikashop::SEARCH_SHOP_SUBTYPE_JEWEL_WRIST:
				return table->bSubType == ARMOR_WRIST;
			case ikashop::SEARCH_SHOP_SUBTYPE_JEWEL_SHOES:
				return table->bSubType == ARMOR_FOOTS;
		}
		return false;
	};

	static auto MatchCostume = [](const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, TItemTable* table) {
		if(table->bType != ITEM_COSTUME)
			return false;
		if(filter.subtype == 0)
			return true;
		switch(filter.subtype)
		{
		case ikashop::SEARCH_SHOP_SUBTYPE_COSTUME_BODY:
			return table->bSubType == COSTUME_BODY;
		case ikashop::SEARCH_SHOP_SUBTYPE_COSTUME_WEAPON:
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
			return table->bSubType == COSTUME_WEAPON;
#else
			return false;
#endif
		case ikashop::SEARCH_SHOP_SUBTYPE_COSTUME_HAIR:
			return table->bSubType == COSTUME_HAIR;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		case ikashop::SEARCH_SHOP_SUBTYPE_COSTUME_SASH:
			return table->bSubType == COSTUME_ACCE;
#endif
		}
		return false;
	};

	static auto MatchPet = [](const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, TItemTable* table) {
		if(table->bType != ITEM_PET || (table->bSubType != PET_EGG && table->bSubType != PET_UPBRINGING && table->bSubType != PET_PAY))
			return false;
		if(filter.subtype == 0)
			return true;
		switch(filter.subtype)
		{
		case ikashop::SEARCH_SHOP_SUBTYPE_PET_EGGS:
			return table->bSubType == PET_EGG;
		case ikashop::SEARCH_SHOP_SUBTYPE_PET_SEALS:
			return table->bSubType == PET_UPBRINGING || table->bSubType == PET_PAY;
		}
		return false;
	};

	switch(filter.type)
	{
		case ikashop::SEARCH_SHOP_TYPE_WEAPON:
			return MatchWeapon(info, filter, table);
		case ikashop::SEARCH_SHOP_TYPE_ARMOR:
			return MatchArmor(info, filter, table);
		case ikashop::SEARCH_SHOP_TYPE_JEWEL:
			return MatchJewel(info, filter, table);
		case ikashop::SEARCH_SHOP_TYPE_COSTUME:
			return MatchCostume(info, filter, table);
		case ikashop::SEARCH_SHOP_TYPE_PET:
			return MatchPet(info, filter, table);
		case ikashop::SEARCH_SHOP_TYPE_ALCHEMY:
			return table->bType == ITEM_DS;
		case ikashop::SEARCH_SHOP_TYPE_BOOK:
			return table->bType == ITEM_SKILLBOOK;
		case ikashop::SEARCH_SHOP_TYPE_STONE:
			return table->bType == ITEM_METIN;
		case ikashop::SEARCH_SHOP_TYPE_FISH:
			return table->bType == ITEM_FISH;
		case ikashop::SEARCH_SHOP_TYPE_MINERALS:
			return table->bType == ITEM_USE && (table->bSubType == USE_PUT_INTO_ACCESSORY_SOCKET || table->bSubType == USE_PUT_INTO_BELT_SOCKET);
		case ikashop::SEARCH_SHOP_TYPE_MOUNT:
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
			return table->bType == ITEM_COSTUME && table->bSubType == COSTUME_MOUNT;
#else
			return false;
#endif
	}

	return false;
}

bool MatchSpecial(const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, TItemTable* table)
{
	if(table->bType == ITEM_DS)
	{
		if(filter.alchemyGrade != 0)
		{
			const auto grade = (table->dwVnum / 1000) % 10;
			return grade+1 >= filter.alchemyGrade;
		}
	}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if(table->bType == ITEM_COSTUME && table->bSubType == COSTUME_ACCE)
	{
		if(filter.sashGrade != 0)
		{
			switch(filter.sashGrade)
			{
			case ikashop::SEARCH_SHOP_SASH_GRADE_VALUE0:
				return info.alSockets[ACCE_ABSORPTION_SOCKET] >= 10;
			case ikashop::SEARCH_SHOP_SASH_GRADE_VALUE1:
				return info.alSockets[ACCE_ABSORPTION_SOCKET] >= 15;
			case ikashop::SEARCH_SHOP_SASH_GRADE_VALUE2:
				return info.alSockets[ACCE_ABSORPTION_SOCKET] >= 20;
			case ikashop::SEARCH_SHOP_SASH_GRADE_VALUE3:
				return info.alSockets[ACCE_ABSORPTION_SOCKET] == 25;
			}
			return false;
		}
	}
#endif

	return true;
}

bool MatchFilter(const ikashop::TShopItem& info, const ikashop::TFilterInfo& filter, const std::string& filtername)
{
	auto table = ITEM_MANAGER::instance().GetTable(info.vnum);
	if (!table)
	{
		sys_err("CANNOT FIND ITEM TABLE [%d]", info.vnum);
		return false;
	}

	// filter by type / subtype
	if(!MatchFilterType(info, filter, table))
		return false;

	// filter by level
	if ((filter.levelstart != 0 || filter.levelend != 0))
	{
		const auto level = table->aLimits[0].bType == LIMIT_LEVEL ?
			table->aLimits[0].lValue :
			table->aLimits[1].bType == LIMIT_LEVEL ?
			table->aLimits[1].lValue : 0;
		if (filter.levelstart != 0 && filter.levelstart > level)
			return false;
		if (filter.levelend != 0 && filter.levelend < level)
			return false;
	}

	// filter by price
	if (filter.pricestart.yang != 0 || filter.pricend.yang != 0)
	{
		const auto total = info.price.GetTotalYangAmount();
		if (filter.pricestart.yang != 0 && total < filter.pricestart.yang)
			return false;

		if (filter.pricend.yang != 0 && total > filter.pricend.yang)
			return false;
	}

	// filter by name
	if (!filtername.empty() && !MatchItemName(filtername, table->szLocaleName, strnlen(table->szLocaleName, ITEM_NAME_MAX_LEN)))
		return false;

	// filter by attrs
	if (!MatchAttributes(filter.attrs, info.aAttr))
		return false;

	// filter bu special
	if (!MatchSpecial(info, filter, table))
		return false;

	return true;
}

long long GetAuctionMinRaise(ikashop::CShopManager::AUCTION_HANDLE auction)
{
	auto bestoff = auction->GetBestOffer();
	if(!bestoff)
		return auction->GetInfo().price.GetTotalYangAmount();

	const auto minRaise = bestoff->price;
	const auto money = minRaise.GetTotalYangAmount();
	if(money < 1000)
		return money + 1000;

	const auto percentage = 1.0 + static_cast<double>(OFFLINESHOP_AUCTION_RAISE_PERCENTAGE) / 100.0;
	return static_cast<long long>(static_cast<double>(money) * percentage);
}

void MoveAndDestroyInstance(LPITEM item, BYTE window, WORD pos)
{
	auto ch = item->GetOwner();
	item->RemoveFromCharacter();
	item->SetWindow(window);
	item->SetCell(ch, pos);
	item->SetSkipSave(false);
	item->Save();
	ITEM_MANAGER::instance().FlushDelayedSave(item);
	item->SetSkipSave(true);
	M2_DESTROY_ITEM(item->RemoveFromCharacter());
}

bool CheckOfferPrice(const ikashop::TPriceInfo& offerPrice, const ikashop::TPriceInfo& itemPrice)
{
	const auto offer = offerPrice.GetTotalYangAmount();
	const auto item = itemPrice.GetTotalYangAmount();
	const auto minOffer = item * OFFLINESHOP_MIN_OFFER_PERCENTAGE / 100;
	return minOffer <= offer && offer < item;
}

namespace ikashop
{
	EVENTFUNC(func_offlineshop_update_duration)
	{
		auto& manager = ikashop::GetManager();
		manager.UpdateShopsDuration();
		manager.UpdateAuctionsDuration();
		manager.ClearSearchTimeMap();
		return OFFLINESHOP_DURATION_UPDATE_TIME;
	}

	ikashop::CShopManager& GetManager()
	{
		return ikashop::CShopManager::instance();
	}

	CShopManager::SHOP_HANDLE CShopManager::PutsNewShop(const TShopInfo& info)
	{
		auto& shop = m_mapShops[info.ownerid] = std::make_shared<ikashop::CShop>();
		shop->SetDuration(info.duration);
		shop->SetOwnerPID(info.ownerid);
		shop->SetName(info.name);
		shop->SetOwnerName(info.ownerName);
#ifdef ENABLE_IKASHOP_ENTITIES
		shop->SetSpawn(info.spawn);
		CreateShopEntity(shop);
#endif
		shop->SetPremium(info.isPremium);
		shop->SetUnlockCount(info.unlockCount);
		return shop;
	}

	void CShopManager::PutsAuction(const TAuctionInfo& auction)
	{
		auto obj = std::make_shared<CAuction>();
		obj->SetInfo(auction);
		m_mapAuctions[auction.owner] = obj;
	}

	void CShopManager::PutsAuctionOffer(const TAuctionOfferInfo& offer)
	{
		if(auto cache = GetAuctionByOwnerID(offer.ownerid))
			cache->AddOffer(offer);
	}

	CShopManager::SHOP_HANDLE CShopManager::GetShopByOwnerID(DWORD pid)
	{
		auto it = m_mapShops.find(pid);
		return it != m_mapShops.end() ? it->second : nullptr;
	}

	CShopManager::AUCTION_HANDLE CShopManager::GetAuctionByOwnerID(DWORD pid)
	{
		auto it = m_mapAuctions.find(pid);
		return it != m_mapAuctions.end() ? it->second : nullptr;
	}

	void CShopManager::PrepareShopSearchFilters()
	{
		m_shopSearchFilters.clear();

		// SHOP_SEARCH_CATEGORY_BOOKS
		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_BOOKS * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_WARRIOR_0] = {
			{50300, 1}, {50300, 2}, {50300, 3}, {50300, 4}, {50300, 5}, {50300, 6}, // body
			{50401, 0}, {50402, 0}, {50403, 0}, {50404, 0}, {50405, 0}, {50406, 0}, // instr body
			{70037, 1}, {70037, 2}, {70037, 3}, {70037, 4}, {70037, 5}, {70037, 6}, // oz body
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_BOOKS * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_WARRIOR_1] = {
			{50300, 16}, {50300, 17}, {50300, 18}, {50300, 19}, {50300, 20}, {50300, 21}, // mental
			{50416, 0}, {50417, 0}, {50418, 0}, {50419, 0}, {50420, 0}, {50421, 0}, // instr mental
			{70037, 16}, {70037, 17}, {70037, 18}, {70037, 19}, {70037, 20}, {70037, 21}, // oz mental
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_BOOKS * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_ASSASSIN_0] = {
			{50300, 31}, {50300, 32}, {50300, 33}, {50300, 34}, {50300, 35}, {50300, 36}, // dagger
			{50431, 0}, {50432, 0}, {50433, 0}, {50434, 0}, {50435, 0}, {50436, 0}, // instr dagger
			{70037, 31}, {70037, 32}, {70037, 33}, {70037, 34}, {70037, 35}, {70037, 36}, // oz dagger
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_BOOKS * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_ASSASSIN_1] = {
			{50300, 46}, {50300, 47}, {50300, 48}, {50300, 49}, {50300, 50}, {50300, 51}, // archer
			{50446, 0}, {50447, 0}, {50448, 0}, {50449, 0}, {50450, 0}, {50451, 0}, // instr archer
			{70037, 46}, {70037, 47}, {70037, 48}, {70037, 49}, {70037, 50}, {70037, 51}, // oz archer

		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_BOOKS * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SURA_0] = {
			{50300, 61}, {50300, 62}, {50300, 63}, {50300, 64}, {50300, 65}, {50300, 66}, // wp
			{50461, 0}, {50462, 0}, {50463, 0}, {50464, 0}, {50465, 0}, {50466, 0}, // instr wp
			{70037, 61}, {70037, 62}, {70037, 63}, {70037, 64}, {70037, 65}, {70037, 66}, // oz wp
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_BOOKS * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SURA_1] = {
			{50300, 76}, {50300, 77}, {50300, 78}, {50300, 79}, {50300, 80}, {50300, 81}, // bm
			{50476, 0}, {50477, 0}, {50478, 0}, {50479, 0}, {50480, 0}, {50481, 0}, // instr bm
			{70037, 76}, {70037, 77}, {70037, 78}, {70037, 79}, {70037, 80}, {70037, 81}, // oz bm
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_BOOKS * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SHAMAN_0] = {
			{50300, 91}, {50300, 92}, {50300, 93}, {50300, 94}, {50300, 95}, {50300, 96}, // smok
			{50491, 0}, {50492, 0}, {50493, 0}, {50494, 0}, {50495, 0}, {50496, 0}, // instr smok
			{70037, 91}, {70037, 92}, {70037, 93}, {70037, 94}, {70037, 95}, {70037, 96}, // oz smok
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_BOOKS * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SHAMAN_1] = {
			{50300, 106}, {50300, 107}, {50300, 108}, {50300, 109}, {50300, 110}, {50300, 111}, // healer
			{50506, 0}, {50507, 0}, {50508, 0}, {50509, 0}, {50510, 0}, {50511, 0}, // instr healer
			{70037, 106}, {70037, 107}, {70037, 108}, {70037, 109}, {70037, 110}, {70037, 111}, // oz healer
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_BOOKS * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_PASSIVE_SKILL] = {
			{50513, 0},{50600, 0},{50301, 0},{50302, 0},{50303, 0},{50304, 0},{50305, 0},{50306, 0},{50311, 0},{50312, 0},{50313, 0},{50314, 0},{50315, 0},{50316, 0},
		};

		// SHOP_SEARCH_CATEGORY_REFINE
		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_REFINE * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_REFINE_M1] = {
			{30003, 0},{30004, 0},{30010, 0},{30023, 0},{30027, 0},{30028, 0},{30037, 0},{30038, 0},{30053, 0},{30069, 0},{30070, 0},{30071, 0},{30072, 0},{30346, 0},{30347, 0},{30353, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_REFINE * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_REFINE_OATH] = {
			{30011, 0},{30017, 0},{30018, 0},{30031, 0},{30034, 0},{30035, 0},{30073, 0},{30356, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_REFINE * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_REFINE_M2] = {
			{30005, 0},{30021, 0},{30030, 0},{30032, 0},{30033, 0},{30041, 0},{30052, 0},{30074, 0},{30075, 0},{30092, 0},{30357, 0},{30358, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_REFINE * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_REFINE_ORC] = {
			{30007, 0},{30076, 0},{30352, 0},{30006, 0},{30077, 0},{30008, 0},{30078, 0},{30051, 0},{30079, 0},{30047, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_REFINE * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_REFINE_DESERT1] = {
			{30022, 0}, {30025, 0}, {30045, 0}, {30046, 0}, {30081, 0}, {30055, 0}, {30056, 0}, {30057, 0}, {30058, 0}, {30059, 0}, {30067, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_REFINE * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_REFINE_DESERT2] = {
			 {30350, 0}, {30351, 0}, {30359, 0}, {30082, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_REFINE * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_REFINE_SNOW] = {
			{30009, 0}, {30014, 0}, {30039, 0}, {30048, 0}, {30049, 0}, {30050, 0}, {30083, 0}, {30085, 0}, {30088, 0}, {30089, 0}, {30090, 0}, {30348, 0}, {30355, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_REFINE * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_REFINE_HWANG] = {
			{30040, 0},{30060, 0},{30061, 0},{30080, 0},{30344, 0},{30345, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_REFINE * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_REFINE_END] = {
			{30019, 0},{30042, 0},{30091, 0},{30367, 0},{30354, 0},{30349, 0},{30015, 0},{30087, 0},{30016, 0},{30086, 0},{30343, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_REFINE * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_REFINE_SPECIAL] = {
			{30371, 0},{30372, 0},{30373, 0}, {30374, 0}, {30375, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_REFINE * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_REFINE_PEARL] = {
			{27992, 0},{27993, 0},{27994, 0},
		};

		//SHOP_SEARCH_CATEGORY_HERBALISM
		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_PRIMARY] = {
			{50721, 0},{50722, 0},{50723, 0},{50724, 0},{50725, 0},{50726, 0},{50727, 0},{50728, 0},{50729, 0},{50730, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_SPECIAL] = {
			{50731, 0},{50732, 0},{50733, 0},{50734, 0},{50735, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_WATER_OFFENSIVE] = {
			{51720, 0}, {51725, 0}, {51740, 0}, {51745, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_WATER_DEFENSIVE] = {
			{51730, 0}, {51750, 0}, {51800, 0}, {51805, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_WATER_POWER] = {
			{27100, 0}, {27103, 0}, {51700, 0}, {51705, 0}, {51710, 0}, {51715, 0}, {51735, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_JUICE_OFFENSIVE] = {
			{51721, 0}, {51726, 0}, {51741, 0}, {51746, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_JUICE_DEFENSIVE] = {
			{51731, 0}, {51751, 0}, {51801, 0}, {51806, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_JUICE_POWER] = {
			{27101, 0}, {27104, 0}, {51701, 0}, {51706, 0}, {51711, 0}, {51716, 0},
			{51736, 0}, {51755, 0}, {51760, 0}, {51775, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_DEW_OFFENSIVE] = {
			{51722, 0}, {51727, 0}, {51742, 0}, {51747, 0}, {51748, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_DEW_DEFENSIVE] = {
			{51732, 0}, {51752, 0}, {51753, 0}, {51802, 0}, {51807, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_DEW_POWER] = {
			{27102, 0}, {27105, 0}, {51702, 0}, {51707, 0}, {51712, 0}, {51717, 0},
			{51737, 0}, {51756, 0}, {51761, 0}, {51776, 0}, {51765, 0}, {51766, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_OTHER_POTION] = {
			{51782, 0}, {51783, 0}, {51784, 0}, {51785, 0}, {71153, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_AUTOPOTION] = {
			{72723, 0}, {72724, 0}, {72725, 0}, {72727, 0}, {72728, 0}, {72729, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_RECIPE_OFFENSIVE] = {
			{50917, 0}, {50918, 0}, {50921, 0}, {50922, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_RECIPE_DEFENSIVE] = {
			{50919, 0}, {50942, 0}, {50943, 0}, {50923, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_RECIPE_POWER] = {
			{50911, 0},
			{50912, 0},
			{50913, 0},
			{50914, 0},
			{50915, 0},
			{50916, 0},
			{50920, 0},
			{50924, 0},
			{50925, 0},
			{50926, 0},
			{50927, 0},
			{50929, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HERBALISM * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HERB_RECIPE_OTHER] = {
			{50930, 0}, {50932, 0}, {50933, 0}, {50934, 0}, {50935, 0},
		};

		// SHOP_SEARCH_CATEGORY_FISHING
		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_FISHING * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_FISHING_FISH] = {
			{27802, 0}, {27803, 0}, {27804, 0}, {27805, 0}, {27806, 0},
			{27807, 0}, {27808, 0}, {27809, 0}, {27810, 0}, {27811, 0}, {27812, 0}, {27813, 0}, {27814, 0},
			{27815, 0}, {27816, 0}, {27818, 0}, {27819, 0}, {27820, 0}, {27821, 0}, {27822, 0}, {27823, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_FISHING * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_FISHING_FISH_COOK] = {
			{27866, 0}, {27868, 0}, {27869, 0}, {27870, 0}, {27871, 0}, {27872, 0}, {27873, 0}, {27875, 0},
			{27879, 0}, {27880, 0}, {27881, 0}, {27882, 0}, {27883, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_FISHING * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_FISHING_FISH_OTHER] = {
			{70201, 0}, {70007, 0}, {70051, 0}, {70050, 0}, {70048, 0}, {70049, 0}, {70102, 0}, {27798, 0}, {27799, 0}, {27987, 0},
		};

		// SHOP_SEARCH_CATEGORY_MINING
		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_MINING * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_MINING_ORE] = {
			{50601, 0}, {50603, 0}, {50604, 0}, {50605, 0}, {50606, 0}, {50607, 0}, {50608, 0}, {50609, 0}, {50610, 0},
			{50611, 0}, {50612, 0}, {50613, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_MINING * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_MINING_MELT] = {
			{50621, 0}, {50623, 0}, {50624, 0}, {50625, 0}, {50626, 0}, {50627, 0},
			{50628, 0}, {50629, 0}, {50630, 0}, {50631, 0}, {50632, 0}, {50633, 0},
		};

		//SHOP_SEARCH_CATEGORY_HORSE
		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HORSE * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HORSE_LEARN] = {
			{50050, 0}, {50060, 0}, {50061, 0}, {50062, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_HORSE * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_HORSE_OTHER] = {
			{50054, 0}, {50055, 0}, {50056, 0}, {50083, 0}, {30378, 0},
		};

		// SHOP_SEARCH_CATEGORY_SPECIAL
		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_SPECIAL * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SPECIAL_REFINE] = {
			{25040, 0}, {25043, 0}, {25044, 0}, {25045, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_SPECIAL * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SPECIAL_TOITEM] = {
			{25100, 0}, {30368, 0}, {30370, 0}, {71282, 0}, {71284, 0}, {71285, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_SPECIAL * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SPECIAL_CHARACTER] = {
			{71201, 0},  {71294, 0}, {70102, 0}, {50513, 0}, {70014, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_SPECIAL * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SPECIAL_OTHER] = {
			{70008, 0}, {72002, 0}, {72005, 0}, {72006, 0}, {70042, 0}, {70038, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_SPECIAL * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SPECIAL_DRAGON_VOUCHER] = {
			{80017, 0}, {80014, 0}, {80018, 0}, {80015, 0}, {80016, 0}, {72199, 0}
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_SPECIAL * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SPECIAL_QUEST] = {
			{50307, 0}, {50308, 0}, {50309, 0}, {50310, 0},
		};

		m_shopSearchFilters[SHOP_SEARCH_CATEGORY_SPECIAL * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SPECIAL_LOOTBOX] = {
			{50006, 0}, {50007, 0}, {50008, 0}, {50009, 0}, {50011, 0}, {50012, 0}, {50013, 0}, {50037, 0}, {50024, 0}, {50025, 0},
			{50070, 0}, {50071, 0}, {50073, 0}, {50076, 0}, {50077, 0}, {50078, 0}, {50079, 0}, {50081, 0}, {50082, 0}, {50097, 0},
			{50098, 0},
		};
		
	}

	CShopManager::CShopManager()
	{
		auto* info = AllocEventInfo<event_info_data>();
		m_eventShopDuration = event_create(func_offlineshop_update_duration, info, OFFLINESHOP_DURATION_UPDATE_TIME);
		PrepareShopSearchFilters();
	}

	CShopManager::~CShopManager()
	{
		Destroy();
	}


	void CShopManager::Destroy()
	{
		// deleting event
		if(m_eventShopDuration)
			event_cancel(&m_eventShopDuration);
		m_eventShopDuration = nullptr;

		// clearing containers
		m_mapOffersByBuyer.clear();
		m_mapSafeboxs.clear();
		m_mapShops.clear();
		m_mapOffer.clear();
	}

	bool CShopManager::ParseShopName(const char* c_pszSign, std::string& stShopSign)
	{
		char szSign[SHOP_SIGN_MAX_LEN * 2 + 1];
		DBManager::instance().EscapeString(szSign, sizeof(szSign), c_pszSign, std::strlen(c_pszSign));

		stShopSign = szSign;
		if (stShopSign.length() == 0)
			return false;


		if (stShopSign.length() > SHOP_SIGN_MAX_LEN)
			stShopSign.resize(SHOP_SIGN_MAX_LEN);

		if (CBanwordManager::instance().CheckString(stShopSign.c_str(), stShopSign.length()) ||
			!has_proper_characters(stShopSign))
		{
			return false;
		}

		return true;
	}

	class CCheckShopPosition
	{
	public:
		LPCHARACTER	m_me;
		bool blockShopCreation;

		CCheckShopPosition(LPCHARACTER me) : m_me(me), blockShopCreation(false)
		{
		}

		void operator () (LPENTITY ent)
		{
			if (!m_me)
				return;

			if (!ent->IsType(ENTITY_NEWSHOPS))
				return;

			if (m_me->DistanceTo(ent) > 60)
				return;

			blockShopCreation = true;
		}
	};

	bool CShopManager::CanOpenOnMap(LPCHARACTER ch)
	{
		if (!ch)
			return false;

		long mapIndex = ch->GetMapIndex();
		auto it = m_mapLimitShops.find(mapIndex);
		if (it == m_mapLimitShops.end())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_CANNOT_OPEN_HERE"));
			return false;
		}

		DWORD shopsOnMap = 0;
		for (auto shopIt : m_mapShops)
		{
			if (shopIt.second && shopIt.second->GetDuration() > 0)
			{
				auto spawn = shopIt.second->GetSpawn();
				if (spawn.channel == g_bChannel && spawn.map == mapIndex)
					shopsOnMap++;
			}
		}

		if (shopsOnMap >= it->second.shopPremiumLimit)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_MAX_LIMIT"));
			return false;
		}

		if (ch->GetPremiumRemainSeconds(PREMIUM_SHOP) <= 0 && shopsOnMap >= it->second.shopLimit)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_MAX_LIMIT_FREE"));
			return false;
		}

		if (!ch->GetSectree())
			return false;

		CCheckShopPosition f(ch);
		ch->GetSectree()->ForEachAround(f);
		if (f.blockShopCreation)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANT_OPEN_SHOP_HERE"));
			return false;
		}

		return true;
	}

	void CShopManager::LoadShopLimits(const TOfflineShopLimit* limits, WORD size)
	{
		m_mapLimitShops.clear();
		for (int i = 0; i < size; i++, ++limits)
		{
			TOfflineShopLimit limit = *limits;
			if (limit.channel != 0 && limit.channel != g_bChannel)
				continue;

			m_mapLimitShops.insert(std::make_pair(limit.mapIndex, limit));
		}
	}

	void CShopManager::_RemoveShopOffers(SHOP_HANDLE shop)
	{
		// storing all buyers's pids
		std::set<DWORD> buyers;

		// iterating over shop's offers
		for (auto& iter : shop->GetOffers())
		{
			auto& offer = iter.second;

			// for each offer removing from buyer's offer list
			DWORD buyer = offer->buyerid;
			buyers.emplace(buyer);

			// searching buyer into map
			auto it = m_mapOffersByBuyer.find(buyer);
			if (it != m_mapOffersByBuyer.end())
			{
				// removing offer from vector
				auto& offers = it->second;
				std::erase_if(offers, [=](auto&& boffer) { return boffer->offerid == offer->offerid; });
			}

			// removing offer from offer map
			m_mapOffer.erase(offer->offerid);
		}

		// refreshing offers page for buyers
		for (auto& pid : buyers)
			if (auto chBuyer = CHARACTER_MANAGER::instance().FindByPID(pid))
				if (chBuyer->IsLookingIkarusShopOfferList())
					RecvOfferListRequestPacket(chBuyer);
	}

#ifdef ENABLE_IKASHOP_ENTITIES
#ifdef EXTEND_IKASHOP_PRO
	void CShopManager::_UpdateEntity(SHOP_HANDLE shop)
	{
		DestroyShopEntity(shop);
		CreateShopEntity(shop);
	}
#endif

	void CShopManager::CreateShopEntity(SHOP_HANDLE shop)
	{
		auto& spawn = shop->GetSpawn();
		if (spawn.channel != g_bChannel)
			return;

		if (auto sectree = SECTREE_MANAGER::Instance().Get(spawn.map, spawn.x, spawn.y))
		{
			auto entity = std::make_shared<ShopEntity>();
			entity->SetShopName(shop->GetName());
			entity->SetOwnerName(shop->GetOwnerName());
			entity->SetMapIndex(spawn.map);
			entity->SetXYZ(spawn.x, spawn.y, 0);
			entity->SetShop(shop);
			sectree->InsertEntity(entity.get());
			entity->UpdateSectree();
			shop->SetEntity(entity);

			m_entityByVID[entity->GetVID()] = entity;
		}
	}

	void CShopManager::DestroyShopEntity(SHOP_HANDLE shop)
	{
		if(auto entity = shop->GetEntity())
		{
			m_entityByVID.erase(entity->GetVID());
			if (entity->GetSectree())
			{
				entity->ViewCleanup();
				entity->GetSectree()->RemoveEntity(entity.get());
			}
			entity->Destroy();
		}
	}

	void CShopManager::EncodeInsertShopEntity(const ShopEntity& shop, LPCHARACTER ch)
	{
		if (!ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader	= SUBHEADER_GC_INSERT_SHOP_ENTITY;
		pack.size = sizeof(pack)+ sizeof(TSubPacketGCInsertShopEntity);

		const auto& pos = shop.GetXYZ();

		TSubPacketGCInsertShopEntity subpack{};
		subpack.vid = shop.GetVID();
		subpack.type = shop.GetShopType();
		subpack.x = pos.x;
		subpack.y = pos.y;
		subpack.z = pos.z;
		str_to_cstring(subpack.name, shop.GetShopName());
		str_to_cstring(subpack.ownerName, shop.GetOwnerName());

		ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
		ch->GetDesc()->Packet(&subpack, sizeof(subpack));
	}

	void CShopManager::EncodeRemoveShopEntity(const ShopEntity& shop, LPCHARACTER ch)
	{
		if (!ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader	= SUBHEADER_GC_REMOVE_SHOP_ENTITY;
		pack.size = sizeof(pack)+ sizeof(TSubPacketGCRemoveShopEntity);

		TSubPacketGCRemoveShopEntity subpack{};
		subpack.vid = shop.GetVID();

		ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
		ch->GetDesc()->Packet(&subpack, sizeof(subpack));
	}

#endif

	CShopManager::SAFEBOX_HANDLE CShopManager::GetShopSafeboxByOwnerID(DWORD pid)
	{
		auto it = m_mapSafeboxs.find(pid);
		return it != m_mapSafeboxs.end() ? it->second : nullptr;
	}

	//offers
	bool CShopManager::PutsNewOffer(OFFER_HANDLE offer)
	{
		// registering offer by buyer
		auto& offers = m_mapOffersByBuyer[offer->buyerid];
		for(auto& boffer : offers)
			if(boffer->offerid == offer->offerid)
				return false;

		// adding offer to buyer list
		offers.emplace_back(offer);
		// registering offer by offer id
		m_mapOffer[offer->offerid] = offer;
		return true;
	}


	//db packets exchanging
	void CShopManager::SendShopBuyDBPacket(DWORD buyerid, DWORD ownerid, DWORD itemid, bool success)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_BUY_ITEM;

		TSubPacketGDBuyItem subpack{};
		subpack.guestid = buyerid;
		subpack.ownerid = ownerid;
		subpack.itemid = itemid;
		subpack.success = success;

		int tax = quest::CQuestManager::instance().GetEventFlag("offline_shop_tax");
		subpack.tax = tax == 0 ? OFFLINESHOP_DEFAULT_TAX_PCT : tax;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		
		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopBuyDBPacket(DWORD buyerid, DWORD ownerid,DWORD itemid, bool requester)
	{
		// searching shop
		auto shop = GetShopByOwnerID(ownerid);
		if(!shop)
			return false;
		
		// searching item
		auto shopItem = shop->GetItem(itemid);
		if(!shopItem)
			return false;

		if (auto ch = CHARACTER_MANAGER::instance().FindByPID(buyerid))
		{
			auto itemInstance = shopItem->CreateItem();
			if (!itemInstance)
			{
				sys_err("cannot create item ( itemid %u , vnum %u, shop owner %u, buyer %u )", itemid, shopItem->GetInfo().vnum, ownerid, buyerid );
				return false;
			}

			TItemPos pos;
			if (!ch->CanTakeInventoryItem(itemInstance, &pos))
			{
				itemInstance->SetSkipSave(true);
				M2_DESTROY_ITEM(itemInstance);

				SendShopSafeboxAddItemDBPacket(ch->GetPlayerID(), *shopItem);
				SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_SAFEBOX_RECV_ITEM");
			}

			else
			{
				itemInstance->AddToCharacter(ch, pos); 
				itemInstance->SetSkipSave(false);
				itemInstance->Save();
				ITEM_MANAGER::instance().FlushDelayedSave(itemInstance);
			}

			shop->BuyItem(shopItem->GetID());
			SendSearchResultDeleteClientPacket(ch, itemid);
		}

		else
		{
			shop->BuyItem(shopItem->GetID());
		}

		// hard close if shop is empty
		if (requester && shop->GetItems().size() < 1)
		{
			SendShopForceHardCloseDBPacket(ownerid);
		}

		auto ownerCh = CHARACTER_MANAGER::instance().FindByPID(ownerid);
		if (ownerCh)
		{
			NotifyOwnerItemSold(ownerCh, shopItem->GetVnum(), shopItem->GetInfo().count, shopItem->GetPrice().GetTotalYangAmount());
		}

		// The one place that knows a line has gone. A playerbot keeper is off
		// hunting while its counter sells, and the goods belong to the shop
		// entity rather than to its bag, so nothing on the AI side can notice
		// this by looking at the owner. Recorded here, drained on the owner's
		// own tick (playerbot_offline_shop.h).
		playerbot_offline::NoteSold(ownerid, itemid, shopItem->GetVnum(), shopItem->GetInfo().count, shopItem->GetPrice().GetTotalYangAmount());
		playerbot_offline::Complete(buyerid, playerbot_offline::Buy, itemid);
		return true;
	}

	void CShopManager::SendShopEditItemDBPacket(DWORD ownerid, DWORD itemid, const TPriceInfo& price)
	{
		playerbot_offline::Sent(ownerid, playerbot_offline::Edit, itemid);
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_EDIT_ITEM;

		TSubPacketGDEditItem subpack{};
		subpack.ownerid = ownerid;
		subpack.itemid = itemid;
		subpack.price = price;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		
		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopEditItemClientPacket(LPCHARACTER ch, DWORD itemid, const TPriceInfo& price)
	{
		if(!ch || !ch->GetIkarusShop())
			return false;

		if (!ch->IsLookingShopOwner())
			return false;

		if(!CheckGMLevel(ch))
			return false;

		if(!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_EDIT_ITEM))
			return false;

		if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::IkaShopTouchItem, std::chrono::milliseconds(200)))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("NEXT_ACTION_WAIT_NO_FORMAT"));
			return false;
		}

		if (ch->IsBusy(BUSY_SHOP_MANAGE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
			return false;
		}

		if (!IsNearShop(ch, ch->GetIkarusShop(), OWNER_INTERRACT_DISTANCE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_TOO_FAR"));
			return false;
		}

		if (!IsGoodSalePrice(price))
			return false;

		auto shop = ch->GetIkarusShop();

		if (!shop->IsEditMode())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The shop is not in edit mode."));
			return false;
		}

		auto shopItem = shop->GetItem(itemid);
		if (!shopItem)
			return false;

		// patch with warp check
		ch->SetIkarusShopUseTime();

#ifndef ENABLE_CHEQUE_SYSTEM
		auto& itemPrice = shopItem->GetPrice();
		if(price.yang == itemPrice.yang)
			return true;
#endif

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Item price edited successfully."));
		SendShopEditItemDBPacket(shop->GetOwnerPID(), itemid, price);
		return true;
	}

	bool CShopManager::RecvShopRequestEditClientPacket(LPCHARACTER ch, bool isEdit)
	{
		if (!ch || !ch->GetIkarusShop())
			return false;

		if (!ch->IsLookingShopOwner())
			return false;

		if (!CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_REQUEST_EDIT_ITEM))
			return false;

		if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::IkaShopEditMode, std::chrono::milliseconds(1000)))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("NEXT_ACTION_WAIT_NO_FORMAT"));
			return false;
		}

		if (ch->IsBusy(BUSY_SHOP_MANAGE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
			return false;
		}

		if (!IsNearShop(ch, ch->GetIkarusShop(), OWNER_INTERRACT_DISTANCE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_TOO_FAR"));
			return false;
		}

		auto shop = ch->GetIkarusShop();
		if (shop)
		{
			shop->SetEditMode(isEdit);
			ch->ChatPacket(CHAT_TYPE_COMMAND, "shop_edit %d", isEdit);
		}
		return true;
	}

	void CShopManager::SendShopRemoveItemDBPacket(DWORD ownerid, DWORD itemid)
	{
		playerbot_offline::Sent(ownerid, playerbot_offline::Remove, itemid);
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_REMOVE_ITEM;

		TSubPacketGDRemoveItem subpack{};
		subpack.ownerid = ownerid;
		subpack.itemid = itemid;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		
		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopRemoveItemDBPacket(DWORD ownerid, DWORD itemid, bool requester)
	{
		// searching shop
		auto shop = GetShopByOwnerID(ownerid);
		if(!shop)
			return false;

		// searching item
		auto shopItem = shop->GetItem(itemid);
		if(!shopItem)
			return false;

		// removing from shop
		shop->RemoveItem(itemid);

		// checking for requester
		if(requester)
		{
			static auto pos = TItemPos{};
			// checking if character can obtain the item
			auto ch = shop->FindOwnerCharacter();
			auto item = shopItem->CreateItem();

			if(!item || !ch || !ch->CanTakeInventoryItem(item, &pos))
			{
				// mt2009 edit: despite the safebox is disabled, it better go into the safebox than be deleted or sth

				// cleaning up item
				if(item){
					item->SetSkipSave(true);
					M2_DESTROY_ITEM(item);
				}

				// sending item to safebox
				SendShopSafeboxAddItemDBPacket(ownerid, *shopItem);
			}

			else
			{
				item->AddToCharacter(ch, pos);
				item->SetSkipSave(false);
				item->Save();
				ITEM_MANAGER::instance().FlushDelayedSave(item);
			}

			// hard close if shop is empty
			if (shop->GetItems().size() < 1)
			{
				SendShopForceHardCloseDBPacket(ownerid);
			}
		}

		playerbot_offline::Complete(ownerid, playerbot_offline::Remove, itemid);
		return true;
	}

	void CShopManager::SendShopAddItemDBPacket(DWORD ownerid, const TShopItem& iteminfo)
	{
		playerbot_offline::Sent(ownerid, playerbot_offline::Add, iteminfo.id);
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_ADD_ITEM;

		TSubPacketGDAddItem subpack{};
		subpack.ownerid = ownerid;
		subpack.sitem = iteminfo;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		
		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopAddItemDBPacket(DWORD ownerid, const TShopItem& iteminfo)
	{
		// searching shop
		auto shop = GetShopByOwnerID(ownerid);
		if(!shop)
			return false;

		// adding item to shop
		
		bool ok = shop->AddItem(iteminfo);
		playerbot_offline::Complete(ownerid, playerbot_offline::Add, iteminfo.id, ok);
		return ok;
	}

	bool ikashop::CShopManager::RecvShopChangeNameClientPacket(LPCHARACTER ch, const char* szName)
	{
		if (!ch || !ch->GetIkarusShop())
			return false;

		if (!ch->IsLookingShopOwner())
			return false;

		auto shop = ch->GetIkarusShop();

		if (!CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_SHOP_CHANGE_NAME))
			return false;

		if (ch->GetPremiumRemainSeconds(PREMIUM_SHOP) < 1)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_NO_PREMIUM"));
			return false;
		}

		if (ch->IsBusy(BUSY_SHOP_MANAGE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
			return false;
		}

		if (!IsNearShop(ch, shop, OWNER_INTERRACT_DISTANCE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_TOO_FAR"));
			return false;
		}

		if (shop->GetName() == szName)
		{
			return false;
		}

		std::string stShopSign;
		if (!ParseShopName(szName, stShopSign))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't give your shop an invalid name."));
			return false;
		}

		SendShopChangeNameDBPacket(ch->GetPlayerID(), stShopSign.c_str());
		return true;
	}


	//SHOPS
	void CShopManager::SendShopForceHardCloseDBPacket(DWORD pid)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader = SUBHEADER_GD_SHOP_FORCE_HARD_CLOSE;

		TSubPacketGDShopForceClose subpack{};
		subpack.ownerid = pid;
		subpack.gm = false;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	void CShopManager::SendShopForceSoftCloseDBPacket(DWORD pid, bool gm = false)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_SHOP_FORCE_SOFT_CLOSE;

		TSubPacketGDShopForceClose subpack{};
		subpack.ownerid = pid;
		subpack.gm = gm;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));
		
		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopForceCloseDBPacket(DWORD pid)
	{
		// getting shop
		auto shop = GetShopByOwnerID(pid);
		if(!shop)
			return false;

		// kicking out guests
		shop->KickAllGuests();
		_RemoveShopOffers(shop);
#ifdef ENABLE_IKASHOP_ENTITIES
		DestroyShopEntity(shop);
#endif
		shop->Clear();

		// searching character
		if (auto ch = CHARACTER_MANAGER::instance().FindByPID(pid)) {
			ch->SetIkarusShop(nullptr);

			if (ch->IsLookingShopOwner())
			{
				SendShopOpenMyShopNoShopClientPacket(ch);
			}
		}

		m_mapShops.erase(shop->GetOwnerPID());
		return true;
	}

	void CShopManager::SendShopLockBuyItemDBPacket(LPCHARACTER buyer, DWORD ownerid, ITEM_HANDLE item, long long TotalPriceSeen) //patch seen price check
	{
		playerbot_offline::Sent(buyer->GetPlayerID(), playerbot_offline::Buy, item->GetID());
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader = SUBHEADER_GD_BUY_LOCK_ITEM;

		TSubPacketGDLockBuyItem subpack{};
		subpack.guestid = buyer->GetPlayerID();
		subpack.ownerid = ownerid;
		subpack.itemid = item->GetID();
		subpack.seenprice = TotalPriceSeen;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		
		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopLockedBuyItemDBPacket(DWORD buyerid, DWORD ownerid,DWORD itemid)
	{
		// Internal negative lock acknowledgement; DB sends owner=0 on refusal.
		if (ownerid == 0) { playerbot_offline::Complete(buyerid, playerbot_offline::Buy, itemid, false); return false; }
		auto SendDBFeedback = [&](auto success){
			SendShopBuyDBPacket(buyerid, ownerid, itemid, success);
			if (!success) playerbot_offline::Complete(buyerid, playerbot_offline::Buy, itemid, false);
			return success;
		};

		auto shop = GetShopByOwnerID(ownerid);
		auto ch	= CHARACTER_MANAGER::instance().FindByPID(buyerid);

		if(!ch || !shop)
			return SendDBFeedback(false);

		auto item = shop->GetItem(itemid);
		if(!item || !item->CanBuy(ch))
			return SendDBFeedback(false);

		const auto& price = item->GetPrice();
		ch->ChangeGold(-price.yang);
#ifdef ENABLE_CHEQUE_SYSTEM
		ch->PointChange(POINT_CHEQUE, -price.cheque);
#endif
		return SendDBFeedback(true);
	}

	bool CShopManager::RecvShopExpiredDBPacket(DWORD pid)
	{
		auto shop = GetShopByOwnerID(pid);
		if (!shop)
			return false;

		// kicking out guests
		shop->KickAllGuests();
		_RemoveShopOffers(shop);

#ifdef ENABLE_IKASHOP_ENTITIES
		DestroyShopEntity(shop);
#endif

#ifndef EXTEND_IKASHOP_PRO
		if (auto ch = CHARACTER_MANAGER::instance().FindByPID(pid))
			ch->SetIkarusShop(NULL);

		shop->Clear();
		m_mapShops.erase(m_mapShops.find(shop->GetOwnerPID()));
#else
		shop->SetDuration(0);
		shop->RefreshToOwner();
#endif
		return true;
	}

	void CShopManager::SendShopCreateNewDBPacket(const TShopInfo& shop, const TShopItem* items)
	{
		playerbot_offline::Sent(shop.ownerid, playerbot_offline::Create, 0);
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_SHOP_CREATE_NEW;

		TSubPacketGDShopCreateNew subpack{};
		subpack.shop = shop;

		std::memcpy(subpack.items, items, sizeof(TShopItem) * SHOP_PLAYER_HOST_ITEM_MAX_NUM);

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopCreateNewDBPacket(const TShopInfo& info, const TShopItem* items)
	{
		CShopManager::SHOP_HANDLE shop;
		if (m_mapShops.contains(info.ownerid))
		{
			// EXTEND DURATION
			shop = m_mapShops[info.ownerid];
			if (shop)
			{
				shop->SetDuration(info.duration);
				shop->SetName(info.name);
				shop->SetOwnerName(info.ownerName);
#ifdef ENABLE_IKASHOP_ENTITIES
				shop->SetSpawn(info.spawn);
#endif
				shop->RefreshToOwner();
				{
					DestroyShopEntity(shop);
					CreateShopEntity(shop);
				}
			}
		}
		else
		{
			// CREATE NEW SHOP
			shop = PutsNewShop(info);
			if (auto owner = shop->FindOwnerCharacter()) {
				owner->SetIkarusShop(shop);
				//SendShopOpenMyShopClientPacket(owner);
			}
			m_mapShops[info.ownerid] = shop;
		}

		if (shop)
		{
			for (int i = 0; i < SHOP_PLAYER_HOST_ITEM_MAX_NUM; i++)
			{
				if (items[i].vnum < 1)
					break;

				shop->AddItem(items[i], false);
			}
		}
		playerbot_offline::Complete(info.ownerid, playerbot_offline::Create, 0);
		return true;
	}

	void CShopManager::SendShopChangeNameDBPacket(DWORD ownerid, const char* name)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_SHOP_CHANGE_NAME;

		TSubPacketGDShopChangeName subpack{};
		subpack.ownerid	= ownerid;
		strncpy(subpack.name, name, sizeof(subpack.name));

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		
		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopChangeNameDBPacket(DWORD ownerid, const char* name)
	{
		auto shop = GetShopByOwnerID(ownerid);
		if(!shop)
			return false;

		shop->SetName(name);
		shop->RefreshToOwner();

#ifdef ENABLE_IKASHOP_ENTITIES
#ifdef EXTEND_IKASHOP_PRO
		_UpdateEntity(shop);
#endif
#endif		
		return true;
	}

	void CShopManager::SendShopOwnerOnlineDBPacket(DWORD ownerID, const bool ownerOnlineState)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader = SUBHEADER_GD_OWNER_ONLINE;

		TSubPacketGDOwnerOnline subpack{};
		subpack.ownerid = ownerID;
		subpack.isOnline = ownerOnlineState;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopOwnerOnlineDBPacket(DWORD ownerID, const bool ownerOnlineState)
	{
		auto shop = GetShopByOwnerID(ownerID);
		if (!shop)
			return false;

		shop->SetOwnerOnline(ownerOnlineState);
		if (!ownerOnlineState && shop->IsEditMode())
			shop->SetEditMode(false);
		return true;
	}

	//OFFER
	void CShopManager::SendShopOfferNewDBPacket(const TOfferInfo& offer, ITEM_HANDLE item)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_OFFER_CREATE;

		TSubPacketGDOfferCreate subpack{};
		subpack.ownerid = offer.ownerid;
		subpack.itemid = offer.itemid;
		subpack.offer = offer;
		subpack.vnum = item->GetInfo().vnum;
		subpack.count = item->GetInfo().count;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		
		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopOfferNewDBPacket(const TOfferInfo& offer)
	{
		// searching shop
		auto shop = GetShopByOwnerID(offer.ownerid);
		if(!shop)
			return false;

		// making shared offer
		auto soffer = std::make_shared<TOfferInfo>(offer);

		// registering offer into shop
		
		if(!shop->AddOffer(soffer))
			return false;

		// registering offer into map
		if(!PutsNewOffer(soffer))
			return false;

		// notify buyer
		if(auto ch = CHARACTER_MANAGER::instance().FindByPID(offer.buyerid))
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_PRIVATE_OFFER_CREATED");

		// updating counters
		if (auto ch = CHARACTER_MANAGER::instance().FindByPID(offer.ownerid))
			if (ch->IsLookingShopOwner())
				SendBoardCountersClientPacket(ch);

		return true;
	}

	void CShopManager::SendShopOfferNotifiedDBPacket(DWORD offerid, DWORD ownerid)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_OFFER_NOTIFIED;

		TSubPacketGDOfferNotified subpack{};
		subpack.offerid	= offerid;
		subpack.ownerid	= ownerid;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopOfferNotifiedDBPacket(DWORD offerid, DWORD ownerid)
	{
		// searching shop
		auto shop = GetShopByOwnerID(ownerid);
		if(!shop)
			return false;

		// updating offer
		for (auto& iter : shop->GetOffers())
		{
			auto& offer = iter.second;

			if (offer->offerid == offerid)
			{				
				offer->notified = true;
				break;
			}
		}

		return true;
	}

	void CShopManager::SendShopOfferAcceptDBPacket(const TOfferInfo& offer)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_OFFER_ACCEPT;

		TSubPacketGDOfferNotified subpack{};
		subpack.ownerid = offer.ownerid;
		subpack.offerid = offer.offerid;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	void CShopManager::SendShopOfferCancelDBPacket(const TOfferInfo& offer, DWORD requester)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_OFFER_CANCEL;

		TSubPacketGDOfferCancel subpack{};
		subpack.ownerid = offer.ownerid;
		subpack.offerid = offer.offerid;
		subpack.requester = requester;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopOfferCancelDBPacket(DWORD offerid, DWORD ownerid, bool removing) //ikashop-updated 05/08/19
	{
		// searching offer handle
		auto it = m_mapOffer.find(offerid);

		// checking it's found
		if(it == m_mapOffer.end())
			return false;

		// copying handle
		auto currentOffer = it->second;

		// removing from map offer by id
		m_mapOffer.erase(it);

		// removing offer from shop
		if(auto shop = GetShopByOwnerID(ownerid))
			shop->RemoveOffer(offerid);

		// removing offer from map by buyer
		const auto buyiter = m_mapOffersByBuyer.find(currentOffer->buyerid);
		if (buyiter != m_mapOffersByBuyer.end())
		{
			auto& vec = buyiter->second;
			std::erase_if(vec, [&](auto&& offer){ return offer->offerid == offerid; });

			// removing from map if empty
			if (buyiter->second.empty())
				m_mapOffersByBuyer.erase(buyiter);
		}

		// refreshing owner view
		if (!removing)
		{
			if(auto chOwner = CHARACTER_MANAGER::Instance().FindByPID(ownerid)){
				if(chOwner->IsLookingIkarusShopOfferList())
					RecvOfferListRequestPacket(chOwner);
				// updating counters
				if (chOwner->IsLookingShopOwner())
					SendBoardCountersClientPacket(chOwner);
			}
		}

		// refreshing buyer view
		if(auto chBuyer = CHARACTER_MANAGER::Instance().FindByPID(currentOffer->buyerid))
			if (chBuyer->IsLookingIkarusShopOfferList())
				RecvOfferListRequestPacket(chBuyer);

		return true;
	}

	bool CShopManager::RecvShopOfferAcceptDBPacket(DWORD offerid, DWORD ownerid)
	{
		// searching shop
		auto shop = GetShopByOwnerID(ownerid);
		if(!shop)
			return false;

		// searching offer handle
		auto it = m_mapOffer.find(offerid);
		if(it == m_mapOffer.end())
			return false;

		// copying handle & removing from map
		auto offer = it->second;
		m_mapOffer.erase(it);

		// accepting the offer		
		shop->AcceptOffer(offer);

		//removing offer from offer by buyer
		auto buyIter = m_mapOffersByBuyer.find(offer->buyerid);
		if (buyIter != m_mapOffersByBuyer.end())
		{
			// removing using find which interrupts the search at first match
			auto& offerVec = buyIter->second;
			auto offerIt = std::find_if(offerVec.begin(), offerVec.end(), [&](auto&& current) { return current->offerid == offerid; });
			if(offerIt != offerVec.end())
				offerVec.erase(offerIt);

			// removing from map if vector is empty
			if (offerVec.empty())
				m_mapOffersByBuyer.erase(buyIter);
		}
		
		// notifying to buyer
		if(auto chBuyer = CHARACTER_MANAGER::Instance().FindByPID(offer->buyerid))
			if (chBuyer->IsLookingIkarusShopOfferList())
				RecvOfferListRequestPacket(chBuyer);

		// notifying to owner
		if(auto chOwner = CHARACTER_MANAGER::Instance().FindByPID(ownerid)){
			if (chOwner->IsLookingIkarusShopOfferList())
				RecvOfferListRequestPacket(chOwner);
			// updating counters
			if (chOwner->IsLookingShopOwner())
				SendBoardCountersClientPacket(chOwner);
		}

		return true;
	}

	bool CShopManager::RecvShopSafeboxAddItemDBPacket(DWORD ownerid, DWORD itemid, const TShopPlayerItem& item)
	{
		auto ch = CHARACTER_MANAGER::instance().FindByPID(ownerid);
		auto safebox = ch && ch->GetIkarusShopSafebox() ? ch->GetIkarusShopSafebox() : GetShopSafeboxByOwnerID(ownerid);
		if(!safebox)
			return false;

		safebox->AddItem(item);
		if(ch && ch->GetIkarusShopSafebox() && ch->GetDesc())
		{
			if(auto insertedItem = safebox->GetItem(itemid))
			{
				TSubPacketGCShoppingSafeboxAddItem subpack{};
				subpack.item = static_cast<const TShopPlayerItem&>(insertedItem->GetInfo());
			
				TPacketGCNewIkarusShop pack{};
				pack.header = HEADER_GC_NEW_OFFLINESHOP;
				pack.size = sizeof(pack) + sizeof(subpack);
				pack.subheader = SUBHEADER_GC_SHOP_SAFEBOX_ADD_ITEM;

				ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
				ch->GetDesc()->Packet(&subpack, sizeof(subpack));
			}
		}

		if (ch && ch->IsLookingShopOwner())
			SendBoardCountersClientPacket(ch);

		
		return true;
	}

	bool CShopManager::SendShopSafeboxAddItemDBPacket(DWORD ownerid, const CShopItem& item) {
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader = SUBHEADER_GD_SAFEBOX_ADD_ITEM;


		TSubPacketGDSafeboxAddItem subpack{};
		subpack.ownerid = ownerid;
		subpack.itemid = item.GetID();
		subpack.item = static_cast<const TShopPlayerItem&>(item.GetInfo());

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
		return true;
	}

	bool CShopManager::RecvShopSafeboxAddValutesDBPacket(DWORD ownerid, const TValutesInfo& valute)
	{
		auto ch = CHARACTER_MANAGER::instance().FindByPID(ownerid);
		auto safebox = ch && ch->GetIkarusShopSafebox() ? ch->GetIkarusShopSafebox() : GetShopSafeboxByOwnerID(ownerid);

		if(!safebox)
			return false;

		safebox->AddValute(valute);
		if(ch && ch->GetIkarusShopSafebox())
			safebox->RefreshToOwner(ch);

		if (ch && ch->IsLookingShopOwner())
			SendBoardCountersClientPacket(ch);

		return true;
	}

	void CShopManager::SendShopSafeboxGetItemDBPacket(DWORD ownerid, DWORD itemid)
	{
		playerbot_offline::Sent(ownerid, playerbot_offline::WithdrawItem, itemid);
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_SAFEBOX_GET_ITEM;

		TSubPacketGDSafeboxGetItem subpack{};
		subpack.ownerid	= ownerid;
		subpack.itemid	= itemid;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		
		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	void CShopManager::SendShopSafeboxGetValutesDBPacket(DWORD ownerid, const TValutesInfo& valutes)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader	= SUBHEADER_GD_SAFEBOX_GET_VALUTES;

		TSubPacketGDSafeboxGetValutes subpack{};
		subpack.ownerid	= ownerid;
		subpack.valute = valutes;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopSafeboxLoadDBPacket(DWORD ownerid, const TValutesInfo& valute, const std::vector<TShopPlayerItem>& items)
	{
		// resetting/inserting safebox
		auto& safebox = m_mapSafeboxs[ownerid] = std::make_shared<CShopSafebox>();
		safebox->SetValuteAmount(valute);

		// adding items
		for (auto& item : items)
			safebox->AddItem(item);

		return true;
	}

	bool CShopManager::RecvShopSafeboxExpiredItemDBPacket(DWORD ownerid, DWORD itemid) {
		if (auto data = GetShopSafeboxByOwnerID(ownerid)) 
		{
			data->RemoveItem(itemid);
			data->RefreshToOwner();
			// updating counters
			if(auto ch = CHARACTER_MANAGER::instance().FindByPID(ownerid))
				if (ch->IsLookingShopOwner())
					SendBoardCountersClientPacket(ch);
		} 
		
		return true;
	}

	bool CShopManager::RecvShopSafeboxGetItemConfirm(DWORD ownerid, DWORD itemid)
	{
		auto _SendFeedbackToDB = [&](auto result){
			TPacketGDNewIkarusShop pack{};
			pack.bSubHeader = ikashop::SUBHEADER_GD_SAFEBOX_GET_TIME_FEEDBACK;

			ikashop::TSubPacketGDSafeboxGetItemFeedback subpack{};
			subpack.ownerid = ownerid;
			subpack.itemid = itemid;
			subpack.result = result;

			TEMP_BUFFER buffer;
			buffer.write(&pack, sizeof(pack));
			buffer.write(&subpack, sizeof(subpack));
			db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buffer.read_peek(), buffer.size());
			playerbot_offline::Complete(ownerid, playerbot_offline::WithdrawItem, itemid, result);
			return result;
		};

		// searching safebox
		auto safebox = GetShopSafeboxByOwnerID(ownerid);
		if(!safebox)
			return _SendFeedbackToDB(false);

		// searching item
		auto item = safebox->GetItem(itemid);
		if(!item)
			return _SendFeedbackToDB(false);
		
		// searching character
		auto ch = CHARACTER_MANAGER::instance().FindByPID(ownerid);
		if(!ch)
			return _SendFeedbackToDB(false);

		// creating item
		auto itemInstance = item->CreateItem();
		if(!itemInstance)
			return _SendFeedbackToDB(false);

		// checking item can be moved to inventory
		static TItemPos pos{};
		if(!ch->CanTakeInventoryItem(itemInstance, &pos))
		{
			itemInstance->SetSkipSave(true);
			M2_DESTROY_ITEM(itemInstance);
			return _SendFeedbackToDB(false);
		}

		// giving item to character
		if (!safebox->RemoveItem(itemid))
		{
			itemInstance->SetSkipSave(true);
			M2_DESTROY_ITEM(itemInstance);
			return _SendFeedbackToDB(false);
		}

		if (ch->IsLookingShopOwner())
			SendBoardCountersClientPacket(ch);

		// refreshing to owner
		if(auto ch = CHARACTER_MANAGER::instance().FindByPID(ownerid))
		{
			TPacketGCNewIkarusShop pack{};
			pack.header = HEADER_GC_NEW_OFFLINESHOP;
			pack.subheader = SUBHEADER_GC_SHOP_SAFEBOX_REMOVE_ITEM;
			pack.size = sizeof(pack) + sizeof(TSubPacketGCShopSafeboxRemoveItem);

			TSubPacketGCShopSafeboxRemoveItem subpack{};
			subpack.itemid = itemid;

			if(ch->GetDesc())
			{
				ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
				ch->GetDesc()->Packet(&subpack, sizeof(subpack));
			}
		}

		itemInstance->AddToCharacter(ch, pos);
		itemInstance->SetSkipSave(false);
		itemInstance->Save();
		ITEM_MANAGER::Instance().FlushDelayedSave(itemInstance);
		return _SendFeedbackToDB(true);
	}

	void CShopManager::SendShopLoadExpiredShop(DWORD ownerid)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader = SUBHEADER_GD_LOAD_EXPIRED_SHOP;

		TSubPacketGDLoadExpiredShop subpack{};
		subpack.owner = ownerid;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	//AUCTION
	void CShopManager::SendAuctionCreateDBPacket(const TAuctionInfo& auction)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader = SUBHEADER_GD_AUCTION_CREATE;

		TSubPacketGDAuctionCreate subpack{};
		subpack.auction = auction;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP , 0 , buff.read_peek() , buff.size());
	}

	void CShopManager::SendAuctionAddOfferDBPacket(const TAuctionOfferInfo& offer)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader = SUBHEADER_GD_AUCTION_ADD_OFFER;

		TSubPacketGDAuctionAddOffer subpack{};
		subpack.offer = offer;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP , 0 , buff.read_peek() , buff.size());
	}

	bool CShopManager::RecvAuctionCreateDBPacket(const TAuctionInfo& auction)
	{
		//check if exist
		if(m_mapAuctions.find(auction.owner) != m_mapAuctions.end())
			return false;

		//set info
		auto& obj = m_mapAuctions[auction.owner] = std::make_shared<CAuction>();
		obj->SetInfo(auction);


		//check about owner
		if (auto ch = CHARACTER_MANAGER::instance().FindByPID(auction.owner))
		{
			ch->SetIkarusAuction(obj);
			SendAuctionOpenAuctionClientPacket(ch, *obj);
		}

		return true;
	}

	bool CShopManager::RecvAuctionAddOfferDBPacket(const TAuctionOfferInfo& offer)
	{
		

		//check if exists
		auto auction = GetAuctionByOwnerID(offer.ownerid);
		if(!auction)
			return false;

		//adding offer
		auction->AddOffer(offer);

		//adding 1 minute time when new offer is done in the last minute
		if (auction->GetInfo().duration == 0)
			auction->IncreaseDuration();

		// updating counters
		if (auto ch = CHARACTER_MANAGER::instance().FindByPID(offer.ownerid))
			if (ch->IsLookingShopOwner())
				SendBoardCountersClientPacket(ch);

		return true;
	}

	//updated 30/09/19
	bool CShopManager::RecvAuctionExpiredDBPacket(DWORD id)
	{
		// searching auction inside map
		auto it = m_mapAuctions.find(id);
		if (it == m_mapAuctions.end())
			return false;


		// copying guest list & removing auction from auctionlist
		auto guests{ it->second->GetGuests() };
		m_mapAuctions.erase(it);

		// kicking auction guests
		for (auto& guest : guests){
			if(auto chGuest = CHARACTER_MANAGER::instance().FindByPID(guest))
			{
				chGuest->SetIkarusAuctionGuest(NULL);
				RecvAuctionListRequestClientPacket(chGuest);
			}
		}

		// setting auction to null on owner character
		if (auto owner = CHARACTER_MANAGER::instance().FindByPID(id)){
			owner->SetIkarusAuction(NULL);
			// updating counters
			if (owner->IsLookingShopOwner())
				SendBoardCountersClientPacket(owner);
		}


		return true;
	}

	bool CShopManager::RecvAuctionOfferSeenDBPacket(DWORD ownerid, DWORD buyerid, long long price)
	{
		if(auto auction = GetAuctionByOwnerID(ownerid))
		{
			for(auto& offer : auction->GetOffers())
			{
				if(offer->buyerid == buyerid && offer->ownerid == ownerid && offer->price.yang == price){
					offer->seenflag = true;
					break;
				}
			}

			if(auto ch = CHARACTER_MANAGER::instance().FindByPID(ownerid))
				if(ch->IsLookingShopOwner())
					SendBoardCountersClientPacket(ch);
		}

		return true;
	}

#ifdef EXTEND_IKASHOP_PRO
	bool CShopManager::RecvNotificationLoadDBPacket(DWORD owner, const std::vector<TNotificationInfo>& notifications)
	{
		m_notificationMap[owner] = notifications;
		return true;
	}

	bool CShopManager::RecvNotificationForwardDBPacket(DWORD owner, const TNotificationInfo& notification)
	{
		if(auto ch = CHARACTER_MANAGER::instance().FindByPID(owner))
		{
			auto& notifications = m_notificationMap[owner];
			notifications.emplace_back(notification);
			if (ch->IsLookingNotificationList())
				SendNotificationListClientPacket(ch);
			if(ch->IsLookingShopOwner())
				SendBoardCountersClientPacket(ch);
		}
		return true;
	}

	void CShopManager::SendShopRestoreDurationDBPacket(DWORD owner)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader = ikashop::SUBHEADER_GD_SHOP_RESTORE_DURATION;

		TEMP_BUFFER buffer;
		buffer.write(&pack, sizeof(pack));
		buffer.write(&owner, sizeof(owner));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buffer.read_peek(), buffer.size());
	}
	
	bool CShopManager::RecvShopRestoreDurationDBPacket(DWORD owner)
	{
		if(auto shop = GetShopByOwnerID(owner))
		{
			shop->SetDuration(OFFLINESHOP_DURATION_MAX_MINUTES);
			shop->RefreshToOwner();

			{
				DestroyShopEntity(shop);
				CreateShopEntity(shop);
			}
		}

		return true;
	}
#endif


#ifdef ENABLE_IKASHOP_ENTITIES
#ifdef EXTEND_IKASHOP_PRO
	void CShopManager::RecvMoveShopEntityDBPacket(DWORD owner, const TShopSpawn& spawn)
	{
		if(auto shop = GetShopByOwnerID(owner))
		{
			shop->SetSpawn(spawn);
			_UpdateEntity(shop);
		}
	}
#endif
#endif

	void CShopManager::SendShopLockedSlotStateDBPacket(DWORD pid, BYTE unlockCount, bool isPremium)
	{
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader = ikashop::SUBHEADER_GD_SHOP_LOCKED_SLOT_STATE;

		TSubPacketGDLockedSlotState subpack{};
		subpack.ownerid = pid;
		subpack.unlockCount = unlockCount;
		subpack.isPremium = isPremium;

		TEMP_BUFFER buffer;
		buffer.write(&pack, sizeof(pack));
		buffer.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buffer.read_peek(), buffer.size());
	}

	void CShopManager::RecvShopLockedSlotStateDBPacket(DWORD ownerID, BYTE unlockCount, bool isPremium)
	{
		if (auto shop = GetShopByOwnerID(ownerID))
		{
			shop->SetUnlockCount(unlockCount);
			shop->SetPremium(isPremium);
		}
	}

	//client packets exchanging
	bool CShopManager::OpenOfflineShop(LPCHARACTER ch, const char* shopSign, TShopItemTable* pItemTable, BYTE bItemCount, BYTE bTimeIndex)
	{
		if(!ch)
			return false;

		if (!CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_CREATE_NEW))
			return false;

		if (!CheckCharacterActions(ch))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot perform this operation right now."));
			return false;
		}

#ifdef EXTEND_IKASHOP_PRO
		if (ch->GetIkarusShop() && ch->GetIkarusShop()->GetDuration() != 0)
			return false;
#endif
#ifdef ENABLE_IKASHOP_ENTITIES
		if(!CanOpenOnMap(ch))
			return false;
#endif

		if (bTimeIndex < 1 || bTimeIndex >= OFFLINESHOP_TIME_OPTION_MAX)
		{
			ch->ChatDebug("unknown time index %u", bTimeIndex);
			return false;
		}
		auto shopTime = aOfflineShopTime[bTimeIndex];
		if (ch->GetGold() < shopTime.price)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have enough Yang."));
			return false;
		}

		if (shopTime.isPremium && ch->GetPremiumRemainSeconds(PREMIUM_SHOP) <= 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_NO_PREMIUM"));
			return false;
		}

		if (ch->IsBusy(BUSY_SHOP_MANAGE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
			return false;
		}

		if (thecore_pulse() - ch->GetIkarusShopOpenClosedTime() < PASSES_PER_SEC(10))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("WAIT_AFTER_EXCHANGE"));
			return false;
		}

		// patch with warp check
		ch->SetIkarusShopUseTime();
		ch->SetIkarusShopOpenClosedTime();

		ch->ChangeGold(-shopTime.price);

		ikashop::TShopInfo shopinfo{};
		shopinfo.ownerid = ch->GetPlayerID();
		shopinfo.duration = shopTime.duration;
		shopinfo.unlockCount = ch->GetShopUnlockedProgress();
		shopinfo.isPremium = ch->GetPremiumRemainSeconds(PREMIUM_SHOP) > 0;
		str_to_cstring(shopinfo.name, shopSign);
		str_to_cstring(shopinfo.ownerName, ch->GetName());

#ifdef ENABLE_IKASHOP_ENTITIES
		shopinfo.spawn.map = ch->GetMapIndex();
		shopinfo.spawn.x = ch->GetX();
		shopinfo.spawn.y = ch->GetY();
		shopinfo.spawn.channel = g_bChannel;
#endif

//#ifdef EXTEND_IKASHOP_PRO
//		if (ch->GetIkarusShop())
//		{
//			SendShopRestoreDurationDBPacket(ch->GetPlayerID());
//			return true;
//		}
//#endif

		// the system assumes that the items were already checked in CHARACTER::OpenMyShop
		TShopItem items[SHOP_PLAYER_HOST_ITEM_MAX_NUM];
		std::memset(items, 0, sizeof(items));
		for (size_t i = 0; i < bItemCount; ++i)
		{
			auto item = ch->GetItem(pItemTable[i].pos);
			if (!item)
			{
				sys_err("OpenOfflineShop item is NULL pid %d ", ch->GetPlayerID());
				return false;
			}

			ch->ChatDebug("add item %d pos %d", item->GetVnum(), pItemTable[i].display_pos);

			TShopItem sitem{};
			TPriceInfo price{ pItemTable[i].price };
			ProcessAddItemToShop(sitem, ch, item, pItemTable[i].display_pos, price);
			items[i] = sitem;
		}
		
		SendShopCreateNewDBPacket(shopinfo, items);
		return true;
	}

	bool CShopManager::IsNearShop(LPCHARACTER ch, SHOP_HANDLE shop, long maxDistance)
	{
		if (!ch)
			return false;

		if (!shop)
		{
			ch->ChatDebug("shop does not exist");
			return false;
		}

		if (shop->GetDuration() == 0) // skonczyl sie sklep to mozna gdziekolwiek
			return true;

		auto spawn = shop->GetSpawn();
		if (spawn.channel != g_bChannel)
		{
			ch->ChatDebug("shop channel %u (current %u)", spawn.channel, g_bChannel);
			return false;
		}

		int distance = ch->DistanceTo(spawn.x, spawn.y);
		if (distance > maxDistance)
		{
			ch->ChatDebug("too far %d", distance);
			return false;
		}

		return true;
	}

	bool CShopManager::RecvShopReopenClientPacket(LPCHARACTER ch, const char* name, BYTE bTimeIndex)
	{
		if (!ch)
			return false;

		if (!ch->IsLookingShopOwner())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("OFFLINE_SHOP_MANAGE_MUST_BE_OPEN"));
			return false;
		}

		auto shop = ch->GetIkarusShop();
		if (!shop || shop->GetDuration() > 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("OFFLINE_SHOP_CANNOT_REOPEN"));
			return false;
		}

		if (shop->GetSpawn().map < 1)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("OFFLINE_SHOP_CANNOT_REOPEN"));
			return false;
		}

		std::string stShopName;
		if (!ParseShopName(name, stShopName))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't give your shop an invalid name."));
			return false;
		}

		auto shopItems = shop->GetItems();
		if (shopItems.size() < 1)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("OFFLINE_SHOP_CANNOT_REOPEN_EMPTY"));
			return false;
		}

		for (auto it : shopItems)
		{
			auto itemHandle = it.second;
			if (itemHandle)
			{
				if (!ch->CanPlaceItemOnShopSlot(itemHandle->GetInfo().pos, itemHandle->GetTable()->bSize))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_NO_PREMIUM"));
					return false;
				}
			}
		}

		return OpenOfflineShop(ch, stShopName.c_str(), NULL, 0, bTimeIndex);
	}

	bool CShopManager::RecvShopForceCloseClientPacket(LPCHARACTER ch)
	{
		if(!ch || !ch->GetIkarusShop())
			return false;

		if (!ch->IsLookingShopOwner())
			return false;

		if (!CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_FORCE_CLOSE_SHOP))
			return false;

		if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::IkaShopTouchItem, std::chrono::milliseconds(1000)))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("NEXT_ACTION_WAIT_NO_FORMAT"));
			return false;
		}

		if (ch->IsBusy(BUSY_SHOP_MANAGE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
			return false;
		}

#ifndef ENABLE_MT2009_DISABLE_IKASHOP_DEFAULT_SAFEBOX
		if(!CheckSafeboxSize(ch))
			return false;
#endif

		auto shop = ch->GetIkarusShop();
		if (!IsNearShop(ch, shop, OWNER_INTERRACT_DISTANCE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_TOO_FAR"));
			return false;
		}

		if (shop->IsEditMode())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The shop is in edit mode."));
			return false;
		}

		auto safebox = ch->GetIkarusShopSafebox();
		if (safebox && safebox->GetItems().size() > 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CLEAR_SHOP_DEPOSIT_FIRST"));
			return false;
		}

		if (thecore_pulse() - ch->GetIkarusShopOpenClosedTime() < PASSES_PER_SEC(10))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("WAIT_AFTER_EXCHANGE"));
			return false;
		}

		// patch with warp check
		ch->SetIkarusShopUseTime();
		ch->SetIkarusShopOpenClosedTime();

		// nie usuwamy itemow z shopa na zamkniecie, gracz moze sobie to odnowic
		//for(auto& [id, item] : shop->GetItems())
		//	SendShopRemoveItemDBPacket(ch->GetPlayerID(), id);

		SendShopForceSoftCloseDBPacket(ch->GetPlayerID());
		return true;
	}

	bool CShopManager::RecvShopOpenClientPacket(LPCHARACTER ch, DWORD ownerid)
	{
		if(!ch || !ch->GetDesc())
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_OPEN_SHOP))
			return false;

		if (ch->IsBusy(BUSY_SHOP))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
			return false;
		}

		auto shop = GetShopByOwnerID(ownerid);
		if(!shop)
			return false;

#ifdef EXTEND_IKASHOP_PRO
		if (shop->GetDuration() == 0)
			return false;
#endif

		if (!IsNearShop(ch, shop))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_TOO_FAR"));
			return false;
		}

		// patch with warp check
		ch->SetIkarusShopUseTime();

		// skipping owner
		if (ch->GetPlayerID() == ownerid) {
			// sending board counters
			SendBoardCountersClientPacket(ch);
			GetManager().SendShopOpenMyShopClientPacket(ch);
			return true;
		}

		ch->SetIkarusShopGuest(shop);
		SendShopOpenClientPacket(ch, shop.get());
		return true;
	}

	bool CShopManager::RecvShopOpenMyShopClientPacket(LPCHARACTER ch)
	{
		if(!ch || !ch->GetDesc())
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_OPEN_MY_SHOP))
			return false;

		if (ch->IsBusy(BUSY_SHOP_MANAGE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
			return false;
		}

		// patch with warp check
		ch->SetIkarusShopUseTime();

		// sending board counters
		SendBoardCountersClientPacket(ch);

		if (!ch->GetIkarusShop())
			SendShopOpenMyShopNoShopClientPacket(ch);
		else
			SendShopOpenMyShopClientPacket(ch);

		return true;
	}

	bool CShopManager::RecvShopBuyItemClientPacket(LPCHARACTER ch, DWORD ownerid, DWORD itemid, bool searching, long long seenprice)  //patch seen price check
	{
		if(!ch || !CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_BUY_ITEM))
			return false;

		if (!CheckCharacterActions(ch))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot perform this operation right now."));
			return true;
		}

		if (ch->IsBusy(BUSY_SHOP))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
			return false;
		}

		if (!ch->GetIkarusShopGuest())
			return false;

		// searching shop
		SHOP_HANDLE shop = GetShopByOwnerID(ownerid);
		if(!shop){
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_GENERIC_ERROR");
			return false;
		}

#ifdef EXTEND_IKASHOP_PRO
		if(shop->GetDuration() == 0)
			return false;
#endif

		if (!IsNearShop(ch, shop))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_TOO_FAR"));
			return false;
		}

		if (shop->IsEditMode())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The shop is in edit mode."));
			return false;
		}

		if (shop->GetLastEditTime() + 10 > get_global_time())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The shop was recently edited by the owner. Please wait a while."));
			return false;
		}

		// searching item
		auto pitem = shop->GetItem(itemid);
		if(!pitem){
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_ITEM_NOT_AVAILABLE");
			SendSearchResultDeleteClientPacket(ch, itemid);
			return false;
		}

		if (!ch->HasSlotForItem(pitem->GetVnum()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("INVENTORY_FULL_ERROR"));
			return false;
		}

		if(!pitem->CanBuy(ch))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have enough Yang."));
			return false;
		}

		//patch seen price check
		if (pitem->GetPrice().GetTotalYangAmount() != seenprice)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Item price has changed."));
			//SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_BUY_PRICE_CHANGED");
			return false;
		}

		// patch with warp check
		ch->SetIkarusShopUseTime();

		//patch seen price check
		SendShopLockBuyItemDBPacket(ch, ownerid, pitem, seenprice);
		return true;
	}

#ifdef ENABLE_IKASHOP_ENTITIES

	bool CShopManager::RecvShopClickEntityClientPacket(LPCHARACTER ch, DWORD evid)
	{
		if (ch && !PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::OfflineShopClick, std::chrono::milliseconds(500)))
			return true;

		auto iter = m_entityByVID.find(evid);
		if(iter != m_entityByVID.end()){
			auto entity = iter->second;
			RecvShopOpenClientPacket(ch, entity->GetShop()->GetOwnerPID());
		}
		return true;
	}

	bool ikashop::CShopManager::RecvFindMyShopClientPacket(LPCHARACTER ch)
	{
		if (!ch)
			return false;

		if (!ch->IsLookingShopOwner())
			return false;
		
		auto shop = ch->GetIkarusShop();
		if (!shop || shop->GetDuration() < 1)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MYSHOP_NOT_FOUND"));
			return false;
		}

		auto spawn = shop->GetSpawn();
		if (spawn.channel != g_bChannel)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MYSHOP_NOT_THIS_CHANNEL"), spawn.channel);
			return false;
		}

		if (spawn.map != ch->GetMapIndex())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MYSHOP_NOT_THIS_MAP"));
			return false;
		}

		auto entity = shop->GetEntity();
		if (!entity)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MYSHOP_NOT_FOUND"));
		}

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MYSHOP_POSITION_SENT"));
		SendMyShopPositionClientPacket(ch, entity->GetVID(), spawn.x, spawn.y);
		return true;
	}

#ifdef EXTEND_IKASHOP_PRO
	bool CShopManager::RecvMoveShopEntityClientPacket(LPCHARACTER ch) // UNUSED
	{
		if (!ch)
			return false;

		if(!ch->GetIkarusShop() || ch->GetIkarusShop()->GetDuration() == 0)
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_MOVE_CLOSED_SHOP");
			return false;
		}

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_MOVE_SHOP))
			return false;

		if(ch->GetGold() < 100000)
			return false;

		ch->ChangeGold(-100000);

		TShopSpawn spawn{};
		spawn.map = ch->GetMapIndex();
		spawn.x = ch->GetX();
		spawn.y = ch->GetY();
		spawn.channel = g_bChannel;

		// encoding packets to db
		TPacketGDNewIkarusShop pack{};
		pack.bSubHeader = ikashop::SUBHEADER_GD_MOVE_SHOP_ENTITY;

		TSubPacketGDMoveShopEntity subpack{};
		subpack.owner = ch->GetPlayerID();
		subpack.spawn = spawn;
		
		TEMP_BUFFER buffer;
		buffer.write(pack);
		buffer.write(subpack);

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buffer.read_peek(), buffer.size());
		return true;
	}
#endif
#endif

	// The category switch below asks a shop three questions - HasItem,
	// HasItemType, HasSoulStoneSocket - and used to ask them of an offline
	// shop only. A playerbot's stall is a classic private shop
	// (CHARACTER::OpenMyShop with no duration), and the finder answered
	// "Znaleziono 0 sklepow" on a square with three hundred of them
	// (sizowski, 2.0.12). The switch is a template now, asked of an offline
	// shop and of a keeper's counter alike (CPlayerBotStallView).
	template <class SHOP, class FILTERS>
	static bool PlayerBotMatchShopCategory(DWORD category, SHOP shop, const FILTERS& m_shopSearchFilters)
	{
		if (!shop)
			return false;

		switch (category)
		{
			case SHOP_SEARCH_CATEGORY_POLYMORPH * SHOP_CATEGORY_MAX_SUB:
				return shop->HasItem(70104);

			// WEAPON
			case SHOP_SEARCH_CATEGORY_WEAPON* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_WEAPON_ONEHAND:
				return shop->HasItemType(ITEM_WEAPON, WEAPON_SWORD, false);
			case SHOP_SEARCH_CATEGORY_WEAPON_ATTR* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_WEAPON_ONEHAND:
				return shop->HasItemType(ITEM_WEAPON, WEAPON_SWORD, true);

			case SHOP_SEARCH_CATEGORY_WEAPON* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_WEAPON_TWOHAND:
				return shop->HasItemType(ITEM_WEAPON, WEAPON_TWO_HANDED, false);
			case SHOP_SEARCH_CATEGORY_WEAPON_ATTR* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_WEAPON_TWOHAND:
				return shop->HasItemType(ITEM_WEAPON, WEAPON_TWO_HANDED, true);

			case SHOP_SEARCH_CATEGORY_WEAPON* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_WEAPON_DAGGER:
				return shop->HasItemType(ITEM_WEAPON, WEAPON_DAGGER, false);
			case SHOP_SEARCH_CATEGORY_WEAPON_ATTR* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_WEAPON_DAGGER:
				return shop->HasItemType(ITEM_WEAPON, WEAPON_DAGGER, true);

			case SHOP_SEARCH_CATEGORY_WEAPON* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_WEAPON_BOW:
				return shop->HasItemType(ITEM_WEAPON, WEAPON_BOW, false);
			case SHOP_SEARCH_CATEGORY_WEAPON_ATTR* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_WEAPON_BOW:
				return shop->HasItemType(ITEM_WEAPON, WEAPON_BOW, true);

			case SHOP_SEARCH_CATEGORY_WEAPON* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_WEAPON_BELL:
				return shop->HasItemType(ITEM_WEAPON, WEAPON_BELL, false);
			case SHOP_SEARCH_CATEGORY_WEAPON_ATTR* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_WEAPON_BELL:
				return shop->HasItemType(ITEM_WEAPON, WEAPON_BELL, true);

			case SHOP_SEARCH_CATEGORY_WEAPON* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_WEAPON_FAN:
				return shop->HasItemType(ITEM_WEAPON, WEAPON_FAN, false);
			case SHOP_SEARCH_CATEGORY_WEAPON_ATTR* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_WEAPON_FAN:
				return shop->HasItemType(ITEM_WEAPON, WEAPON_FAN, true);

			// ARMOR
			case SHOP_SEARCH_CATEGORY_ARMOR * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_ARMOR_BODY:
				return shop->HasItemType(ITEM_ARMOR, ARMOR_BODY, false);
			case SHOP_SEARCH_CATEGORY_ARMOR_ATTR* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_ARMOR_BODY:
				return shop->HasItemType(ITEM_ARMOR, ARMOR_BODY, true);

			case SHOP_SEARCH_CATEGORY_ARMOR* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_ARMOR_SHIELD:
				return shop->HasItemType(ITEM_ARMOR, ARMOR_SHIELD, false);
			case SHOP_SEARCH_CATEGORY_ARMOR_ATTR* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_ARMOR_SHIELD:
				return shop->HasItemType(ITEM_ARMOR, ARMOR_SHIELD, true);

			case SHOP_SEARCH_CATEGORY_ARMOR* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_ARMOR_HEAD:
				return shop->HasItemType(ITEM_ARMOR, ARMOR_HEAD, false);
			case SHOP_SEARCH_CATEGORY_ARMOR_ATTR* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_ARMOR_HEAD:
				return shop->HasItemType(ITEM_ARMOR, ARMOR_HEAD, true);

			// JEWELRY
			case SHOP_SEARCH_CATEGORY_JEWELRY* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_JEWELRY_EAR:
				return shop->HasItemType(ITEM_ARMOR, ARMOR_EAR, false);
			case SHOP_SEARCH_CATEGORY_JEWELRY_ATTR* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_JEWELRY_EAR:
				return shop->HasItemType(ITEM_ARMOR, ARMOR_EAR, true);

			case SHOP_SEARCH_CATEGORY_JEWELRY* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_JEWELRY_NECK:
				return shop->HasItemType(ITEM_ARMOR, ARMOR_NECK, false);
			case SHOP_SEARCH_CATEGORY_JEWELRY_ATTR* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_JEWELRY_NECK:
				return shop->HasItemType(ITEM_ARMOR, ARMOR_NECK, true);

			case SHOP_SEARCH_CATEGORY_JEWELRY* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_JEWELRY_WRIST:
				return shop->HasItemType(ITEM_ARMOR, ARMOR_WRIST, false);
			case SHOP_SEARCH_CATEGORY_JEWELRY_ATTR* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_JEWELRY_WRIST:
				return shop->HasItemType(ITEM_ARMOR, ARMOR_WRIST, true);

			case SHOP_SEARCH_CATEGORY_JEWELRY* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_JEWELRY_BOOTS:
				return shop->HasItemType(ITEM_ARMOR, ARMOR_FOOTS, false);
			case SHOP_SEARCH_CATEGORY_JEWELRY_ATTR* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_JEWELRY_BOOTS:
				return shop->HasItemType(ITEM_ARMOR, ARMOR_FOOTS, true);

			// SOUL STONES
			case SHOP_SEARCH_CATEGORY_SOULSTONE * SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SOULSTONE_0:
				return shop->HasSoulStoneSocket(0);
			case SHOP_SEARCH_CATEGORY_SOULSTONE* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SOULSTONE_1:
				return shop->HasSoulStoneSocket(1);
			case SHOP_SEARCH_CATEGORY_SOULSTONE* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SOULSTONE_2:
				return shop->HasSoulStoneSocket(2);
			case SHOP_SEARCH_CATEGORY_SOULSTONE* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SOULSTONE_3:
				return shop->HasSoulStoneSocket(3);
			case SHOP_SEARCH_CATEGORY_SOULSTONE* SHOP_CATEGORY_MAX_SUB + SHOP_SEARCH_SUB_SOULSTONE_4:
				return shop->HasSoulStoneSocket(4);

			default:
			{
				auto it = m_shopSearchFilters.find(category);
				if (it != m_shopSearchFilters.end())
				{
					for (auto filter : it->second)
					{
						if (shop->HasItem(filter.itemVnum, filter.socket0))
							return true;
					}
				}

			}
		}
		return false;
	}

	bool CShopManager::SearchItemsByCategory(DWORD category, ikashop::CShopManager::SHOP_HANDLE shop)
	{
		return PlayerBotMatchShopCategory(category, shop, m_shopSearchFilters);
	}

	// A keeper's counter seen through the three questions the category
	// switch asks. The lines are the classic shop's item vector; the item
	// behind each line is the keeper's own (CShop::SHOP_ITEM::pkItem), so
	// type, sockets and bonus lines are read from it as the offline shop
	// reads them from its table.
	class CPlayerBotStallView
	{
	public:
		explicit CPlayerBotStallView(LPCHARACTER keeper) : m_keeper(keeper) {}

		bool HasItem(DWORD itemVnum, int socket0 = 0)
		{
			const std::vector< ::CShop::SHOP_ITEM>& lines = Lines();
			for (size_t i = 0; i < lines.size(); ++i)
			{
				LPITEM item = lines[i].pkItem;
				if (item && item->GetVnum() == itemVnum &&
						(socket0 == 0 || item->GetSocket(0) == (long) socket0))
					return true;
			}
			return false;
		}

		bool HasItemType(BYTE type, BYTE subtype, bool checkAttribute)
		{
			const std::vector< ::CShop::SHOP_ITEM>& lines = Lines();
			for (size_t i = 0; i < lines.size(); ++i)
			{
				LPITEM item = lines[i].pkItem;
				if (!item || item->GetType() != type || item->GetSubType() != subtype)
					continue;
				if (checkAttribute && item->GetAttributeType(0) == 0)
					continue;
				return true;
			}
			return false;
		}

		bool HasSoulStoneSocket(BYTE level)
		{
			const std::vector< ::CShop::SHOP_ITEM>& lines = Lines();
			for (size_t i = 0; i < lines.size(); ++i)
			{
				LPITEM item = lines[i].pkItem;
				if (item && item->GetVnum() >= (28030 + 100 * level) && item->GetVnum() <= (28043 + 100 * level))
					return true;
			}
			return false;
		}

	private:
		const std::vector< ::CShop::SHOP_ITEM>& Lines()
		{
			static const std::vector< ::CShop::SHOP_ITEM> s_none;
			::CShop* shop = m_keeper ? m_keeper->GetMyShop() : NULL;
			return shop ? shop->GetItemVector() : s_none;
		}

		LPCHARACTER m_keeper;
	};

	// The keepers with a counter open on this map within the finder's reach,
	// matched like the offline shops above. The client marks a result by the
	// entity's VID and its position; a keeper is an entity like any other.
	template <class FILTERS>
	static void PlayerBotSearchStalls(LPCHARACTER ch, DWORD category, const FILTERS& filters,
			std::vector<TSubPacketGCShopSearchItemShop>& foundShops)
	{
		const CHARACTER_MANAGER::NAME_MAP& pcs = CHARACTER_MANAGER::instance().GetPCMap();
		for (CHARACTER_MANAGER::NAME_MAP::const_iterator it = pcs.begin();
				it != pcs.end() && foundShops.size() < 400; ++it)
		{
			LPCHARACTER keeper = it->second;
			if (!keeper || keeper == ch || !keeper->GetMyShop() ||
					keeper->GetMapIndex() != ch->GetMapIndex() ||
					ch->DistanceTo(keeper->GetX(), keeper->GetY()) > 7500)
				continue;
			CPlayerBotStallView view(keeper);
			if (!PlayerBotMatchShopCategory(category, &view, filters))
				continue;
			TSubPacketGCShopSearchItemShop found{};
			found.shopVid = keeper->GetVID();
			found.x = keeper->GetX();
			found.y = keeper->GetY();
			foundShops.push_back(found);
			// The client marks a result only when the VID is one of its own
			// offline-shop entities - ikashop keeps that list from its spawn
			// packet - so a keeper's VID lists the stall and marks nothing:
			// "found 16 shops, none highlighted, none on the map" (sizowski,
			// 12 September). What a keeper can get is a SPECIAL_EFFECT packet, sent
			// to the searcher alone, on every keeper found. Not the level-up one:
			// SE_LEVELUP_ON_14_FOR_GERMANY is, on this client, the German promo
			// text ("Noch 1 Level-Up! ... siehe www.metin2.de") drawn over the
			// character - 2.0.15 hung it over every stall found (vasils.). The
			// firework is a plain effect on every client.
			if (ch->GetDesc())
			{
				TPacketGCSpecialEffect effect{};
				effect.header = HEADER_GC_SEPCIAL_EFFECT;
				effect.type = SE_CHINA_FIREWORK;
				effect.vid = keeper->GetVID();
				ch->GetDesc()->Packet(&effect, sizeof(effect));
			}
		}
	}

	bool CShopManager::RecvShopSearchItemClientPacket(LPCHARACTER ch, DWORD itemVnum, int socket0)
	{
		if (!ch)
			return false;

		if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::ShopSearch, std::chrono::milliseconds(10000)))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("NEXT_ACTION_WAIT_NO_FORMAT"));
			return false;
		}

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_SEARCH_ITEM))
			return false;

		if (itemVnum >= SHOP_SEARCH_CATEGORY_MAX * SHOP_CATEGORY_MAX_SUB)
			return false;

		std::vector<TSubPacketGCShopSearchItemShop> foundShops{};
		for (auto it : m_mapShops)
		{
			auto shop = it.second;
			if (shop)
			{
				if (shop->GetSpawn().channel != g_bChannel || shop->GetSpawn().map != ch->GetMapIndex())
					continue;

				if (shop->GetDuration() < 1)
					continue;

				if (shop->GetOwnerPID() == ch->GetPlayerID())
					continue;

				if (!shop->GetEntity())
					continue;

				auto pos = shop->GetEntity()->GetXYZ();
				if (ch->DistanceTo(pos.x, pos.y) > 7500)
					continue;

				if (!SearchItemsByCategory(itemVnum, shop))
				{
					continue;
				}

				//if (shop->HasItem(itemVnum, socket0)
				{
					TSubPacketGCShopSearchItemShop found{};
					found.shopVid = shop->GetEntity()->GetVID();
					found.x = shop->GetSpawn().x;
					found.y = shop->GetSpawn().y;
					foundShops.push_back(found);

					if (foundShops.size() >= 400)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Searching was stopped. Found above %d shops with specified item."), 400);
						break;
					}

					continue;
				}
			}
		}

		// The playerbots' stalls, after the offline shops. They are listed
		// and lit up, not drawn on the map, and the line says so.
		if (foundShops.size() < 400)
		{
			const size_t offlineShops = foundShops.size();
			PlayerBotSearchStalls(ch, itemVnum, m_shopSearchFilters, foundShops);
			if (foundShops.size() > offlineShops)
				ch->ChatPacket(CHAT_TYPE_INFO, "Stragany botow z tym towarem: %d - kazdy oznaczony fajerwerkiem.",
						(int)(foundShops.size() - offlineShops));
		}

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Searching ended. Found %d shops."), foundShops.size());

		if (foundShops.size() > 0)
		{
			TPacketGCNewIkarusShop pack;
			pack.header = HEADER_GC_NEW_OFFLINESHOP;
			pack.size = sizeof(pack) + sizeof(TSubPacketGCShopSearchItem) + sizeof(TSubPacketGCShopSearchItemShop) * foundShops.size();
			pack.subheader = ikashop::SUBHEADER_GC_SHOP_SEARCH;

			TSubPacketGCShopSearchItem subpack{};
			subpack.itemCount = foundShops.size();

			TEMP_BUFFER buff;
			buff.write(&pack, sizeof(pack));
			buff.write(&subpack, sizeof(subpack));

			for (auto& it : foundShops)
			{
				buff.write(&it, sizeof(it));
			}

			ch->GetDesc()->Packet(buff.read_peek(), buff.size());
		}
		
		return true;
	}

	void CShopManager::SendSearchResultDeleteClientPacket(LPCHARACTER ch, DWORD itemid)
	{
		if (!ch || !ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack;
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.size = sizeof(pack) + sizeof(DWORD);
		pack.subheader = ikashop::SUBHEADER_GC_SEARCH_RESULT_DELETE;

		ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
		ch->GetDesc()->Packet(&itemid, sizeof(itemid));
	}

	void CShopManager::SendShopOpenClientPacket(LPCHARACTER ch, ikashop::CShop* shop)
	{
		if (!ch->GetDesc())
			return;

		const auto& items = shop->GetItems();

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader	= SUBHEADER_GC_SHOP_OPEN;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCShopOpen) + sizeof(TShopItem)* items.size();

		TSubPacketGCShopOpen subpack{};
		if (shop->GetEntity())
			subpack.vid = shop->GetEntity()->GetVID();
		else
			subpack.vid = 0;

		subpack.isOwnerOnline = shop->IsOwnerOnline();
		subpack.shop.count = shop->GetItems().size();
		subpack.shop.duration = shop->GetGuestDuration();
		subpack.shop.ownerid = shop->GetOwnerPID();
		subpack.shop.unlockCount = shop->GetUnlockCount();
		subpack.shop.isPremium = shop->IsPremium();
		str_to_cstring(subpack.shop.name, shop->GetName());
		str_to_cstring(subpack.shop.ownerName, shop->GetOwnerName());

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		for (auto& [id, item] : items)
		{
			auto& itemInfo = item->GetInfo();
			buff.write(&itemInfo, sizeof(itemInfo));
		}

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}

	void CShopManager::SendShopOpenMyShopNoShopClientPacket(LPCHARACTER ch)
	{
		if (!ch->GetDesc())
			return;

		ch->SetLookingShopOwner(false);

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader	= SUBHEADER_GC_SHOP_OPEN_OWNER_NO_SHOP;
		pack.size = sizeof(pack);

		ch->GetDesc()->Packet(&pack, sizeof(pack));
	}

	void CShopManager::SendShopExpiredGuesting(DWORD ownerid, LPCHARACTER ch)
	{
		if (!ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader = SUBHEADER_GC_SHOP_EXPIRED_GUESTING;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCShopExpiredGuesting);

		TSubPacketGCShopExpiredGuesting subpack{};
		subpack.ownerid = ownerid;

		ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
		ch->GetDesc()->Packet(&subpack, sizeof(subpack));
	}

	void CShopManager::SendShopOpenMyShopClientPacket(LPCHARACTER ch)
	{
		// validating character and shop
		if (!ch->GetDesc())
			return;
		if(!ch->GetIkarusShop())
			return;

		// storing looking state
		ch->SetLookingShopOwner(true);

		// getting shop
		auto shop = ch->GetIkarusShop();
		auto owner = ch->GetPlayerID();

		const auto& items = shop->GetItems();

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader	= SUBHEADER_GC_SHOP_OPEN_OWNER;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCShopOpenOwner) + sizeof(TShopItem) * items.size();

		TSubPacketGCShopOpenOwner subpack{};
		int tax = quest::CQuestManager::instance().GetEventFlag("offline_shop_tax");
		subpack.tax = tax == 0 ? OFFLINESHOP_DEFAULT_TAX_PCT : tax;
		subpack.shop.count = items.size();
		subpack.shop.duration = shop->GetDuration();
		subpack.shop.ownerid = owner;
		str_to_cstring(subpack.shop.name, shop->GetName());
		str_to_cstring(subpack.shop.ownerName, shop->GetOwnerName());

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));		

		for (auto& [id, item] : items)
		{			
			auto& itemInfo = item->GetInfo();
			buff.write(&itemInfo, sizeof(itemInfo));
		}

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}

	void CShopManager::SendShopForceClosedClientPacket(DWORD ownerid)
	{
		if(auto ch = CHARACTER_MANAGER::instance().FindByPID(ownerid))
		{
			if(ch->GetDesc())
			{
				TPacketGCNewIkarusShop pack{};
				pack.header = HEADER_GC_NEW_OFFLINESHOP;
				pack.size = sizeof(pack);
				pack.subheader = SUBHEADER_GC_SHOP_OPEN_OWNER_NO_SHOP;
				ch->GetDesc()->Packet(&pack, sizeof(pack));
			}
		}		
	}

	void CShopManager::SendMyShopPositionClientPacket(LPCHARACTER ch, DWORD shopVID, long x, long y)
	{
		if (!ch)
			return;

		if (ch->GetDesc())
		{
			TPacketGCNewIkarusShop pack{};
			pack.header = HEADER_GC_NEW_OFFLINESHOP;
			pack.size = sizeof(pack);
			pack.subheader = SUBHEADER_GC_SHOP_POSITION;

			TSubPacketGCShopPosition subpack{};
			subpack.vid = shopVID;
			subpack.x = x;
			subpack.y = y;

			TEMP_BUFFER buff;
			buff.write(&pack, sizeof(pack));
			buff.write(&subpack, sizeof(subpack));

			ch->GetDesc()->Packet(buff.read_peek(), buff.size());
		}
	}

	void CShopManager::ProcessAddItemToShop(TShopItem& sitem, LPCHARACTER ch, LPITEM item, WORD destPos, const TPriceInfo price)
	{
		sitem.pos = destPos;
		sitem.id = item->GetID();
		sitem.owner = ch->GetPlayerID();
		sitem.vnum = item->GetVnum();
		sitem.count = item->GetCount();
		sitem.expiration = GetItemExpiration(item);
		sitem.price = price;
#ifdef ENABLE_CHANGELOOK_SYSTEM
		sitem.item.dwTransmutation = item->GetTransmutation();
#endif
		memcpy(sitem.aAttr, item->GetAttributes(), sizeof(sitem.aAttr));
		memcpy(sitem.alSockets, item->GetSockets(), sizeof(sitem.alSockets));

		MoveAndDestroyInstance(item, IKASHOP_OFFLINESHOP, destPos);
	}

	//ITEMS
	bool CShopManager::RecvShopAddItemClientPacket(LPCHARACTER ch, const TItemPos& pos, const TPriceInfo& price, int destPos)
	{
		if(!ch || !ch->GetIkarusShop())
			return false;

		if (!ch->IsLookingShopOwner())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("OFFLINE_SHOP_MANAGE_MUST_BE_OPEN"));
			return false;
		}

		auto shop = ch->GetIkarusShop();

		if(!CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_ADD_ITEM))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("NEXT_ACTION_WAIT_NO_FORMAT"));
			return false;
		}

		if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::IkaShopTouchItem, std::chrono::milliseconds(200)))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("NEXT_ACTION_WAIT_NO_FORMAT"));
			return false;
		}

		if (!CheckSafeboxSize(ch))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Blad depozytu sklepu. Zglos administracji (kod: check_safe)");
			return false;
		}

		if (!CheckCharacterActions(ch))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot perform this operation right now."));
			return true;
		}

		if (ch->IsBusy(BUSY_SHOP_MANAGE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
			return false;
		}

		if (!shop->IsEditMode())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The shop is not in edit mode."));
			return false;
		}

		// patch with warp check
		ch->SetIkarusShopUseTime();

#ifdef EXTEND_IKASHOP_PRO
		if (shop->GetDuration() == 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("OFFLINE_SHOP_IS_CLOSED"));
			return false;
		}
#endif

		if (!IsNearShop(ch, shop, OWNER_INTERRACT_DISTANCE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_TOO_FAR"));
			return false;
		}

		if (shop->GetTotalYangValue() + price.GetTotalYangAmount() > GOLD_MAX)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MONEY_WILL_EXCEED_OVER_LIMIT"));
			return false;
		}

		auto item = ch->GetItem(pos);
		if(!item)
			return false;

		if (!ch->CanAddItemToShop(item, destPos))
			return false;

		if (!IsGoodSalePrice(price))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_ITEM_WRONG_PRICE%s"), item->GetName());
			return false;
		}

		// checking space
		if (!shop->ReserveSpace(destPos, item->GetSize()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_ITEM_WRONG_POS"));
			ch->ChatDebug("pos is reserverd %d (item size %d)", destPos, item->GetSize());
			return false;
		}

		if ((ch->GetPremiumRemainSeconds(PREMIUM_SHOP) > 0) != shop->IsPremium() ||
			ch->GetShopUnlockedProgress() != shop->GetUnlockCount())
		{
			SendShopLockedSlotStateDBPacket(ch->GetPlayerID(), ch->GetShopUnlockedProgress(), ch->GetPremiumRemainSeconds(PREMIUM_SHOP) > 0);
		}

		TShopItem sitem{};
		ProcessAddItemToShop(sitem, ch, item, destPos, price);
		SendShopAddItemDBPacket(ch->GetPlayerID(), sitem);
		return true;
	}

	bool CShopManager::RecvShopRemoveItemClientPacket(LPCHARACTER ch, DWORD itemid)
	{
		if(!ch || !ch->GetIkarusShop())
			return false;

		if (!ch->IsLookingShopOwner())
			return false;

		if(!CheckGMLevel(ch))
			return false;

		if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::IkaShopTouchItem, std::chrono::milliseconds(200)))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("NEXT_ACTION_WAIT_NO_FORMAT"));
			return false;
		}

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_REMOVE_ITEM))
			return false;

		if (ch->IsBusy(BUSY_SHOP_MANAGE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
			return false;
		}

		// checking if it is the last item
		auto shop = ch->GetIkarusShop();
		if (!shop)
			return false;

		if (shop->GetDuration() > 0 && !shop->IsEditMode())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The shop is not in edit mode."));
			return false;
		}

		if (!IsNearShop(ch, shop, OWNER_INTERRACT_DISTANCE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_TOO_FAR"));
			return false;
		}

		auto shopItem = shop->GetItem(itemid);
		// checking shop has the item
		if (!shopItem)
			return false;

#ifdef ENABLE_MT2009_DISABLE_IKASHOP_DEFAULT_SAFEBOX
		if (!ch->HasSlotForItem(shopItem->GetVnum()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("INVENTORY_FULL_ERROR"));
			return false;
		}
#else
		if (!CheckSafeboxSize(ch))
			return false;
#endif

		// patch with warp check
		ch->SetIkarusShopUseTime();
		SendShopRemoveItemDBPacket(shop->GetOwnerPID(), itemid);
		return true;
	}

	bool CShopManager::RecvShopRemoveAllItemClientPacket(LPCHARACTER ch)
	{
		if (!ch || !ch->GetIkarusShop())
			return false;

		if (!ch->IsLookingShopOwner())
			return false;

		if (!CheckGMLevel(ch))
			return false;

		if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::IkaShopTouchItem, std::chrono::milliseconds(1000)))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("NEXT_ACTION_WAIT_NO_FORMAT"));
			return false;
		}

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_REMOVE_ALL_ITEM))
			return false;

		if (ch->IsBusy(BUSY_SHOP_MANAGE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
			return false;
		}

		// checking if it is the last item
		auto shop = ch->GetIkarusShop();
		if (!shop)
			return false;

		if (shop->GetDuration() > 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The shop is open."));
			return false;
		}

		if (!IsNearShop(ch, shop, OWNER_INTERRACT_DISTANCE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_TOO_FAR"));
			return false;
		}

		// patch with warp check
		ch->SetIkarusShopUseTime();

		bool showNoInventorySpace = false;

		BYTE bCheckItemGrid[INVENTORY_DEFAULT_MAX_NUM]{};
		auto playerInventoryGrid = ch->GetInventoryItemGrid();
		for (int i = 0; i < INVENTORY_DEFAULT_MAX_NUM; i++)
		{
			bCheckItemGrid[i] = playerInventoryGrid[i];
		}

		auto items = shop->GetItems();
		for (auto item : items)
		{
			if (item.second)
			{
				const auto pTable = item.second->GetTable();

				int iEmptySlot = -1;
				for (int cell = 0; cell < INVENTORY_DEFAULT_MAX_NUM; cell++)
				{
					if (is_empty_page_grid(bCheckItemGrid, cell, pTable->bSize, -1, INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW, INVENTORY_DEFAULT_PAGE_COUNT))
					{
						iEmptySlot = cell;
						break;
					}
				}

				if (iEmptySlot < 0)
				{
					showNoInventorySpace = true;
					continue;
				}

				for (int i = 0; i < pTable->bSize; i++)
				{
					auto bSlot = iEmptySlot + i * INVENTORY_PAGE_COLUMN;
					if (bSlot < INVENTORY_DEFAULT_MAX_NUM)
						bCheckItemGrid[bSlot] = iEmptySlot + 1;
					else
					{
						showNoInventorySpace = true;
						continue;
					}
				}

				SendShopRemoveItemDBPacket(shop->GetOwnerPID(), item.second->GetID());
			}
		}

		if (showNoInventorySpace)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("INVENTORY_FULL_ERROR"));

		return true;
	}

	bool CShopManager::RecvShopEditItemDBPacket(DWORD ownerid, DWORD itemid, const TPriceInfo& price)
	{
		auto shop = GetShopByOwnerID(ownerid);
		if (!shop)
			return false;

		shop->ModifyItemPrice(itemid, price);
		playerbot_offline::Complete(ownerid, playerbot_offline::Edit, itemid);
		return true;
	}

	//FILTER
	bool CShopManager::RecvShopFilterRequestClientPacket(LPCHARACTER ch, const TFilterInfo& filter)
	{
		if(!ch)
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_FILTER_REQUEST))
			return false;

		if (!CheckSearchCooldown(ch->GetPlayerID()))
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_SEARCH_COOLDOWN_STILL_ACTIVED");
			return true;
		}

		// patch with warp check
		ch->SetIkarusShopUseTime();

		std::vector<TSearchResultInfo> matches;
		const auto filtername = StringToLower(filter.name, strnlen(filter.name, sizeof(filter.name)));

		for (auto& [id, auction] : m_mapAuctions)
		{
			// skipping my own items
			if (auction->GetInfo().owner == ch->GetPlayerID())
				continue;

			const auto& info = auction->GetInfo();
			if (!MatchFilter(info, filter, filtername))
				continue;

			auto& result = matches.emplace_back(DerivedFromBase<TSearchResultInfo, TShopItem>(info));
			result.auction = true;
			result.duration = auction->GetInfo().duration;
			str_to_cstring(result.seller_name, auction->GetInfo().ownername);
		}

		for (auto& [id, shop] : m_mapShops)
		{
			// skipping my own items
			if(shop->GetOwnerPID() == ch->GetPlayerID())
				continue;

#ifdef EXTEND_IKASHOP_PRO
			if (shop->GetDuration() == 0)
				continue;
#endif

			for (auto& [id, sitem] : shop->GetItems())
			{
				const auto& info = sitem->GetInfo();
				if(!MatchFilter(info, filter, filtername))
					continue;

				auto& result = matches.emplace_back(DerivedFromBase<TSearchResultInfo>(info));
				result.auction = false;
				result.duration = shop->GetDuration();
				str_to_cstring(result.seller_name, shop->GetOwnerName());
				if (matches.size() >= OFFLINESHOP_MAX_SEARCH_RESULT)
					break;
			}

			if(matches.size() >= OFFLINESHOP_MAX_SEARCH_RESULT)
				break;
		}

		SendShopFilterResultClientPacket(ch, matches);
		return true;
	}

	bool CShopManager::RecvShopSearchFillRequestClientPacket(LPCHARACTER ch)
	{
		if (!ch)
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_FILTER_REQUEST))
			return false;

		if (!CheckSearchCooldown(ch->GetPlayerID()))
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_SEARCH_COOLDOWN_STILL_ACTIVED");
			return true;
		}

		// patch with warp check
		ch->SetIkarusShopUseTime();

		std::vector<TSearchResultInfo> matches;
		std::set<DWORD> matchedIds;

		for (int i = 0; i < 20; i++)
		{
			const auto shopCount = m_mapShops.size();
			if(shopCount == 0)
				break;

			auto shopIter = std::next(m_mapShops.begin(), number(0, shopCount-1));
			auto& shop = shopIter->second;

			// skipping my own items
			if (shop->GetOwnerPID() == ch->GetPlayerID())
				continue;

#ifdef EXTEND_IKASHOP_PRO
			if (shop->GetDuration() == 0)
				continue;
#endif

			auto& items = shop->GetItems();
			const auto itemCount = items.size();
			if(itemCount == 0)
				continue;

			auto itemIter = std::next(items.begin(), number(0, itemCount-1));
			auto& item = itemIter->second;

			if(matchedIds.contains(item->GetInfo().id))
				continue;

			const auto& info = item->GetInfo();
			auto& result = matches.emplace_back(DerivedFromBase<TSearchResultInfo>(info));
			result.auction = false;
			result.duration = shop->GetDuration();
			str_to_cstring(result.seller_name, shop->GetOwnerName());
			matchedIds.emplace(info.id);
		}

		for(int i = 0; i < 5; i++)
		{
			const auto auctionCount = m_mapAuctions.size();
			if (auctionCount == 0)
				break;

			auto auctionIter = std::next(m_mapAuctions.begin(), number(0, auctionCount - 1));
			auto& auction = auctionIter->second;

			// skipping my own items
			if (auction->GetInfo().owner == ch->GetPlayerID())
				continue;

			auto bestoff = auction->GetBestOffer();

			if (matchedIds.contains(auction->GetInfo().id))
				continue;

			const auto& info = auction->GetInfo();
			auto& result = matches.emplace_back(DerivedFromBase<TSearchResultInfo, TShopItem>(info));
			result.auction = true;
			result.price.yang = GetAuctionMinRaise(auction);
			result.duration = auction->GetInfo().duration;
			str_to_cstring(result.seller_name, auction->GetInfo().ownername);
			matchedIds.emplace(info.id);
		}

		SendShopFilterResultClientPacket(ch, matches);
		return true;
	}

	void CShopManager::SendShopFilterResultClientPacket(LPCHARACTER ch, const std::vector<TSearchResultInfo>& items)
	{
		if(!ch || !ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader = SUBHEADER_GC_SHOP_FILTER_RESULT;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCShopFilterResult) + sizeof(TSearchResultInfo)*items.size();

		TSubPacketGCShopFilterResult subpack{};
		subpack.count = items.size();

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		if(!items.empty())
			buff.write(items.data(), items.size() * sizeof(items[0]));

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}

	//OFFERS
	bool CShopManager::RecvShopCreateOfferClientPacket(LPCHARACTER ch, TOfferInfo offer)
	{
		if(!ch)
			return false;

#ifdef ENABLE_MT2009_IKASHOP_RESTRICTIONS
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("FEATURE_UNAVAILABLE"));
		return false;
#endif

		if(!CheckGMLevel(ch))
			return false;

		if (!CheckSafeboxSize(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_CREATE_OFFER))
			return false;

		// checking pending offer limit
		auto offersIter = m_mapOffersByBuyer.find(ch->GetPlayerID());
		if (offersIter != m_mapOffersByBuyer.end() && offersIter->second.size() >= OFFLINESHOP_MAX_PENDING_OFFERS)
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_PENDING_PRIVATE_OFFER_LIMIT");
			return false;
		}

		// checking for existing offers on this item
		if (offersIter != m_mapOffersByBuyer.end())
		{
			auto& offers = offersIter->second;
			auto iter = std::find_if(offers.begin(), offers.end(),
				[&offer](const OFFER_HANDLE& handle){ return handle->itemid == offer.itemid; });
			if(iter != offers.end())
			{
				SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_PENDING_PRIVATE_OFFER_ALREADY_EXIST");
				return false;
			}
		}

		// stopping me to offer to myself
		if(ch->GetPlayerID() == offer.ownerid)
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_GENERIC_ERROR");
			return false;
		}

		// searching shop
		auto shop = GetShopByOwnerID(offer.ownerid);
		if(!shop)
			return false;

#ifdef EXTEND_IKASHOP_PRO
		if (shop->GetDuration() == 0)
			return false;
#endif

		// searching item
		auto item = shop->GetItem(offer.itemid);
		if(!item)
			return false;

		// checking price min
		if(item->GetPrice().GetTotalYangAmount() < 10000)
			return false;

		// patch with warp check
		ch->SetIkarusShopUseTime();

		// fix flooding offers
		if (!CheckOfferCooldown(ch->GetPlayerID()))
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CREATE_OFFER_COOLDOWN_STILL_ACTIVED");
			return false;
		}
		
		if (!CheckOfferPrice(offer.price, item->GetPrice()))
			return false;

#ifdef ENABLE_CHEQUE_SYSTEM
		if (offer.price.yang < 0 || offer.price.cheque < 0)
			return false;
#else
		if (offer.price.yang < 0)
			return false;
#endif

		if (offer.price.GetTotalYangAmount() <= 0)
			return false;

		if( (long long) ch->GetGold() < offer.price.yang)
			return false;

#ifdef ENABLE_CHEQUE_SYSTEM
		if (ch->GetCheque() < offer.price.cheque)
			return false;
#endif

		ch->ChangeGold(-offer.price.yang);
#ifdef ENABLE_CHEQUE_SYSTEM
		ch->PointChange(POINT_CHEQUE, -offer.price.cheque);
#endif
		offer.accepted = false;
		offer.notified = false;
		offer.buyerid	= ch->GetPlayerID();
#ifdef ENABLE_CHEQUE_SYSTEM
		offer.price.yang = offer.price.GetTotalYangAmount();
		offer.price.cheque = 0;
#endif
		str_to_cstring(offer.buyername, ch->GetName());

		SendShopOfferNewDBPacket(offer, item);
		return true;
	}

	bool CShopManager::RecvShopAcceptOfferClientPacket(LPCHARACTER ch, DWORD offerid)
	{
		if(!ch || !ch->GetIkarusShop())
			return false;

		if(!CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_ACCEPT_OFFER))
			return false;

		auto shop = ch->GetIkarusShop();
		auto& offers = shop->GetOffers();

#ifdef EXTEND_IKASHOP_PRO
		if (shop->GetDuration() == 0)
			return false;
#endif

		// searching offer
		auto offerIter = offers.find(offerid);
		if(offerIter == offers.end())
			return false;

		// chekcing item was already accepted
		auto info = offerIter->second;
		if(info->accepted)
			return false;

		// checking that accepting character is the owner of the shop
		if(ch->GetPlayerID() != info->ownerid)
			return false;

		// checking item exists
		if(!shop->GetItem(info->itemid))
			return false;

		// patch with warp check
		ch->SetIkarusShopUseTime();

		// forwarding to db
		info->accepted = true;
		SendShopOfferAcceptDBPacket(*info);
		return true;
	}

	bool CShopManager::RecvShopCancelOfferClientPacket(LPCHARACTER ch, DWORD offerid, DWORD ownerid)
	{
		if(!ch || !CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_CANCEL_OFFER))
			return false;

		// searching shop
		auto shop = GetShopByOwnerID(ownerid);
		if(!shop)
			return false;

#ifdef EXTEND_IKASHOP_PRO
		if (shop->GetDuration() == 0)
			return false;
#endif

		// searching offer
		auto& offers = shop->GetOffers();
		auto offerIter = offers.find(offerid);
		if(offerIter == offers.end())
			return false;

		// checking offer wasnt accepted
		auto offer = offerIter->second;
		if(offer->accepted)
			return false;

		// checking item exists
		if(!shop->GetItem(offer->itemid))
			return false;

		// checking who is rejecting the offer is the owner of the shop or the buyer
		if(ch->GetPlayerID() != shop->GetOwnerPID() && ch->GetPlayerID() != offer->buyerid)
			return false;

		// patch with warp check
		ch->SetIkarusShopUseTime();

		
		SendShopOfferCancelDBPacket(*offer, ch->GetPlayerID());
		return true;
	}

	bool CShopManager::RecvOfferListRequestPacket(LPCHARACTER ch) //ikashop-updated 03/08/19
	{
		if (!ch->GetDesc())
			return false;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader = SUBHEADER_GC_OFFER_LIST;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCShopOfferList);

		TSubPacketGCShopOfferList subpack{};
		subpack.offercount = 0;

		// storing offers inside a vector
		std::vector<TOfferListElement> offerlist;

		// searching offers as buyer
		auto it = m_mapOffersByBuyer.find(ch->GetPlayerID());
		if (it != m_mapOffersByBuyer.end())
		{
			auto& offers = it->second;
			for(const auto& offer : offers)
			{
				auto shop = GetShopByOwnerID(offer->ownerid);
				if (!shop)
					continue;

				auto item = shop->GetItem(offer->itemid);
				if (!item)
					continue;

				auto& listElement = offerlist.emplace_back(DerivedFromBase<TOfferListElement>(*offer));
				listElement.item = item->GetInfo();
				listElement.incoming = false;
				str_to_cstring(listElement.shopname, shop->GetName());
			}
		}

		// searching offers as seller
		if(auto shop = ch->GetIkarusShop())
		{
			for (const auto& [id, offer] : shop->GetOffers())
			{
				auto item = shop->GetItem(offer->itemid);
				if (!item)
					continue;

				auto& listElement = offerlist.emplace_back(DerivedFromBase<TOfferListElement>(*offer));
				listElement.item = item->GetInfo();
				listElement.incoming = true;
				str_to_cstring(listElement.shopname, shop->GetName());
			}
		}

		// updating pack & subpack
		pack.size += sizeof(TOfferListElement) * offerlist.size();
		subpack.offercount = offerlist.size();

		// writing bytes into a buffer
		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));
		for(auto& offer : offerlist)
			buff.write(&offer, sizeof(offer));

		// forwarding bytes
		ch->SetLookingIkarusShopOfferList(true);
		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
		return true;
	}

	//SAFEBOX
	bool CShopManager::RecvShopSafeboxOpenClientPacket(LPCHARACTER ch)
	{
		if(!ch || ch->GetIkarusShopSafebox())
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_SAFEBOX_OPEN))
			return false;

		auto safebox = GetShopSafeboxByOwnerID(ch->GetPlayerID());
		if(!safebox)
			return false;

		// patch with warp check
		ch->SetIkarusShopUseTime();

		ch->SetIkarusShopSafebox(safebox);
		safebox->RefreshToOwner(ch);
		return true;
	}

	bool CShopManager::RecvShopSafeboxGetItemClientPacket(LPCHARACTER ch, DWORD itemid)
	{
		if(!ch || !ch->GetIkarusShopSafebox())
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_SAFEBOX_GET_ITEM))
			return false;

		// searching safebox & item
		auto safebox = ch->GetIkarusShopSafebox();
		auto sitem = safebox->GetItem(itemid);
		if(!sitem)
			return false;

		// creating temporary item
		auto item = sitem->CreateItem();
		if(!item)
			return false;

		// checking item can be stored
		TItemPos itemPos;
		auto ret = ch->CanTakeInventoryItem(item, &itemPos);
		
		// destroying temporary item
		item->SetSkipSave(true);
		M2_DESTROY_ITEM(item);

		// if item can't be store returning false
		if (!ret){
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_SAFEBOX_CANNOT_GET_ITEM");
			return false;
		}

		// patch with warp check
		ch->SetIkarusShopUseTime();
		SendShopSafeboxGetItemDBPacket(ch->GetPlayerID(), itemid);
		return true;
	}

	bool CShopManager::RecvShopSafeboxGetValutesClientPacket(LPCHARACTER ch)
	{
		if(!ch || !ch->GetIkarusShopSafebox())
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_SAFEBOX_GET_VALUTES))
			return false;

		if (ch->IsBusy(BUSY_SHOP_MANAGE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
			return false;
		}

		// getting safebox
		auto safebox = ch->GetIkarusShopSafebox();
		if(!safebox)
			return false;

		const auto& ownedValute = safebox->GetValutes();
		auto claimingValute = ownedValute;

#ifdef ENABLE_CHEQUE_SYSTEM
		int64_t claimingYang = claimingValute.yang;
		int64_t claimingCheque = 0;
#endif

		if(claimingValute.yang + static_cast<int64_t>(ch->GetGold()) >= GOLD_MAX)
		{
#ifdef ENABLE_CHEQUE_SYSTEM
			// checking about yang & cheque overflow
			const int64_t yangSpace = GOLD_MAX - ch->GetGold() - 1;
			const int64_t chequeSpace = CHEQUE_MAX - ch->GetCheque() - 1;
			
			// getting claimable cheque
			claimingCheque = std::min(chequeSpace, claimingValute.yang / YANG_PER_CHEQUE);
			claimingYang = std::min(yangSpace, claimingValute.yang - claimingCheque*YANG_PER_CHEQUE);
			
			if(claimingCheque < 0)
				return false;
			if(claimingYang < 0)
				return false;
			if(claimingCheque == 0 && claimingYang == 0)
				return false;

			claimingValute.yang = claimingYang + claimingCheque * YANG_PER_CHEQUE;
#else
			claimingValute.yang = GOLD_MAX - ch->GetGold() - 1;
			if (claimingValute.yang <= 0)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("MONEY_WILL_EXCEED_OVER_LIMIT"));
				return false;
			}
#endif
		}


		if (!safebox->RemoveValute(claimingValute))
			return false;

		// updating db
		SendShopSafeboxGetValutesDBPacket(ch->GetPlayerID(), claimingValute);

		if (ch->IsLookingShopOwner())
			SendBoardCountersClientPacket(ch);

		// patch with warp check
		ch->SetIkarusShopUseTime();
		safebox->RefreshToOwner();

#ifdef ENABLE_CHEQUE_SYSTEM
		if (claimingYang != 0)
			ch->PointChange(POINT_GOLD, claimingYang);
		if (claimingCheque != 0)
			ch->PointChange(POINT_CHEQUE, claimingCheque);
#else
		ch->ChangeGold(claimingValute.yang);
#endif
		return true;
	}

	bool CShopManager::RecvShopSafeboxCloseClientPacket(LPCHARACTER ch)
	{
		if(!ch)
			return false;

		ch->SetIkarusShopSafebox(NULL);
		return true;
	}

	void CShopManager::SendShopSafeboxRefreshClientPacket(LPCHARACTER ch, const TValutesInfo& valute, const std::vector<ITEM_HANDLE>& vec)
	{
		if (!ch->GetDesc())
			return;

		if(!ch || !ch->GetIkarusShopSafebox())
			return;

		// patch with warp check
		ch->SetIkarusShopUseTime();

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCShopSafeboxRefresh) + sizeof(TShopPlayerItem) * vec.size();
		pack.subheader = SUBHEADER_GC_SHOP_SAFEBOX_REFRESH;

		TSubPacketGCShopSafeboxRefresh subpack{};
		subpack.itemcount = vec.size();
		subpack.valute = valute;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));
		
		for (auto& sitem : vec)
		{
			auto& info = static_cast<const TShopPlayerItem&>(sitem->GetInfo());
			buff.write(&info, sizeof(info));
		}

		ch->GetDesc()->Packet(buff.read_peek() , buff.size());
	}

	bool CShopManager::RecvAuctionListRequestClientPacket(LPCHARACTER ch)
	{
		if(!ch)
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_AUCTION_LIST_REQUEST))
			return false;

		std::vector<TAuctionListElement> vec;
		vec.reserve(m_mapAuctions.size());

		for (auto& [id, auction] : m_mapAuctions)
		{
			auto bestOffer = auction->GetBestOffer();

			static TAuctionListElement temp;
			temp.auction = auction->GetInfo();
			temp.best = bestOffer ? bestOffer->price : temp.auction.price;
			temp.offercount = auction->GetOffers().size();
			vec.push_back(temp);
		}

		SendAuctionListClientPacket(ch, vec);
		return true;
	}

	bool CShopManager::RecvAuctionOpenRequestClientPacket(LPCHARACTER ch, DWORD ownerid)
	{
		auto it = m_mapAuctions.find(ownerid);
		if(it == m_mapAuctions.end())
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_AUCTION_OPEN_REQUEST))
			return false;

		if(ch->GetPlayerID() == ownerid)
			return RecvMyAuctionOpenRequestClientPacket(ch);

		// patch with warp check
		ch->SetIkarusShopUseTime();

		auto& auction = it->second;
		ch->SetIkarusAuctionGuest(auction);
		return true;
	}

	bool CShopManager::RecvMyAuctionOpenRequestClientPacket(LPCHARACTER ch)
	{
		// patch with warp check
		ch->SetIkarusShopUseTime();
		
		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_MY_ACUTION_OPEN_REQUEST))
			return false;

		// refreshing to owner
		if(auto auction = ch->GetIkarusAuction())
		{
			ch->SetLookingAuctionOwner(true);
			auction->RefreshToOwner(ch);
		}

		else
			SendAuctionOpenMyAuctionNoAuctionClientPacket(ch);
		
		return true;
	}

	bool CShopManager::RecvMyAuctionCloseClientPacket(LPCHARACTER ch)
	{
		// patch with warp check
		ch->SetIkarusShopUseTime();
		ch->SetLookingAuctionOwner(false);
		return true;
	}

	bool CShopManager::RecvMyAuctionCancelClientPacket(LPCHARACTER ch)
	{
		if(auto auction = ch->GetIkarusAuction())
		{
			if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_MY_AUCTION_CANCEL))
				return false;

			if(!auction->GetOffers().empty())
				return false;

			TPacketGDNewIkarusShop pack;
			pack.bSubHeader = ikashop::SUBHEADER_GD_AUCTION_CANCEL;
			
			DWORD ownerid = ch->GetPlayerID();

			TEMP_BUFFER buff;
			buff.write(&pack, sizeof(pack));
			buff.write(&ownerid, sizeof(ownerid));

			db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
		}
		return true;
	}

	bool CShopManager::RecvAuctionCreateClientPacket(LPCHARACTER ch, const TPriceInfo& startprice, const TItemPos& pos)
	{
		if(!ch || ch->GetIkarusAuction())
			return false;

		if(!CheckGMLevel(ch))
			return false;

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_AUCTION_CREATE))
			return false;

		if (!CheckCharacterActions(ch))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot perform this operation right now."));
			return true;
		}

		if (ch->IsBusy(BUSY_SHOP_MANAGE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
			return false;
		}

		if (!IsGoodSalePrice(startprice))
			return false;


		//checking about duration limit
		auto duration = OFFLINESHOP_AUCTION_DURATION;
		auto item = ch->GetItem(pos);
		if(!item)
			return false;

		if(item->IsEquipped() || item->IsExchanging() || item->isLocked())
			return false;

		auto table= ITEM_MANAGER::instance().GetTable(item->GetVnum());
		if(!table)
			return false;

		if(IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_GIVE) || IS_SET(table->dwAntiFlags , ITEM_ANTIFLAG_MYSHOP))
			return false;

#ifdef ENABLE_SOULBIND_SYSTEM
		if (item->IsSealed())
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_CREATE_AUCTION_ITEM_SEALED");
			return false;
		}
#endif

		// patch with warp check
		ch->SetIkarusShopUseTime();

		//making info
		TAuctionInfo auction;
		auction.duration = duration;
		auction.owner = ch->GetPlayerID();
		auction.id = item->GetID();
		auction.price = startprice;
		auction.count = item->GetCount();
		auction.vnum = item->GetVnum();
		auction.expiration = GetItemExpiration(item);
#ifdef ENABLE_CHANGELOOK_SYSTEM
		auction.dwTransmutation = item->GetTransmutation();
#endif
		str_to_cstring(auction.ownername, ch->GetName());
		std::memcpy(auction.aAttr, item->GetAttributes(), sizeof(auction.aAttr));
		std::memcpy(auction.alSockets, item->GetSockets(), sizeof(auction.alSockets));

		//removing from character and deleting item instance
		MoveAndDestroyInstance(item, IKASHOP_AUCTION, 0);

		SendAuctionCreateDBPacket(auction);
		return true;
	}

	bool CShopManager::RecvAuctionAddOfferClientPacket(LPCHARACTER ch, DWORD ownerid, const TPriceInfo& price)
	{
		//checking about guesting
		if(!ch || !ch->GetIkarusAuctionGuest() || ch->GetIkarusAuctionGuest()->GetInfo().owner != ownerid)
			return false;

#ifdef ENABLE_MT2009_IKASHOP_RESTRICTIONS
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("FEATURE_UNAVAILABLE"));
		return false;
#endif

		if (!ch->IkarusShopFloodCheck(SHOP_ACTION_WEIGHT_AUCTION_ADD_OFFER))
			return false;

		if(!CheckGMLevel(ch))
			return false;

		if (!CheckSafeboxSize(ch))
			return false;

		//check anti auto-offer
		if(ch->GetPlayerID() == ownerid)
			return false;

		//check about enough money
		if(static_cast<long long>(ch->GetGold()) < price.yang)
			return false;

#ifdef ENABLE_CHEQUE_SYSTEM
		if (ch->GetCheque() < price.cheque)
			return false;
#endif

		//checking about raise from best buyer
		auto auction = ch->GetIkarusAuctionGuest();
		if(auction->GetBestBuyer() == ch->GetPlayerID()){
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_RAISE_OVER_YOURSELF");
			return false;
		}

		// patch with warp check
		ch->SetIkarusShopUseTime();

		//checking about min raise amount (+10% by default)
		auto minRaise = GetAuctionMinRaise(auction);
		if (price.GetTotalYangAmount() < minRaise)
			return false;

		ch->ChangeGold(-price.yang);
#ifdef ENABLE_CHEQUE_SYSTEM
		ch->PointChange(POINT_CHEQUE, -price.cheque);
#endif

		TAuctionOfferInfo offer;
		offer.buyerid = ch->GetPlayerID();
		offer.ownerid = ownerid;
		offer.datetime = static_cast<int64_t>(std::time(nullptr));
		offer.price = price;
		offer.seenflag = false;
#ifdef ENABLE_CHEQUE_SYSTEM
		// converting amount cheque in yang
		offer.price.yang = offer.price.GetTotalYangAmount();
		offer.price.cheque=0;
#endif
		str_to_cstring(offer.buyername, ch->GetName());

		SendAuctionAddOfferDBPacket(offer);
		return true;
	}

	bool CShopManager::RecvAuctionExitFromAuction(LPCHARACTER ch, DWORD ownerid)
	{
		if(ch->GetIkarusAuctionGuest() && ch->GetIkarusAuctionGuest()->GetInfo().owner == ownerid)
			ch->SetIkarusAuctionGuest(nullptr);
		ch->SetIkarusShopUseTime();
		return true;
	}

	void CShopManager::SendAuctionListClientPacket(LPCHARACTER ch, const std::vector<TAuctionListElement>& auctionVec)
	{
		if (!ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader	= SUBHEADER_GC_AUCTION_LIST;
		pack.size = sizeof(pack) + sizeof(TSubPacketGCAuctionList) + (sizeof(TAuctionListElement)*auctionVec.size());

		TSubPacketGCAuctionList subpack{};
		subpack.count = auctionVec.size();

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));
		if(!auctionVec.empty())
			buff.write(&auctionVec[0], sizeof(auctionVec[0]) * auctionVec.size());

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}

	void CShopManager::SendAuctionOpenAuctionClientPacket(LPCHARACTER ch, const CAuction& auction)
	{
		if (!ch->GetDesc())
			return;

		const auto& info = auction.GetInfo();
		const auto& offers = auction.GetOffers();

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader = ch->GetPlayerID() != info.owner ? SUBHEADER_GC_OPEN_AUCTION : SUBHEADER_GC_OPEN_MY_AUCTION;
		pack.size = sizeof(pack)+ sizeof(TSubPacketGCAuctionOpen) + (sizeof(TAuctionOfferInfo) * offers.size());

		TSubPacketGCAuctionOpen subpack{};
		subpack.auction = info;
		subpack.offercount = offers.size();

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		for(auto& offer : offers)
			buff.write(offer.get(), sizeof(TAuctionOfferInfo));

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());

		// checking for new auction offers forwarded to owner
		if(pack.subheader == SUBHEADER_GC_OPEN_MY_AUCTION)
		{
			for(auto& offer : auction.GetOffers())
			{
				if(offer->seenflag == false)
				{
					TPacketGDNewIkarusShop pack{};
					pack.bSubHeader = ikashop::SUBHEADER_GD_AUCTION_OFFER_SEEN;

					ikashop::TSubPacketGDAuctionOfferSeen subpack{};
					subpack.ownerid = auction.GetInfo().owner;
					subpack.buyerid = offer->buyerid;
					subpack.price = offer->price.yang;

					TEMP_BUFFER buffer;
					buffer.write(&pack, sizeof(pack));
					buffer.write(&subpack, sizeof(subpack));

					db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buffer.read_peek(), buffer.size());
				}
			}	
		}
	}

	void CShopManager::SendAuctionOpenMyAuctionNoAuctionClientPacket(LPCHARACTER ch)
	{
		if (!ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.subheader	= SUBHEADER_GC_OPEN_MY_AUCTION_NO_AUCTION;
		pack.size = sizeof(pack);

		ch->GetDesc()->Packet(&pack, sizeof(pack));
	}

	void CShopManager::RecvCloseMyShopBoardClientPacket(LPCHARACTER ch)
	{
		if (ch)
		{
			ch->SetIkarusShopUseTime(); // check warp
			ch->SetLookingShopOwner(false);
			
			if (auto shop = ch->GetIkarusShop())
			{
				if (shop->IsEditMode())
					shop->SetEditMode(false);
			}
		}
	}

	void CShopManager::RecvCloseOfferListClientPacket(LPCHARACTER ch)
	{
		if (ch)
		{
			ch->SetIkarusShopUseTime(); // check warp
			ch->SetLookingIkarusShopOfferList(false);
		}
	}

	void CShopManager::RecvCloseShopGuestClientPacket(LPCHARACTER ch)
	{
		if (ch)
		{
			ch->SetIkarusShopUseTime(); // check warp
			ch->SetIkarusShopGuest(nullptr);
		}
	}

#ifdef EXTEND_IKASHOP_PRO
	void CShopManager::SendNotificationListClientPacket(LPCHARACTER ch)
	{
		if(ch && ch->GetDesc())
		{
			auto& notifications = m_notificationMap[ch->GetPlayerID()];

			TPacketGCNewIkarusShop pack{};
			pack.header = HEADER_GC_NEW_OFFLINESHOP;
			pack.size = sizeof(pack) + sizeof(ikashop::TSubPacketGCNotificationList) + notifications.size() * sizeof(TNotificationInfo);
			pack.subheader = ikashop::SUBHEADER_GC_NOTIFICATION_LIST;

			ikashop::TSubPacketGCNotificationList subpack{};
			subpack.count = notifications.size();

			TEMP_BUFFER buffer;
			buffer.write(&pack, sizeof(pack));
			buffer.write(&subpack, sizeof(subpack));
			if (!notifications.empty())
				buffer.write(notifications.data(), notifications.size() * sizeof(TNotificationInfo));

			ch->GetDesc()->Packet(buffer.read_peek(), buffer.size());


			// removing notifications
			for(auto& notification : notifications)
			{
				TPacketGDNewIkarusShop pack{};
				pack.bSubHeader = ikashop::SUBHEADER_GD_NOTIFICATION_SEEN;

				ikashop::TSubPacketGDNotificationSeen subpack{};
				subpack.id = notification.id;
				subpack.owner = ch->GetPlayerID();

				TEMP_BUFFER buffer;
				buffer.write(&pack, sizeof(pack));
				buffer.write(&subpack, sizeof(subpack));

				db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buffer.read_peek(), buffer.size());
			}

			notifications.clear();
			m_notificationMap[ch->GetPlayerID()].clear();

			// forwarding board counters
			if(ch->IsLookingShopOwner())
				SendBoardCountersClientPacket(ch);
		}
	}
#endif

	void CShopManager::SendBoardCountersClientPacket(LPCHARACTER ch)
	{
		// mt2009 remove counters
		/*if (!ch || !ch->GetDesc())
			return;

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.size = sizeof(pack) + sizeof(ikashop::TSubPacketGCBoardCounters);
		pack.subheader = ikashop::SUBHEADER_GC_BOARD_COUNTERS;

		ikashop::TSubPacketGCBoardCounters subpack{};

		// checking safebox items counter
		if(const auto safebox = ch->GetIkarusShopSafebox()){
			subpack.safebox = safebox->GetItems().size();
			if(safebox->GetValutes().yang != 0)
				subpack.safebox++;
		}

		// checking auction offers counter
		if(const auto auction = ch->GetIkarusAuction())
			subpack.auction = auction->GetNewOffersCounter();

		// checking incoming offers counter
		if(const auto shop = ch->GetIkarusShop())
			subpack.offers = shop->GetNewOffersCounter();

#ifdef EXTEND_IKASHOP_PRO
		const auto& notifications = m_notificationMap[ch->GetPlayerID()];
		subpack.notification = notifications.size();
#endif
		ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
		ch->GetDesc()->Packet(&subpack, sizeof(subpack));*/
	}

	void ikashop::CShopManager::NotifyOwnerItemSold(LPCHARACTER ch, DWORD item_vnum, ITEM_COUNT item_count, YANG price)
	{
		if (!ch || !ch->GetDesc())
			return;

		TSubPacketGCShopNotifyItemSold subpack{};

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.size = sizeof(pack) + sizeof(subpack);
		pack.subheader = ikashop::SUBHEADER_GC_NOTIFY_ITEM_SOLD;

		subpack.itemVnum = item_vnum;
		subpack.count = item_count;
		subpack.price = price;

		ch->GetDesc()->BufferedPacket(pack);
		ch->GetDesc()->Packet(subpack);
	}


	void CShopManager::SendPopupMessage(LPCHARACTER ch, const std::string& message)
	{
		if (!ch || !ch->GetDesc()) return;
		ikashop::TSubPacketGCPopupMessage subpack{};
		str_to_cstring(subpack.localeString, message.data());

		TPacketGCNewIkarusShop pack{};
		pack.header = HEADER_GC_NEW_OFFLINESHOP;
		pack.size = sizeof(pack) + sizeof(subpack);
		pack.subheader = ikashop::SUBHEADER_GC_POPUP_MESSAGE;

		ch->GetDesc()->BufferedPacket(pack);
		ch->GetDesc()->Packet(subpack);
	}

	void CShopManager::UpdateShopsDuration()
	{
		for(auto& [id, shop] : m_mapShops)
			if(shop->GetDuration() > 0)
				shop->DecreaseDuration();
	}

	void CShopManager::UpdateAuctionsDuration()
	{
		for(auto& [id, auction] : m_mapAuctions)
			auction->DecreaseDuration();
	}

	void CShopManager::ClearSearchTimeMap()
	{
		m_searchCooldownMap.clear();
		m_offerCooldownMap.clear();
	}

	bool CShopManager::CheckOfferCooldown(DWORD pid) {
		static const auto cooldown_seconds = OFFLINESHOP_SECONDS_PER_OFFER * 1000;
		const auto now = get_dword_time();

		auto it = m_offerCooldownMap.find(pid);
		if (it == m_offerCooldownMap.end()) {
			m_offerCooldownMap[pid] = now + cooldown_seconds ;
			return true;
		}

		auto& cooldown = it->second;
		if (cooldown > now)
			return false;

		cooldown = now + cooldown_seconds;
		return true;
	}

	bool CShopManager::CheckSafeboxSize(LPCHARACTER ch)
	{
		// validating character pointer
		if(!ch)
			return false;

		// searching for safebox
		auto safebox = ch->GetIkarusShopSafebox() ? ch->GetIkarusShopSafebox() : GetShopSafeboxByOwnerID(ch->GetPlayerID());
		if(!safebox)
			return false;

		// checking safebox size
		if(safebox->GetLeftSpace() < 1)
		{
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_SAFEBOX_FULL");
			return false;
		}

		return true;
	}

	bool CShopManager::CheckSearchCooldown(DWORD pid)
	{
		static const auto cooldown_seconds = OFFLINESHOP_SECONDS_PER_SEARCH * 1000;
		const auto now = get_dword_time();

		auto it = m_searchCooldownMap.find(pid);
		if (it == m_searchCooldownMap.end()) {
			m_searchCooldownMap[pid] = now + cooldown_seconds;
			return true;
		}

		auto& cooldown = it->second;
		if (cooldown > now)
			return false;

		cooldown = now + cooldown_seconds;
		return true;
	}
}

#endif
// Files shared by GameCore.top
