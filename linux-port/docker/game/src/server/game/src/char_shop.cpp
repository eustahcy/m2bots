#include "stdafx.h"
#include <sstream>

#include "utils.h"
#include "char.h"
#include "char_manager.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "packet.h"
#include "item.h"
#include "item_manager.h"
#include "p2p.h"
#include "party.h"
#include "buffer_manager.h"
#include "log.h"
#include "shop.h"
#include "shop_manager.h"
#include "questmanager.h"
#include "banword.h"
#include "exchange.h"
#include "db.h"
#include "config.h"
#include "ikarus_shop.h"
#include "ikarus_shop_manager.h"

#include <boost/algorithm/string.hpp>
#include "../../libgame/include/grid.h"

std::pair<int, int> GetSlotXY(int slotIndex) {
	if (slotIndex < 0 || slotIndex >= SHOP_PLAYER_WIDTH * SHOP_PLAYER_HEIGHT) {
		return std::make_pair(0, 0);
	}

	int x = slotIndex % SHOP_PLAYER_WIDTH;
	int y = slotIndex / SHOP_PLAYER_WIDTH;
	return std::make_pair(x, y);
}

bool IsInLockedArea(int slotIndex, int unlockValue = 0) {
	if (unlockValue > SHOP_PLAYER_LOCKED_Y_END - SHOP_PLAYER_LOCKED_Y_START)
		return false;

	auto xy = GetSlotXY(slotIndex);
	int x = xy.first;
	int y = xy.second;
	return (
		(SHOP_PLAYER_LOCKED_Y_START + unlockValue) <= y && y <= SHOP_PLAYER_LOCKED_Y_END &&
		SHOP_PLAYER_LOCKED_X_START <= x && x <= SHOP_PLAYER_LOCKED_X_END
		);
}

bool IsInPremiumArea(int slotIndex) {
	auto xy = GetSlotXY(slotIndex);
	int x = xy.first;
	int y = xy.second;
	return (
		SHOP_PLAYER_PREMIUM_Y_START <= y && y <= SHOP_PLAYER_PREMIUM_Y_END &&
		SHOP_PLAYER_PREMIUM_X_START <= x && x <= SHOP_PLAYER_PREMIUM_X_END
		);
}

bool CheckItem(LPITEM pkItem, LPCHARACTER ch, BYTE pos)
{
	if (!ch)
		return false;

	if (pkItem)
	{
		const TItemTable* item_table = pkItem->GetProto();

		if (item_table && (IS_SET(item_table->dwAntiFlags, ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_MYSHOP)))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't sell Item-Shop items in a private shop."));
			return false;
		}

		if (pkItem->IsEquipped() == true)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot sell items you have equipped."));
			return false;
		}

		if (true == pkItem->isLocked())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Items you are using cannot be sold in your personal shop."));
			return false;
		}

		if (pkItem->GetOwner() != ch)
		{
			return false;
		}

#ifdef ENABLE_SOULBIND_SYSTEM
		if (pkItem->IsSealed()) {
			SendPopupMessage(ch, "IKASHOP_SERVER_POPUP_MESSAGE_CANNOT_ADD_SEALED_ITEM");
			return true;
		}
#endif

		if (!ch->CanPlaceItemOnShopSlot(pos, pkItem->GetSize()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_NO_PREMIUM"));
			return false;
		}
		return true;
	}
	return false;
}

bool CHARACTER::CanPlaceOnShopSlot(BYTE slot)
{
	if (IsInPremiumArea(slot) && GetPremiumRemainSeconds(PREMIUM_SHOP) < 1)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_NO_PREMIUM"));
		return false;
	}

	if (IsInLockedArea(slot, GetShopUnlockedProgress()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_LOCKED_SLOTS"));
		return false;
	}
	return true;
}

bool CHARACTER::CanPlaceItemOnShopSlot(BYTE slot, BYTE itemHeight)
{
	for (int i = 0; i < itemHeight; i++)
	{
		BYTE checkSlot = slot + i * SHOP_PLAYER_WIDTH;
		if (!CanPlaceOnShopSlot(checkSlot))
			return false;
	}
	return true;
}

bool CHARACTER::CanAddItemToShop(LPITEM item, BYTE slot)
{
	return CheckItem(item, this, slot);
}

void CHARACTER::OpenMyShop(const char* c_pszSign, TShopItemTable* pTable, BYTE bItemCount, BYTE bTimeIndex)
{
	if (!CanHandleItem() || IsBusy()) // @fixme149
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't use a private shop now."));
		return;
	}

	if (!CanOpenShop())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("YOU_DONT_MEET_SHOP_REQUIREMENTS"));
		return;
	}

#ifndef ENABLE_OPEN_SHOP_WITH_ARMOR
	if (GetPart(PART_MAIN) > 2)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can only open the shop if you take off your armour."));
		return;
	}
#endif

	if (GetMyShop())
	{
		CloseMyShop();
		return;
	}

	if (GetIkarusShop())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_OPEN_NEW_SHOP"));
		return;
	}

	const bool isOfflineShop = bTimeIndex > 0;
	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(GetPlayerID());
	if (pPC->IsRunning())
		return;

	if (bItemCount == 0)
		return;

	YANG nTotalMoney = 0;
#ifdef ENABLE_CHEQUE_SYSTEM
	YANG nTotalCheque = 0;
#endif

	for (int n = 0; n < bItemCount; ++n)
	{
		nTotalMoney += static_cast<YANG>((pTable + n)->price);
#ifdef ENABLE_CHEQUE_SYSTEM
		nTotalCheque += static_cast<YANG>((pTable + n)->cheque);
#endif
	}

	nTotalMoney += GetGold();
#ifdef ENABLE_CHEQUE_SYSTEM
	nTotalCheque += static_cast<YANG>(GetCheque());
#endif

	if (GOLD_MAX <= nTotalMoney)
	{
		sys_err("[OVERFLOW_GOLD] Overflow (GOLD_MAX) id %u name %s", GetPlayerID(), GetName());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade because you are carrying more than 2 billion Yang."));
		return;
	}

#ifdef ENABLE_CHEQUE_SYSTEM
	if (CHEQUE_MAX <= nTotalCheque)
	{
		sys_err("[OVERFLOW_CHEQUE] Overflow (CHEQUE_MAX) id %u name %s", GetPlayerID(), GetName());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't create a shop with more than %d won."), CHEQUE_MAX - 1);
		return;
	}
#endif

	if (IsHorseRiding())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You're already riding. Get off first."));
		return;
	}

	std::string stShopSign;
	if (!ikashop::CShopManager::instance().ParseShopName(c_pszSign, stShopSign))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't give your shop an invalid name."));
		return;
	}

	// MYSHOP_PRICE_LIST
	std::map<DWORD, ITEM_COUNT> itemkind;
	// END_OF_MYSHOP_PRICE_LIST

	std::unique_ptr<CGrid> _tempGrid = std::make_unique<CGrid>(SHOP_PLAYER_WIDTH, SHOP_PLAYER_HEIGHT);

	auto offlineShop = GetIkarusShop();
	if (isOfflineShop && offlineShop && offlineShop->GetItems().size() > 0)
	{
		for (auto v : offlineShop->GetItems())
		{
			auto item = v.second;
			if (item && item->GetTable())
				_tempGrid->Put(item->GetInfo().pos, 1, item->GetTable()->bSize);
		}
	}

	std::set<TItemPos> cont;
	for (BYTE i = 0; i < bItemCount; ++i)
	{
		// check if pos is valid
		if (pTable->display_pos < 0 || pTable->display_pos >= SHOP_PLAYER_HOST_ITEM_MAX_NUM)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_ITEM_WRONG_POS"));
			return;
		}

		if (cont.find((pTable + i)->pos) != cont.end())
		{
			sys_err("MYSHOP: duplicate shop item detected! (name: %s)", GetName());
			return;
		}

		if (!isOfflineShop && (pTable + i)->pos.cell >= INVENTORY_DEFAULT_MAX_NUM)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot add items from horse inventory to normal shop."));
			return;
		}

		auto shopItem = (pTable + i);
		if (!shopItem)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_ITEM_ERROR"));
			return;
		}

		BYTE shop_pos = shopItem->display_pos;

		// ANTI_GIVE, ANTI_MYSHOP check
		LPITEM pkItem = GetItem((pTable + i)->pos);

		if (!pkItem)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_ITEM_ERROR"));
			return;
		}

		if (pkItem)
		{
			if (!CanAddItemToShop(pkItem, shop_pos))
			{
				return;
			}

			if (shopItem->price < 1 || shopItem->price > GOLD_MAX)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_ITEM_WRONG_PRICE%s"), pkItem->GetName());
				return;
			}

			const TItemTable* item_table = pkItem->GetProto();
			if (!_tempGrid->IsEmpty(shop_pos, 1, item_table->bSize)) {
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_WRONG_CONFIGURATION"));
				return;
			}
			_tempGrid->Put(shop_pos, 1, item_table->bSize);

			// MYSHOP_PRICE_LIST
			itemkind[pkItem->GetVnum()] = (pTable + i)->price / pkItem->GetCount();
			// END_OF_MYSHOP_PRICE_LIST
		}

		cont.emplace((pTable + i)->pos);
	}

	_tempGrid->Clear();

	// MYSHOP_PRICE_LIST
	// mt2009 edit: nie uzywamy tego czegos
	/*if (CountSpecifyItem(71049)) {
		// @fixme403 BEGIN
		TItemPriceListTable header;
		memset(&header, 0, sizeof(TItemPriceListTable));

		header.dwOwnerID = GetPlayerID();
		header.byCount = itemkind.size();

		size_t idx=0;
		for (itertype(itemkind) it = itemkind.begin(); it != itemkind.end(); ++it)
		{
			header.aPriceInfo[idx].dwVnum = it->first;
			header.aPriceInfo[idx].dwPrice = it->second;
			idx++;
		}

		db_clientdesc->DBPacket(HEADER_GD_MYSHOP_PRICELIST_UPDATE, GetDesc()->GetHandle(), &header, sizeof(TItemPriceListTable));
		// @fixme403 END
	}*/
	// END_OF_MYSHOP_PRICE_LIST
	if (!CountSpecifyItem(50200))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_CREATE_NO_ITEM"));
		return;
	}

	if (m_pkExchange)
		m_pkExchange->Cancel();

	if (isOfflineShop)
	{
		ChatDebug("OpenOfflineShop time %u", bTimeIndex);
		if (ikashop::CShopManager::instance().OpenOfflineShop(this, stShopSign.c_str(), pTable, bItemCount, bTimeIndex))
			RemoveSpecifyItem(50200, 1);
	}
	else
	{
		RemoveSpecifyItem(50200, 1);

		TPacketGCShopSign p;

		p.bHeader = HEADER_GC_SHOP_SIGN;
		p.dwVID = GetVID();
		strlcpy(p.szSign, c_pszSign, sizeof(p.szSign));

		PacketAround(p);

		if (GetHorse())
		{
			HorseSummon(false, true);
		}
		else if (GetMountVnum())
		{
			RemoveAffect(AFFECT_MOUNT);
			RemoveAffect(AFFECT_MOUNT_BONUS);
		}

		m_stShopSign = stShopSign;
		m_pkMyShop = CShopManager::instance().CreatePCShop(this, pTable, bItemCount);

		if (IsPolymorphed() == true)
		{
			RemoveAffect(AFFECT_POLYMORPH);
		}

		SetPolymorph(30000, true);
	}
}

void CHARACTER::CloseMyShop()
{
	if (GetMyShop())
	{
		m_stShopSign.clear();
		CShopManager::instance().DestroyPCShop(this);
		m_pkMyShop = NULL;

		TPacketGCShopSign p;

		p.bHeader = HEADER_GC_SHOP_SIGN;
		p.dwVID = GetVID();
		p.szSign[0] = '\0';

		PacketAround(p);
#ifdef ENABLE_WOLFMAN_CHARACTER
		SetPolymorph(m_points.job, true);
		// SetPolymorph(0, true);
#else
		SetPolymorph(GetJob(), true);
#endif
	}
}

BYTE CHARACTER::GetShopUnlockedProgress()
{
	return GetSpecialFlag(SHOP_SLOT_UNLOCK_PROGRESS_FLAG);
}

bool CHARACTER::CanOpenShop()
{
	// playerbot: a GM opens a stall without the kill count.
	if (GetGMLevel() > GM_PLAYER)
		return true;
	// A playerbot trades from the start: the level and the eight hundred kills
	// are a rule for people, and a world of two thousand bots opened no
	// counter for hours after every restart while each one earned them again.
	if (GetDesc() && GetDesc()->IsBot())
		return true;
	// playerbot: no kill count for a player either - the eight hundred
	// kills were the public server's gate (gregoszky, 14 September).
	return GetLevel() >= 15;
}

// Files shared by GameCore.top
