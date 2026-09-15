#include "constants.h"

class CSpecialShopManager : public singleton<CSpecialShopManager>
{
	typedef std::vector<DWORD> TSpecialShopItemList;
	typedef std::map<DWORD, TSpecialShopItem> TSpecialShopItemMap;
	typedef std::map<DWORD, TSpecialShopItemList> TSpecialShopMap;

	// PIDs
	typedef std::set<DWORD> GUEST_SET;

public:
	CSpecialShopManager();
	virtual ~CSpecialShopManager();

	bool					InitializeItems(TSpecialShopItem* table, int size);
	bool					InitializeShops(TSpecialShopTable* table, int size);
	const TSpecialShopItem*	GetSpecialShopItem(DWORD id);
	TSpecialShopItemList	GetSpecialShopItemList(DWORD shop_vnum);
	std::vector<TRefineMaterial>	GetRandomItems(DWORD seed, const TSpecialShopItem* item);
	TSpecialShopMetaData		GetShopMetaData(DWORD shop_vnum);
	void					SetShopSeed(DWORD shop_vnum, DWORD seed);
	void					SetShopMeta(DWORD shop_vnum, TSpecialShopMetaData meta, bool send_db=false);
	bool					CheckShopOfferChange(DWORD shop_vnum);

	bool					IsSpecialShopExist(DWORD shop_vnum);
	bool					IsItemInSpecialShop(DWORD shop_vnum, DWORD item_vnum);
	TSpecialShopLimit		GetLimit(DWORD vnum, BYTE limitType);

	void					SendShopInfo(LPCHARACTER ch, DWORD shop_vnum, LPCHARACTER seller);
	void					SendBuyInfo(LPCHARACTER ch, DWORD vnum);
	void					BuyItem(LPCHARACTER ch, DWORD vnum);
	void					OpenShop(LPCHARACTER ch, LPCHARACTER seller, DWORD shop_vnum);
	void					CloseShop(LPCHARACTER ch);

	void					AddGuest(DWORD shop_vnum, DWORD pid);
	void					UpdateAllGuests(DWORD shop_vnum);
	void					RemoveAllGuests(DWORD shop_vnum);
	void					RemoveGuest(DWORD shop_vnum, DWORD pid);
	bool					IsPlayerGuest(DWORD shop_vnum, DWORD pid);
	GUEST_SET&				GetGuestSet(DWORD shop_vnum);

private:
	TSpecialShopMap    m_map_SpecialShop;
	TSpecialShopItemMap    m_map_SpecialShopItems;

	// shop_vnum, guest_set
	std::map<DWORD, GUEST_SET>    m_map_SpecialShopGuestMap;

	std::map<DWORD, TSpecialShopMetaData>    m_map_SpecialShopMetaData;

};

// Files shared by GameCore.top
