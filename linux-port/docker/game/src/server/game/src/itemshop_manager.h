
class CItemShopManager : public singleton<CItemShopManager>
{
	typedef std::set<DWORD> PLAYER_SET;
public:
	struct PlayerAccountBalance {
		DWORD dwDragonCoins;
		DWORD dwDragonMarks;
	};

	enum ECurrency {
		CURRENCY_DRAGON_COIN = 1,
		CURRENCY_DRAGON_MARK = 2,
	};


	CItemShopManager();
	virtual ~CItemShopManager();

	bool	Initialize(TItemShopItem* pTable, int iSize);
	void	Destroy();

	bool IsProperItem(TItemShopItem item) { return item.dwIndex > 0; }
	bool IsItemExist(int index);
	bool IsAuctionExist(int index);
	TItemShopItem GetItem(int index);
	TItemShopItem GetTimeAuctionItem(int index);
	DWORD GetItemPrice(TItemShopItem item);
	BYTE GetItemCurrency(TItemShopItem item);
	bool IsItemPromotion(TItemShopItem item);
	bool IsItemTimeAuction(TItemShopItem item);
	bool IsTimeAuctionActive(TItemShopItem item);
	bool CheckIfCanBuyTimeAuctionAndUpdate(TItemShopItem item, LPCHARACTER buyer, DWORD quantity=1);
	bool CanSendItem(LPCHARACTER buyer, TItemShopItem item);
	bool CanSendTimeAuction(TItemShopItem item);
	void SendBalance(LPCHARACTER ch, PlayerAccountBalance balance);
	void SendBalance(LPCHARACTER ch);
	void SendHappyHour(LPCHARACTER ch);
	void SendLatestTimeAuction(LPCHARACTER ch);
	void SendItemShopData(LPCHARACTER ch);
	void SendAuctionData(LPCHARACTER ch);
	void SendAuctionUpdate(LPCHARACTER ch, DWORD index);
	PlayerAccountBalance GetPlayerAccountBalance(LPCHARACTER ch);

	bool CheckBalance(LPCHARACTER ch, BYTE chargeType, long amount, PlayerAccountBalance& currentBalance, bool addMarks = true);
	bool AddCash(LPCHARACTER ch, BYTE chargeType, long amount, bool checkBalance = true, long customSecondAmount = -1);
	bool BuyItem(LPCHARACTER ch, int itemIndex, int quantity);
	LPITEM GiveItem(LPCHARACTER ch, int itemIndex, DWORD itemVnum, DWORD itemCount);
	void RefreshItemShop(LPCHARACTER ch);
	void OpenItemShop(LPCHARACTER ch, bool refresh = false);
	void CloseItemShop(LPCHARACTER ch);
	void SendUseCodeVoucher(LPCHARACTER ch, const char * code);
	void RecvUseCodeVoucher(LPCHARACTER ch, BYTE bResponse, const char * code);

	void OpenAuctionTab(LPCHARACTER ch);
	void CloseAuctionTab(LPCHARACTER ch);

	void RefreshAuctionViewers(DWORD dwAuctionIndex);

	void AddVIP(LPCHARACTER ch, BYTE premiumType, DWORD durationInHours);

	void UpdateAuctionBuyAmount(DWORD dwItemIndex, DWORD dwBuyAmount);

private:
	std::map<DWORD, TItemShopItem> m_map_ItemShopItems;
	std::map<DWORD, TItemShopItem> m_map_ItemShopTimeAuctions;

	PLAYER_SET	m_map_AuctionViewer;

	DWORD GetTimeAuctionProgress(TItemShopItem item);
	void GetPacketDataFromItem(struct TItemShopItemClient& pack, int& i, TItemShopItem item);
};

// Files shared by GameCore.top
