#include "stdafx.h"
#include "config.h"
#include "utils.h"
#include "constants.h"
#include "locale_service.h"
#include "text_file_loader.h"
#include "itemshop_manager.h"
#include "item_manager.h"
#include "questmanager.h"
#include "char.h"
#include "item.h"
#include "log.h"
#include "desc.h"
#include "db.h"
#include "packet.h"
#include "desc_client.h"
#include "buffer_manager.h"
#include "p2p.h"
#include "char_manager.h"
#include "../../common/PulseManager.h"

CItemShopManager::CItemShopManager()
{
}

CItemShopManager::~CItemShopManager()
{
	Destroy();
}

bool CItemShopManager::Initialize(TItemShopItem* pTable, int iSize)
{
	m_map_ItemShopItems.clear();
	m_map_ItemShopTimeAuctions.clear();
	m_map_AuctionViewer.clear();

	TItemShopItem* t = pTable;

	for (int i = 0; i < iSize; ++i, ++t)
	{
		sys_log(0, "mamy item %d o vnumie %d promotionPrice %d", t->dwIndex, t->dwVnum, t->promotion.dwPrice);
		TItemShopItem item;
		thecore_memcpy(&item, t, sizeof(TItemShopItem));

		if (IsItemTimeAuction(item))
		{
			m_map_ItemShopTimeAuctions.insert(std::make_pair(item.dwIndex, item));
		}
		else
		{
			m_map_ItemShopItems.insert(std::make_pair(item.dwIndex, item));
		}
	}

	// Refresh auction buy amount
	std::unique_ptr<SQLMsg> pkMsg(DBManager::instance().DirectQuery("SELECT item_index, buy_count FROM itemshop_time_auction"));
	if (pkMsg->Get()->uiNumRows > 0)
	{
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pkMsg->Get()->pSQLResult)) != NULL)
		{
			int col = 0;
			DWORD item_index;
			DWORD buy_count;
			str_to_number(item_index, row[col++]);
			str_to_number(buy_count, row[col++]);
			UpdateAuctionBuyAmount(item_index, buy_count);
		}
	}

	return true;
}

void CItemShopManager::Destroy()
{
	m_map_ItemShopItems.clear();
	m_map_ItemShopTimeAuctions.clear();
}

bool CItemShopManager::IsItemExist(int index)
{
	auto it = m_map_ItemShopItems.find(index);

	if (it != m_map_ItemShopItems.end()) {
		return true;
	}
	else {
		return false;
	}
}

bool CItemShopManager::IsAuctionExist(int index)
{
	auto it = m_map_ItemShopTimeAuctions.find(index);

	if (it != m_map_ItemShopTimeAuctions.end()) {
		return true;
	}
	else {
		return false;
	}
}

TItemShopItem CItemShopManager::GetItem(int index)
{
	if (IsItemExist(index))
		return m_map_ItemShopItems[index];
	else if (IsAuctionExist(index)) {
		return m_map_ItemShopTimeAuctions[index];
	}
	return TItemShopItem{};
}

DWORD CItemShopManager::GetItemPrice(TItemShopItem item)
{
	if (IsProperItem(item))
	{
		if (IsItemPromotion(item))
			return item.promotion.dwPrice;

		return item.dwPrice;
	}
	return 0;
}

BYTE CItemShopManager::GetItemCurrency(TItemShopItem item)
{
	if (IsProperItem(item))
		return item.bCurrency;
	return 0;
}

bool CItemShopManager::IsItemPromotion(TItemShopItem item)
{
	if (IsProperItem(item))
	{
		DWORD start_time = item.promotion.dwStartTime;
		DWORD end_time = item.promotion.dwEndTime;
		DWORD current_time = get_global_time();
		return current_time >= start_time && current_time < end_time;
	}
	return false;
}

bool CItemShopManager::IsItemTimeAuction(TItemShopItem item)
{
	if (IsProperItem(item))
	{
		DWORD start_time = item.timeAuction.dwStartTime;
		DWORD end_time = item.timeAuction.dwEndTime;
		return start_time > 0 && end_time > 0;
	}
	return false;
}

bool CItemShopManager::IsTimeAuctionActive(TItemShopItem item)
{
	if (IsProperItem(item))
	{
		DWORD start_time = item.timeAuction.dwStartTime;
		DWORD end_time = item.timeAuction.dwEndTime;
		DWORD current_time = get_global_time();
		return current_time >= start_time && current_time < end_time;
	}
	return false;
}

bool CItemShopManager::CheckIfCanBuyTimeAuctionAndUpdate(TItemShopItem item, LPCHARACTER buyer, DWORD quantity)
{
	if (!buyer)
		return false;

	if (!buyer->GetDesc())
		return false;

	if (IsItemTimeAuction(item))
	{
		if (!IsTimeAuctionActive(item))
			return false;

		bool isInfinite = item.timeAuction.dwMaxBuyAmount == 0;
		if (!isInfinite) {
			if (item.timeAuction.dwCurrentBuyAmount >= item.timeAuction.dwMaxBuyAmount)
				return false;

			std::unique_ptr<SQLMsg> pkMsg(DBManager::instance().DirectQuery("SELECT buy_count FROM itemshop_time_auction WHERE item_index=%d", item.dwIndex));
			if (pkMsg->Get()->uiNumRows < 1)
				return false;

			DWORD buy_count = 0;
			MYSQL_ROW row = mysql_fetch_row(pkMsg->Get()->pSQLResult);
			str_to_number(buy_count, row[0]);

			if (buy_count >= item.timeAuction.dwMaxBuyAmount)
				return false;

			DWORD item_count = item.dwCount * quantity;

			// account limit start
			const DWORD MAX_BUY_PER_ACCOUNT = item.timeAuction.dwAccountLimit;
			if (MAX_BUY_PER_ACCOUNT > 0)
			{
				std::string auction_buy_flag = "itemshop_auction_buy_" + std::to_string(item.dwIndex);
				std::string auction_buy_time_flag = "itemshop_auction_time_" + std::to_string(item.dwIndex);
				int buyer_buy_count = buyer->GetAccountFlag(auction_buy_flag);
				if (get_global_time() > buyer->GetAccountFlag(auction_buy_time_flag))
				{
					buyer_buy_count = 0;
				}

				if (buyer_buy_count >= MAX_BUY_PER_ACCOUNT)
				{
					buyer->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ITEMSHOP_TIME_AUCTION_ACCOUNT_LIMIT"), item.timeAuction.dwAccountLimit);
					return false;
				}

				if (buyer_buy_count + item_count > MAX_BUY_PER_ACCOUNT)
				{
					buyer->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ITEMSHOP_TIME_AUCTION_PURCHASE_WOULD_EXCEED_LIMIT"));
					return false;
				}

				buyer->SetAccountFlag(auction_buy_flag, buyer_buy_count + item_count);
				buyer->SetAccountFlag(auction_buy_time_flag, item.timeAuction.dwEndTime);
			}
			// account limit end

			DWORD current_progress = GetTimeAuctionProgress(item);
			item.timeAuction.dwCurrentBuyAmount = buy_count + item_count;
			DWORD new_progress = GetTimeAuctionProgress(item);
			buyer->ChatDebug("%d != %d", current_progress, new_progress);
			if (current_progress != new_progress)
			{
				buyer->ChatDebug("wysylam!");
				TPacketGGItemShopAuctionUpdate p;
				p.bHeader = HEADER_GG_ITEMSHOP_AUCTION_UPDATE;
				p.dwItemIndex = item.dwIndex;
				p.dwBuyAmount = item.timeAuction.dwCurrentBuyAmount;

				P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGItemShopAuctionUpdate));
				UpdateAuctionBuyAmount(item.dwIndex, item.timeAuction.dwCurrentBuyAmount);
			}
			DBManager::instance().DirectQuery("UPDATE itemshop_time_auction SET buy_count=buy_count+%d WHERE item_index=%d", item_count, item.dwIndex);
		}

		return true;
	}

	return false;
}

void CItemShopManager::UpdateAuctionBuyAmount(DWORD dwItemIndex, DWORD dwBuyAmount)
{
	if (IsAuctionExist(dwItemIndex)) {
		auto item = m_map_ItemShopTimeAuctions[dwItemIndex];
		item.timeAuction.dwCurrentBuyAmount = dwBuyAmount;
		m_map_ItemShopTimeAuctions[dwItemIndex] = item;
		RefreshAuctionViewers(item.dwIndex);
	}
}

bool CItemShopManager::CanSendItem(LPCHARACTER buyer, TItemShopItem item)
{
	TItemTable* pkItemTable = ITEM_MANAGER::instance().GetTable(item.dwVnum);
	switch (buyer->GetRaceNum())
	{
	case MAIN_RACE_WARRIOR_M:
	case MAIN_RACE_ASSASSIN_M:
	case MAIN_RACE_SURA_M:
	case MAIN_RACE_SHAMAN_M:
	{
		if (IS_SET(pkItemTable->dwAntiFlags, ITEM_ANTIFLAG_MALE))
			return false;
	}
	break;
	case MAIN_RACE_WARRIOR_W:
	case MAIN_RACE_ASSASSIN_W:
	case MAIN_RACE_SURA_W:
	case MAIN_RACE_SHAMAN_W:
	{
		if (IS_SET(pkItemTable->dwAntiFlags, ITEM_ANTIFLAG_FEMALE))
			return false;
	}
	break;
	}

	if (buyer->GetJob() == JOB_WARRIOR && IS_SET(pkItemTable->dwAntiFlags, ITEM_ANTIFLAG_WARRIOR))
		return false;
	else if (buyer->GetJob() == JOB_ASSASSIN && IS_SET(pkItemTable->dwAntiFlags, ITEM_ANTIFLAG_ASSASSIN))
		return false;
	else if (buyer->GetJob() == JOB_SURA && IS_SET(pkItemTable->dwAntiFlags, ITEM_ANTIFLAG_SURA))
		return false;
	else if (buyer->GetJob() == JOB_SHAMAN && IS_SET(pkItemTable->dwAntiFlags, ITEM_ANTIFLAG_SHAMAN))
		return false;

	return true;
}

bool CItemShopManager::CanSendTimeAuction(TItemShopItem item)
{
	DWORD start_time = item.timeAuction.dwStartTime;
	DWORD end_time = item.timeAuction.dwEndTime;
	DWORD current_time = get_global_time();

	if (current_time < start_time)
	{
		DWORD deltaTime = start_time - current_time;
		return deltaTime < 86400; // 1 day
	}

	if (current_time > end_time) {
		DWORD deltaTime = current_time - end_time;
		return deltaTime < 3600; // 1 hour
	}

	return true;
}

DWORD CItemShopManager::GetTimeAuctionProgress(TItemShopItem item)
{
	if (item.timeAuction.dwMaxBuyAmount < 1)
		return 999; // infinite

	int amount_left = MAX(item.timeAuction.dwMaxBuyAmount - item.timeAuction.dwCurrentBuyAmount, 0);
	return amount_left * 100 / item.timeAuction.dwMaxBuyAmount;
}

void CItemShopManager::SendBalance(LPCHARACTER ch, PlayerAccountBalance balance)
{
	if (!ch)
		return;

	LPDESC desc = ch->GetDesc();
	if (!desc)
		return;

	TPacketGCItemShop pack;
	pack.header = HEADER_GC_ITEMSHOP;
	pack.subheader = SUBHEADER_ITEMSHOP_BALANCE;

	TPacketGCItemShopBalance pack2;
	memset(&pack2, 0, sizeof(pack2));

	ch->ChatDebug("current balance dc: %d, dm: %d", balance.dwDragonCoins, balance.dwDragonMarks);
	pack2.dwDragonCoins = balance.dwDragonCoins;
	pack2.dwDragonMarks = balance.dwDragonMarks;

	pack.size = sizeof(pack) + sizeof(pack2);
	ch->GetDesc()->BufferedPacket(&pack, sizeof(TPacketGCItemShop));
	ch->GetDesc()->Packet(&pack2, sizeof(TPacketGCItemShopBalance));
}

void CItemShopManager::SendBalance(LPCHARACTER ch)
{
	auto balance = GetPlayerAccountBalance(ch);
	return SendBalance(ch, balance);
}

void CItemShopManager::SendHappyHour(LPCHARACTER ch)
{
	int value = quest::CQuestManager::instance().GetEventFlag("happy_hour_value");
	int dwEndTime = quest::CQuestManager::instance().GetEventFlag("happy_hour_end_time");

	if (value < 1)
		return;

	if (get_global_time() > dwEndTime)
		return;

	if (!ch)
		return;

	LPDESC desc = ch->GetDesc();
	if (!desc)
		return;

	TPacketGCItemShop pack;
	pack.header = HEADER_GC_ITEMSHOP;
	pack.subheader = SUBHEADER_ITEMSHOP_HAPPY_HOUR;

	TPacketGCItemShopHappyHour pack2;
	memset(&pack2, 0, sizeof(pack2));

	pack2.dwHappyHourValue = value;
	pack2.dwEndTime = dwEndTime;

	pack.size = sizeof(pack) + sizeof(pack2);
	ch->GetDesc()->BufferedPacket(&pack, sizeof(TPacketGCItemShop));
	ch->GetDesc()->Packet(&pack2, sizeof(TPacketGCItemShopHappyHour));
}

void CItemShopManager::SendLatestTimeAuction(LPCHARACTER ch)
{
	if (!ch || !ch->GetDesc())
		return;

	DWORD latestEndTime = 0;
	for (auto it : m_map_ItemShopTimeAuctions)
	{
		auto item = it.second;
		if (CanSendTimeAuction(item))
		{
			if (item.timeAuction.dwEndTime > latestEndTime)
				latestEndTime = item.timeAuction.dwEndTime;
		}
	}

	if (latestEndTime > 0)
	{
		TPacketGCItemShop pack;
		pack.header = HEADER_GC_ITEMSHOP;
		pack.subheader = SUBHEADER_ITEMSHOP_TIME_AUCTION;

		TPacketGCItemShopLatestTimeAuction pack2;
		memset(&pack2, 0, sizeof(pack2));
		pack2.dwEndTime = latestEndTime;

		pack.size = sizeof(pack) + sizeof(pack2);
		ch->GetDesc()->BufferedPacket(&pack, sizeof(TPacketGCItemShop));
		ch->GetDesc()->Packet(&pack2, sizeof(TPacketGCItemShopLatestTimeAuction));
	}
}

void CItemShopManager::SendItemShopData(LPCHARACTER ch)
{
	if (!ch)
		return;

	LPDESC desc = ch->GetDesc();
	if (!desc)
		return;

	int itemCount = 0;
	for (const auto& pair : m_map_ItemShopItems)
		if (CanSendItem(ch, pair.second))
			itemCount++;

	TPacketGCItemShop pack{};
	pack.header = HEADER_GC_ITEMSHOP;
	pack.subheader = SUBHEADER_ITEMSHOP_REFRESH;
	pack.size = sizeof(pack) + sizeof(TPacketGCItemShopRefresh) + sizeof(TItemShopItem) * itemCount;

	TPacketGCItemShopRefresh subpack{};
	memset(&subpack, 0, sizeof(subpack));
	subpack.count = itemCount;

	TEMP_BUFFER buff;
	buff.write(&pack, sizeof(pack));
	buff.write(&subpack, sizeof(subpack));

	for (const auto& pair : m_map_ItemShopItems) {
		DWORD key = pair.first;
		auto& item = pair.second;
		if (CanSendItem(ch, item)) {
			//ch->ChatDebug("sending item vnum: %d count: %d", item.dwVnum, item.dwCount);
			buff.write(&item, sizeof(TItemShopItem));
		}
	}

	desc->Packet(buff.read_peek(), buff.size());
	ch->ChatDebug("%d items sent", itemCount);
}

void CItemShopManager::SendAuctionData(LPCHARACTER ch)
{
	if (!ch)
		return;

	LPDESC desc = ch->GetDesc();
	if (!desc)
		return;

	int itemCount = 0;
	for (const auto& pair : m_map_ItemShopTimeAuctions)
		if (CanSendTimeAuction(pair.second))
			itemCount++;

	TPacketGCItemShop pack{};
	pack.header = HEADER_GC_ITEMSHOP;
	pack.subheader = SUBHEADER_ITEMSHOP_OPEN_AUCTIONS;
	pack.size = sizeof(pack) + sizeof(TPacketGCItemShopRefresh) + sizeof(TItemShopItem) * itemCount;

	TPacketGCItemShopRefresh subpack{};
	memset(&subpack, 0, sizeof(subpack));
	subpack.count = itemCount;

	TEMP_BUFFER buff;
	buff.write(&pack, sizeof(pack));
	buff.write(&subpack, sizeof(subpack));

	for (const auto& pair : m_map_ItemShopTimeAuctions) {
		DWORD key = pair.first;
		auto& item = pair.second;
		if (CanSendTimeAuction(item)) {
			TItemShopItem sendItem{};
			memcpy(&sendItem, &item, sizeof(TItemShopItem));
			sendItem.timeAuction.dwMaxBuyAmount = 0;
			sendItem.timeAuction.dwCurrentBuyAmount = GetTimeAuctionProgress(item);
			//ch->ChatDebug("sending auction item vnum: %d count: %d", sendItem.dwVnum, sendItem.dwCount);
			buff.write(&sendItem, sizeof(TItemShopItem));
		}
	}

	desc->Packet(buff.read_peek(), buff.size());
	ch->ChatDebug("%d time auctions sent", itemCount);
}

void CItemShopManager::SendAuctionUpdate(LPCHARACTER ch, DWORD index)
{
	if (!ch)
		return;

	LPDESC desc = ch->GetDesc();
	if (!desc)
		return;

	auto it = m_map_ItemShopTimeAuctions.find(index);
	if (it == m_map_ItemShopTimeAuctions.end())
		return;
	
	auto item = it->second;
	if (!CanSendTimeAuction(item))
		return;

	TPacketGCItemShop pack{};
	pack.header = HEADER_GC_ITEMSHOP;
	pack.subheader = SUBHEADER_ITEMSHOP_UPDATE_AUCTION;
	pack.size = sizeof(pack) + sizeof(TItemShopUpdateAuctionItem);

	TItemShopUpdateAuctionItem auctionPacket{};
	memset(&auctionPacket, 0, sizeof(auctionPacket));
	auctionPacket.dwIndex = item.dwIndex;
	auctionPacket.dwProgress = GetTimeAuctionProgress(item);
	auctionPacket.dwStartTime = item.timeAuction.dwStartTime;
	auctionPacket.dwEndTime = item.timeAuction.dwEndTime;
	auctionPacket.dwAccountLimit = item.timeAuction.dwAccountLimit;

	TEMP_BUFFER buff;
	buff.write(&pack, sizeof(pack));
	buff.write(&auctionPacket, sizeof(TItemShopUpdateAuctionItem));

	desc->Packet(buff.read_peek(), buff.size());
}

CItemShopManager::PlayerAccountBalance CItemShopManager::GetPlayerAccountBalance(LPCHARACTER ch)
{
	auto balance = PlayerAccountBalance();
	if (!ch)
		return balance;

	if (!ch->GetDesc())
		return balance;

	DWORD account_id = ch->GetDesc()->GetAccountTable().id;
	std::unique_ptr<SQLMsg> pkMsg(DBManager::instance().AccountQuery("SELECT cash, cash_mark FROM account WHERE id=%d", account_id));
	if (pkMsg->Get()->uiNumRows < 1)
		return balance;

	if (pkMsg->Get()->uiNumRows > 1)
	{
		sys_err("MORE THAN 1 account with id %d ???? FATAL ERROR!!!!! ", account_id);
		return balance;
	}

	MYSQL_ROW row = mysql_fetch_row(pkMsg->Get()->pSQLResult);
	str_to_number(balance.dwDragonCoins, row[0]);
	str_to_number(balance.dwDragonMarks, row[1]);

	return balance;
}

void CItemShopManager::RefreshItemShop(LPCHARACTER ch)
{
	if (!ch)
		return;

	ch->ChatDebug("refresh itemshop");
	SendBalance(ch);
	SendHappyHour(ch);
	SendLatestTimeAuction(ch);
	SendItemShopData(ch);

	if (ch->HasPlayerData())
		ch->playerData->m_isRefreshItemShopOnOpen = false;
}

void CItemShopManager::OpenItemShop(LPCHARACTER ch, bool refresh)
{
	if (!ch)
		return;

	if (ch->IsBusy())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_CONTINUE_WHEN_BUSY"));
		return;
	}

	if (ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_WHILE_DEAD"));
		return;
	}

	if (!ch->HasPlayerData())
		return;

	if (!refresh && ch->playerData->m_isRefreshItemShopOnOpen)
		refresh = true;

	ch->ChatDebug("open itemshop");
	ch->playerData->SetItemShopBrowse(true);
	if (refresh) {
		RefreshItemShop(ch);
	}
	ch->ChatPacket(CHAT_TYPE_COMMAND, "itemshop_open");
}

void CItemShopManager::CloseItemShop(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (!ch->HasPlayerData())
		return;

	ch->ChatDebug("close itemshop");
	ch->playerData->SetItemShopBrowse(false);
	CloseAuctionTab(ch);
}

void CItemShopManager::SendUseCodeVoucher(LPCHARACTER ch, const char* code)
{
	if (!ch)
		return;

	if (ch->HasPlayerData() && !ch->playerData->IsItemShopBrowse())
		return;

	if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::ItemShopBuy, std::chrono::milliseconds(2000)))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("NEXT_ACTION_WAIT_NO_FORMAT"));
		return;
	}

	char szCode[CODE_VOUCHER_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szCode, sizeof(szCode), code, std::strlen(code));

	ch->ChatDebug("checkCode %s", szCode);

	TRequestUseCodeVoucher packet;
	packet.dwPID = ch->GetPlayerID();
	packet.dwAID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(packet.code, szCode, sizeof(packet.code));

	db_clientdesc->DBPacketHeader(HEADER_GD_REQUEST_USE_VOUCHER, 0, sizeof(TRequestUseCodeVoucher));
	db_clientdesc->Packet(&packet, sizeof(packet));
}

void CItemShopManager::RecvUseCodeVoucher(LPCHARACTER ch, BYTE bResponse, const char* code)
{
	if (!ch)
		return;

	if (!ch->GetDesc())
		return;

	TPacketGCItemShop pack{};
	pack.header = HEADER_GC_ITEMSHOP;
	pack.subheader = SUBHEADER_ITEMSHOP_USE_VOUCHER;
	pack.size = sizeof(pack) + sizeof(TPacketGCItemShopUseCodeVoucher);

	TPacketGCItemShopUseCodeVoucher pack2{};
	pack2.bResponse = bResponse;
	ch->GetDesc()->BufferedPacket(&pack, sizeof(TPacketGCItemShop));
	ch->GetDesc()->Packet(&pack2, sizeof(TPacketGCItemShopUseCodeVoucher));

	if (bResponse == USE_VOUCHER_SUCCESS)
	{
		SendBalance(ch);

		LogManager::Instance().VoucherCodeLog(ch, code);
	}
}

void CItemShopManager::OpenAuctionTab(LPCHARACTER ch)
{
	if (!ch)
		return;

	m_map_AuctionViewer.emplace(ch->GetPlayerID());
	SendAuctionData(ch);
}

void CItemShopManager::CloseAuctionTab(LPCHARACTER ch)
{
	m_map_AuctionViewer.erase(ch->GetPlayerID());
}

void CItemShopManager::RefreshAuctionViewers(DWORD auctionIndex)
{
	for (auto& guest : m_map_AuctionViewer)
	{
		if (auto ch = CHARACTER_MANAGER::Instance().FindByPID(guest))
		{
			SendAuctionUpdate(ch, auctionIndex);
		}
	}
}

bool CItemShopManager::CheckBalance(LPCHARACTER ch, BYTE chargeType, long amount, PlayerAccountBalance& currentBalance, bool addMarks)
{
	if (chargeType == ERequestCharge_Cash)
	{
		if (amount < 0 && abs(amount) > currentBalance.dwDragonCoins)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ITEMSHOP_NO_DRAGON_COINS"));
			return false;
		}
		currentBalance.dwDragonCoins += amount;
		if (addMarks && amount < 0) {
			currentBalance.dwDragonMarks += abs(amount);
		}
	}
	else if (chargeType == ERequestCharge_Marks)
	{
		if (amount < 0 && abs(amount) > currentBalance.dwDragonMarks)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ITEMSHOP_NO_DRAGON_MARKS"));
			return false;
		}
		currentBalance.dwDragonMarks += amount;
	}
	return true;
}

bool CItemShopManager::AddCash(LPCHARACTER ch, BYTE chargeType, long amount, bool checkBalance, long customSecondAmount)
{
	if (NULL == ch || NULL == ch->GetDesc())
	{
		return false;
	}
	if (checkBalance) {
		PlayerAccountBalance balance = GetPlayerAccountBalance(ch);
		if (!CheckBalance(ch, chargeType, amount, balance))
			return false;

		SendBalance(ch, balance);
	}

	TRequestChargeCash packet;
	packet.dwAID = ch->GetDesc()->GetAccountTable().id;
	packet.lAmount = amount;
	if (customSecondAmount >= 0)
		packet.lSecondAmount = customSecondAmount;
	else
		packet.lSecondAmount = amount;
	packet.eChargeType = (ERequestChargeType)chargeType;

	db_clientdesc->DBPacketHeader(HEADER_GD_REQUEST_CHARGE_CASH, 0, sizeof(TRequestChargeCash));
	db_clientdesc->Packet(&packet, sizeof(packet));
	return true;
}

bool CItemShopManager::BuyItem(LPCHARACTER ch, int itemIndex, int quantity)
{
	if (!ch)
		return false;

	if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::ItemShopBuy, std::chrono::milliseconds(1000)))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("NEXT_ACTION_WAIT_NO_FORMAT"));
		return false;
	}

	if (!ch->CanHandleItem())
		return false;

	if (ch->HasPlayerData() && !ch->playerData->IsItemShopBrowse())
		return false;

	if (quantity < 1 || quantity > 10)
		return false;

	TItemShopItem item = GetItem(itemIndex);
	if (!IsProperItem(item))
		return false;

	if (ch->GetLevel() < item.bMinLevel)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ITEMSHOP_LOW_LEVEL_FOR_ITEM"), item.bMinLevel);
		return false;
	}

	TItemTable* itemTable = ITEM_MANAGER::instance().GetTable(item.dwVnum);
	if (!itemTable) {
		ch->ChatDebug("item vnum (%d) not found", item.dwVnum);
		return false;
	}

	DWORD PREMIUM_TOKEN_VNUM = 72199;
	bool isVIPEffect = itemTable->bType == ITEM_USE && itemTable->bSubType == USE_VIP && item.dwVnum != PREMIUM_TOKEN_VNUM;
	if (ch->IsActivePremiumSubscription() && isVIPEffect)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("BUY_VIP_WITH_SUBSCRIPTION_ACTIVE_ERROR"));
		return false;
	}

	if (IS_SET(itemTable->dwAntiFlags, ITEM_ANTIFLAG_STACK) || !IS_SET(itemTable->dwFlags, ITEM_FLAG_STACKABLE))
	{
		quantity = 1;
	}

	long lPrice = (long)GetItemPrice(item) * quantity;
	PlayerAccountBalance balance = GetPlayerAccountBalance(ch);
	DWORD money_before_buy;

	ERequestChargeType chargeType;
	if (item.bCurrency == CURRENCY_DRAGON_COIN)
	{
		chargeType = ERequestCharge_Cash;
		money_before_buy = balance.dwDragonCoins;
	}
	else
	{
		chargeType = ERequestCharge_Marks;
		money_before_buy = balance.dwDragonMarks;
	}

	bool isItemDragonScroll = itemTable->dwVnum >= 80014 && itemTable->dwVnum <= 80019;

	auto betaServer = g_dwClientVersion < 1000000 && quest::CQuestManager::instance().GetEventFlag("beta_server");
	if (betaServer && betaServer < 4)
	{
		if (isItemDragonScroll)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "BETA: Zakup kuponow dostepny od etapu 3+.");
			return false;
		}
	}


	if (!CheckBalance(ch, chargeType, -lPrice, balance))
		return false;

	if (IsItemTimeAuction(item) && !CheckIfCanBuyTimeAuctionAndUpdate(item, ch, quantity))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ITEMSHOP_AUCTION_NOT_AVAILABLE"));
		return false;
	}

	ch->ChatDebug("Buy item index %d (quantity %d) with price %d", itemIndex, quantity, lPrice);

	if (isItemDragonScroll && item.bCurrency == CURRENCY_DRAGON_COIN) {

		long dragonMarkAmount = lPrice - itemTable->alValues[0];
		if (dragonMarkAmount < 0)
			dragonMarkAmount = 0;

		balance.dwDragonMarks -= lPrice;
		balance.dwDragonMarks += dragonMarkAmount;
		AddCash(ch, chargeType, -lPrice, false, dragonMarkAmount);
	}
	else
	{
		AddCash(ch, chargeType, -lPrice, false);
	}
	SendBalance(ch, balance);


	std::string itemId = "null";
	if (isVIPEffect)
	{
		BYTE premiumType = itemTable->alValues[0];
		DWORD durationInHours = itemTable->alValues[1];
		AddVIP(ch, premiumType, durationInHours);
	}
	else
	{
		DWORD itemVnum = item.dwVnum;
		int itemCount = item.dwCount * quantity;
		int maxI = 0;
		int r = 0;
		DWORD max_item_stack = itemTable->dwMaxStack;
		if (itemCount > max_item_stack)
		{
			maxI = itemCount / max_item_stack;
			r = itemCount - maxI * max_item_stack;
		}
		else
		{
			r = itemCount;
		}

		for (int i = 0; i < maxI; i++) {
			ch->ChatDebug("add item %d %d", itemVnum, max_item_stack);
			LPITEM newItem = GiveItem(ch, itemIndex, itemVnum, max_item_stack);
			if (newItem)
			{
				itemId = std::to_string(newItem->GetID());
			}
		}

		if (r > 0) {
			ch->ChatDebug("add item %d %d", itemVnum, r);
			LPITEM newItem = GiveItem(ch, itemIndex, itemVnum, r);
			if (newItem)
			{
				itemId = std::to_string(newItem->GetID());
			}
		}
	}

	LogManager::instance().Query("INSERT INTO itemshop VALUES (%d, %d, %d, %d, %d, %ld, %d, %s, NOW(), %d)",
		ch->GetPlayerID(), ch->GetDesc()->GetAccountTable().id, itemIndex, item.dwVnum, quantity, lPrice, chargeType + 1, itemId.c_str(), money_before_buy);
	return true;
}

LPITEM CItemShopManager::GiveItem(LPCHARACTER ch, int itemIndex, DWORD itemVnum, DWORD itemCount)
{
	bool addToItemAward = false;
	if (ch->HasSlotForItem(itemVnum))
	{
		LPITEM pkItem = ch->AutoGiveItem(itemVnum, itemCount, -1, true, true);
		if (pkItem)
			return pkItem;

		addToItemAward = true;
	}
	else
	{
		addToItemAward = true;
	}

	if (addToItemAward)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ITEMSHOP_ITEM_AWARD"));
		DBManager::instance().DirectQuery("INSERT INTO item_award (login, vnum, count, given_time, why, mall) VALUES ('%s', %d, %d, NOW(), '%s', %u)",
			ch->GetDesc()->GetAccountTable().login, itemVnum, itemCount, "ITEMSHOP_BUY", 1);

		return nullptr;
	}
}

void CItemShopManager::AddVIP(LPCHARACTER ch, BYTE premiumType, DWORD durationInHours)
{
	if (!ch)
		return;

	if (!ch->GetDesc())
		return;

	if (premiumType >= PREMIUM_MAX_NUM)
		return;

	DWORD aid = ch->GetDesc()->GetAccountTable().id;

	std::string premiumColumnStr[PREMIUM_MAX_NUM] = {
		"silver_expire",
		"gold_expire",
		"safebox_expire",
		"autoloot_expire",
		"fish_mind_expire",
		"marriage_fast_expire",
		"money_drop_rate_expire",
		"shop_expire",
		"premium_expire",
	};
	std::string column = premiumColumnStr[premiumType];

	std::string query = "UPDATE account SET " + column + " = CASE "
		"WHEN " + column + " <= NOW() THEN DATE_ADD(NOW(), INTERVAL " + std::to_string(durationInHours) + " HOUR) "
		"ELSE DATE_ADD(" + column + ", INTERVAL " + std::to_string(durationInHours) + " HOUR) "
		"END "
		"WHERE id = " + std::to_string(aid) + ";";

	DBManager::instance().AccountQuery(query.c_str());
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ITEMSHOP_BUY_VIP_INFO"));
}

// Files shared by GameCore.top
