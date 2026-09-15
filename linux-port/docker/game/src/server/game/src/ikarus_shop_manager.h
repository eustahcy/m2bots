#ifndef __INCLUDE_HEADER_OFFLINESHOP_MANAGER__
#define __INCLUDE_HEADER_OFFLINESHOP_MANAGER__

#ifdef ENABLE_IKASHOP_RENEWAL
#define SUBTYPE_NOSET 255
#define OFFLINESHOP_DURATION_UPDATE_TIME PASSES_PER_SEC(60)
#define OFFLINESHOP_AUCTION_RAISE_PERCENTAGE 10 //to raise the offer will be 10% more than best offer

#include "packet.h"
#include "ikarus_shop.h"

namespace ikashop
{
	enum SearchConstant 
	{
		// ITEM TYPES
		SEARCH_SHOP_TYPE_WEAPON = 1,
		SEARCH_SHOP_TYPE_ARMOR,
		SEARCH_SHOP_TYPE_JEWEL,
		SEARCH_SHOP_TYPE_COSTUME,
		SEARCH_SHOP_TYPE_PET,
		SEARCH_SHOP_TYPE_MOUNT,
		SEARCH_SHOP_TYPE_ALCHEMY,
		SEARCH_SHOP_TYPE_BOOK,
		SEARCH_SHOP_TYPE_STONE,
		SEARCH_SHOP_TYPE_MINERALS,
		SEARCH_SHOP_TYPE_FISH,

		// WEAPON SUBTYPES
		SEARCH_SHOP_SUBTYPE_WEAPON_SWORD = 1,
		SEARCH_SHOP_SUBTYPE_WEAPON_TWOHANDED,
		SEARCH_SHOP_SUBTYPE_WEAPON_BLADE,
		SEARCH_SHOP_SUBTYPE_WEAPON_DAGGER,
		SEARCH_SHOP_SUBTYPE_WEAPON_BOW,
		SEARCH_SHOP_SUBTYPE_WEAPON_BELL,
		SEARCH_SHOP_SUBTYPE_WEAPON_FAN,
#ifdef ENABLE_WOLFMAN_CHARACTER
		SEARCH_SHOP_SUBTYPE_WEAPON_CLAW,
#endif

		// ARMOR SUBTYPES
		SEARCH_SHOP_SUBTYPE_ARMOR_WARRIOR = 1,
		SEARCH_SHOP_SUBTYPE_ARMOR_SURA,
		SEARCH_SHOP_SUBTYPE_ARMOR_ASSASSIN,
		SEARCH_SHOP_SUBTYPE_ARMOR_SHAMAN,
#ifdef ENABLE_WOLFMAN_CHARACTER
		SEARCH_SHOP_SUBTYPE_ARMOR_WOLFMAN,
#endif

		// JEWEL SUBTYPES
		SEARCH_SHOP_SUBTYPE_JEWEL_HELM = 1,
		SEARCH_SHOP_SUBTYPE_JEWEL_SHIELD,
		SEARCH_SHOP_SUBTYPE_JEWEL_EAR,
		SEARCH_SHOP_SUBTYPE_JEWEL_NECK,
		SEARCH_SHOP_SUBTYPE_JEWEL_WRIST,
		SEARCH_SHOP_SUBTYPE_JEWEL_SHOES,

		// COSTUME SUBTYPES
		SEARCH_SHOP_SUBTYPE_COSTUME_BODY = 1,
		SEARCH_SHOP_SUBTYPE_COSTUME_WEAPON,
		SEARCH_SHOP_SUBTYPE_COSTUME_HAIR,
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		SEARCH_SHOP_SUBTYPE_COSTUME_SASH,
#endif

		// PET SUBTYPES
		SEARCH_SHOP_SUBTYPE_PET_EGGS = 1,
		SEARCH_SHOP_SUBTYPE_PET_SEALS,

		// SASH GRADE TYPES
		SEARCH_SHOP_SASH_GRADE_VALUE0 = 1,
		SEARCH_SHOP_SASH_GRADE_VALUE1,
		SEARCH_SHOP_SASH_GRADE_VALUE2,
		SEARCH_SHOP_SASH_GRADE_VALUE3,

		// ALCHEMY GRADE TYPES
		SEARCH_SHOP_DRAGON_SOUL_GRADE_NORMAL = 1,
		SEARCH_SHOP_DRAGON_SOUL_GRADE_BRILLIANT,
		SEARCH_SHOP_DRAGON_SOUL_GRADE_RARE,
		SEARCH_SHOP_DRAGON_SOUL_GRADE_ANCIENT,
		SEARCH_SHOP_DRAGON_SOUL_GRADE_LEGENDARY,
	};


	enum EShopActionWeight 
	{
		SHOP_ACTION_WEIGHT_REQUEST_EDIT_ITEM = 1,
		SHOP_ACTION_WEIGHT_EDIT_ITEM = 5,
		SHOP_ACTION_WEIGHT_CREATE_NEW = 10,
		SHOP_ACTION_WEIGHT_FORCE_CLOSE_SHOP = 5,
		SHOP_ACTION_WEIGHT_OPEN_SHOP = 1,
		SHOP_ACTION_WEIGHT_OPEN_MY_SHOP = 1,
		SHOP_ACTION_WEIGHT_BUY_ITEM = 5,
		SHOP_ACTION_WEIGHT_MOVE_SHOP = 5,
		SHOP_ACTION_WEIGHT_ADD_ITEM = 5,
		SHOP_ACTION_WEIGHT_REMOVE_ITEM = 5,
		SHOP_ACTION_WEIGHT_REMOVE_ALL_ITEM = 400,
		SHOP_ACTION_WEIGHT_FILTER_REQUEST = 3,
		SHOP_ACTION_WEIGHT_CREATE_OFFER = 5,
		SHOP_ACTION_WEIGHT_ACCEPT_OFFER = 5,
		SHOP_ACTION_WEIGHT_CANCEL_OFFER = 5,
		SHOP_ACTION_WEIGHT_SAFEBOX_OPEN = 1,
		SHOP_ACTION_WEIGHT_SAFEBOX_GET_ITEM = 5,
		SHOP_ACTION_WEIGHT_SAFEBOX_GET_VALUTES = 5,
		SHOP_ACTION_WEIGHT_AUCTION_LIST_REQUEST = 2,
		SHOP_ACTION_WEIGHT_AUCTION_OPEN_REQUEST = 1,
		SHOP_ACTION_WEIGHT_MY_ACUTION_OPEN_REQUEST = 1,
		SHOP_ACTION_WEIGHT_MY_AUCTION_CANCEL = 5,
		SHOP_ACTION_WEIGHT_AUCTION_CREATE = 5,
		SHOP_ACTION_WEIGHT_AUCTION_ADD_OFFER = 5,
		SHOP_ACTION_WEIGHT_PRICE_AVERAGE_REQUEST = 1,
		SHOP_ACTION_WEIGHT_SHOP_DECORATION_USE = 5,
		SHOP_ACTION_WEIGHT_SHOP_MOVE_ITEM = 5,
		SHOP_ACTION_WEIGHT_SHOP_CHANGE_NAME = 10,
		SHOP_ACTION_WEIGHT_SEARCH_ITEM = 100,
	};

	enum EShopDistance {
		GUEST_INTERRACT_DISTANCE = 5000,
		OWNER_INTERRACT_DISTANCE = 1500,
	};


	class CShopManager : public singleton<CShopManager>
	{
	public:

		using ITEM_HANDLE = std::shared_ptr<ikashop::CShopItem>;
		using OFFER_HANDLE = std::shared_ptr<ikashop::TOfferInfo>;
		using SHOP_HANDLE = std::shared_ptr<ikashop::CShop>;
		using SAFEBOX_HANDLE = std::shared_ptr<ikashop::CShopSafebox>;
		using AUCTION_HANDLE = std::shared_ptr<CAuction>;

		using SHOP_LIMIT_MAP = std::map<DWORD, TOfflineShopLimit>;
		using SHOPMAP = std::map<DWORD, SHOP_HANDLE>;
		using SAFEBOXMAP = std::map<DWORD, SAFEBOX_HANDLE>;
		using OFFERSMAPBUYER = std::map<DWORD, std::vector<OFFER_HANDLE>>;
		using OFFERSMAP = std::map<DWORD, OFFER_HANDLE>;
		using SEARCHTIMEMAP = std::map<DWORD, DWORD>;
		using AUCTIONMAP = std::map<DWORD, AUCTION_HANDLE>;

#ifdef EXTEND_IKASHOP_PRO
		using NOTIFICATION_MAP = std::unordered_map<DWORD, std::vector<TNotificationInfo>>;
#endif

#ifdef ENABLE_IKASHOP_ENTITIES
		using ENTITY_HANDLE = std::shared_ptr<ShopEntity>;
		using ENTITY_MAP = std::unordered_map<DWORD, ENTITY_HANDLE>;
#endif
		
		CShopManager();
		~CShopManager();

		//booting
		SHOP_HANDLE PutsNewShop(const TShopInfo& info);
		void PutsAuction(const TAuctionInfo& auction);
		void PutsAuctionOffer(const TAuctionOfferInfo& offer);

		void PrepareShopSearchFilters();

		// Read-only native ledger, including owners currently absent from this map.
		const SHOPMAP& GetPlayerBotOfflineShops() const { return m_mapShops; }
		SHOP_HANDLE GetShopByOwnerID(DWORD pid);
		SAFEBOX_HANDLE GetShopSafeboxByOwnerID(DWORD pid);
		AUCTION_HANDLE GetAuctionByOwnerID(DWORD pid);

		//offers
		bool PutsNewOffer(OFFER_HANDLE pInfo);


#ifdef ENABLE_IKASHOP_ENTITIES
	public:
		void CreateShopEntity(SHOP_HANDLE shop);
		void DestroyShopEntity(SHOP_HANDLE shop);			 
		void EncodeInsertShopEntity(const ShopEntity& shop, LPCHARACTER ch);
		void EncodeRemoveShopEntity(const ShopEntity& shop, LPCHARACTER ch);

#ifdef EXTEND_IKASHOP_PRO
	private:
		void _UpdateEntity(SHOP_HANDLE shop);
#endif
#endif


	public:
//packets exchanging db
//ITEMS
/*db*/	void SendShopBuyDBPacket(DWORD buyerid, DWORD ownerid,DWORD itemid, bool success);
/*db*/	void SendShopLockBuyItemDBPacket(LPCHARACTER buyer, DWORD ownerid, ITEM_HANDLE item, long long seenprice); //patch seen price check
			 
/*db*/	bool RecvShopLockedBuyItemDBPacket(DWORD buyerid, DWORD ownerid,DWORD itemid);
/*db*/	bool RecvShopBuyDBPacket(DWORD buyerid, DWORD ownerid, DWORD itemid, bool requester);
			 
/*db*/	void SendShopEditItemDBPacket(DWORD ownerid, DWORD itemid, const TPriceInfo& price);
/*db*/	bool RecvShopEditItemDBPacket(DWORD ownerid, DWORD itemid, const TPriceInfo& price);
			 
			 
/*db*/	void SendShopRemoveItemDBPacket(DWORD ownerid, DWORD itemid);
/*db*/	bool RecvShopRemoveItemDBPacket(DWORD ownerid, DWORD itemid, bool requester);
			 
			 
/*db*/	void SendShopAddItemDBPacket(DWORD ownerid, const TShopItem& iteminfo);
/*db*/	bool RecvShopAddItemDBPacket(DWORD ownerid, const TShopItem& iteminfo);
			 
//SHOPS 	 
/*db*/	void SendShopForceHardCloseDBPacket(DWORD pid);
/*db*/	void SendShopForceSoftCloseDBPacket(DWORD pid, bool gm);
/*db*/	bool RecvShopForceCloseDBPacket(DWORD pid);
/*db*/	bool RecvShopExpiredDBPacket(DWORD pid);
			 
/*db*/	void SendShopCreateNewDBPacket(const TShopInfo&, const TShopItem*);
/*db*/	bool RecvShopCreateNewDBPacket(const TShopInfo&, const TShopItem*);
			 
/*db*/	void SendShopChangeNameDBPacket(DWORD ownerid, const char* name);
/*db*/	bool RecvShopChangeNameDBPacket(DWORD ownerid, const char* name);	

/*db*/	void SendShopOwnerOnlineDBPacket(DWORD ownerID, const bool ownerOnlineState);
/*db*/	bool RecvShopOwnerOnlineDBPacket(DWORD ownerID, const bool ownerOnlineState);
			 
//OFFER		 
/*db*/	void SendShopOfferNewDBPacket(const TOfferInfo& offer, ITEM_HANDLE item);
/*db*/	bool RecvShopOfferNewDBPacket(const TOfferInfo& offer);
			 
/*db*/	void SendShopOfferNotifiedDBPacket(DWORD offerid, DWORD ownerid);
/*db*/	bool RecvShopOfferNotifiedDBPacket(DWORD offerid, DWORD ownerid);
			 
/*db*/	void SendShopOfferAcceptDBPacket(const TOfferInfo& offer);
/*db*/	bool RecvShopOfferAcceptDBPacket(DWORD offerid, DWORD ownerid);
			 
/*db*/	void SendShopOfferCancelDBPacket(const TOfferInfo& offer, DWORD requester);
/*db*/	bool RecvShopOfferCancelDBPacket(DWORD offerid, DWORD ownerid, bool removing);			 
			 
/*db*/	void SendShopSafeboxGetItemDBPacket(DWORD ownerid, DWORD itemid);
/*db*/	void SendShopSafeboxGetValutesDBPacket(DWORD ownerid, const TValutesInfo& valutes);
/*db*/  bool SendShopSafeboxAddItemDBPacket(DWORD ownerid, const CShopItem& item);
/*db*/	bool RecvShopSafeboxAddItemDBPacket(DWORD ownerid, DWORD itemid, const TShopPlayerItem& item);
/*db*/	bool RecvShopSafeboxAddValutesDBPacket(DWORD ownerid, const TValutesInfo& valute);
/*db*/	bool RecvShopSafeboxLoadDBPacket(DWORD ownerid, const TValutesInfo& valute, const std::vector<TShopPlayerItem>& items);
//patch 08-03-2020
/*db*/  bool RecvShopSafeboxExpiredItemDBPacket(DWORD ownerid, DWORD itemid);
/*db*/	bool RecvShopSafeboxGetItemConfirm(DWORD ownerid, DWORD itemid);

/*db*/	void SendShopLoadExpiredShop(DWORD ownerid);
			 
//AUCTION	 
/*db*/	void SendAuctionCreateDBPacket(const TAuctionInfo& auction);
/*db*/	void SendAuctionAddOfferDBPacket(const TAuctionOfferInfo& offer);
			 
/*db*/	bool RecvAuctionCreateDBPacket(const TAuctionInfo& auction);
/*db*/	bool RecvAuctionAddOfferDBPacket(const TAuctionOfferInfo& offer);
/*db*/	bool RecvAuctionExpiredDBPacket(DWORD id);
/*db*/	bool RecvAuctionOfferSeenDBPacket(DWORD ownerid, DWORD buyerid, long long price);

#ifdef EXTEND_IKASHOP_PRO
/*db*/	bool RecvNotificationLoadDBPacket(DWORD owner, const std::vector<TNotificationInfo>& notifications);
/*db*/	bool RecvNotificationForwardDBPacket(DWORD owner, const TNotificationInfo& notification);
/*db*/	void SendShopRestoreDurationDBPacket(DWORD owner);
/*db*/	bool RecvShopRestoreDurationDBPacket(DWORD owner);
#ifdef ENABLE_IKASHOP_ENTITIES
/*db*/	void RecvMoveShopEntityDBPacket(DWORD owner, const TShopSpawn& spawn);
#endif
#endif
		void SendShopLockedSlotStateDBPacket(DWORD pid, BYTE unlockCount, bool isPremium);
		void RecvShopLockedSlotStateDBPacket(DWORD ownerID, BYTE unlockCount, bool isPremium);

//packets echanging clients
//SHOPS
///*cli.*/bool RecvShopCreateNewClientPacket(LPCHARACTER ch); // UNUSED
/*cli.*/bool RecvShopReopenClientPacket(LPCHARACTER ch, const char * name, BYTE bTimeIndex);
/*cli.*/bool RecvShopForceCloseClientPacket(LPCHARACTER ch);
/*cli.*/bool RecvShopOpenClientPacket(LPCHARACTER ch, DWORD ownerid);
/*cli.*/bool RecvShopOpenMyShopClientPacket(LPCHARACTER ch);
/*cli.*/bool RecvShopBuyItemClientPacket(LPCHARACTER ch, DWORD ownerid, DWORD itemid, bool searching, long long seenprice); //patch seen price check
#ifdef ENABLE_IKASHOP_ENTITIES
/*cli.*/bool RecvShopClickEntityClientPacket(LPCHARACTER ch, DWORD evid);
/*cli.*/bool RecvFindMyShopClientPacket(LPCHARACTER ch);
#ifdef EXTEND_IKASHOP_PRO
/*cli.*/bool RecvMoveShopEntityClientPacket(LPCHARACTER ch);
#endif
#endif

		bool SearchItemsByCategory(DWORD category, ikashop::CShopManager::SHOP_HANDLE shop);
		bool RecvShopSearchItemClientPacket(LPCHARACTER ch, DWORD itemVnum, int socket0);

/*cli.*/void SendSearchResultDeleteClientPacket(LPCHARACTER ch, DWORD itemid);
/*cli.*/void SendShopOpenClientPacket(LPCHARACTER ch, CShop* shop);
/*cli.*/void SendShopOpenMyShopClientPacket(LPCHARACTER ch);
/*cli.*/void SendShopOpenMyShopNoShopClientPacket(LPCHARACTER ch);
/*cli.*/void SendShopExpiredGuesting(DWORD ownerid, LPCHARACTER ch);
			 
/*cli.*/void SendShopForceClosedClientPacket(DWORD ownerid);
/*cli.*/void SendMyShopPositionClientPacket(LPCHARACTER ch, DWORD shopVID, long x, long y);


		//ITEMS
		void ProcessAddItemToShop(TShopItem& sitem, LPCHARACTER ch, LPITEM item, WORD destPos, const TPriceInfo price);

/*cli.*/bool RecvShopAddItemClientPacket(LPCHARACTER ch, const TItemPos& item, const TPriceInfo& price, int destPos);
/*cli.*/bool RecvShopRemoveItemClientPacket(LPCHARACTER ch, DWORD itemid);
/*cli.*/bool RecvShopRemoveAllItemClientPacket(LPCHARACTER ch);
/*cli.*/bool RecvShopEditItemClientPacket(LPCHARACTER ch, DWORD itemid, const TPriceInfo& price);
/*cli.*/bool RecvShopRequestEditClientPacket(LPCHARACTER ch, bool isEdit);
/*cli.*/bool RecvShopChangeNameClientPacket(LPCHARACTER ch, const char * szName);

		//FILTER
/*cli.*/bool RecvShopFilterRequestClientPacket(LPCHARACTER ch, const TFilterInfo& filter);
/*cli.*/bool RecvShopSearchFillRequestClientPacket(LPCHARACTER ch);
/*cli.*/void SendShopFilterResultClientPacket(LPCHARACTER ch, const std::vector<TSearchResultInfo>& items);


		//OFFERS
/*cli.*/bool RecvShopCreateOfferClientPacket(LPCHARACTER ch, TOfferInfo offer);
/*cli.*/bool RecvShopAcceptOfferClientPacket(LPCHARACTER ch, DWORD offerid);
/*cli.*/bool RecvShopCancelOfferClientPacket(LPCHARACTER ch, DWORD offerid, DWORD ownerid);
/*cli.*/bool RecvOfferListRequestPacket(LPCHARACTER ch);

		
		//SAFEBOX
/*cli.*/bool RecvShopSafeboxOpenClientPacket(LPCHARACTER ch);
/*cli.*/bool RecvShopSafeboxGetItemClientPacket(LPCHARACTER ch, DWORD itemid);
/*cli.*/bool RecvShopSafeboxGetValutesClientPacket(LPCHARACTER ch);
/*cli.*/bool RecvShopSafeboxCloseClientPacket(LPCHARACTER ch);
		

		//AUCTION
/*cli.*/bool RecvAuctionListRequestClientPacket(LPCHARACTER ch);
/*cli.*/bool RecvAuctionOpenRequestClientPacket(LPCHARACTER ch, DWORD ownerid);
/*cli.*/bool RecvMyAuctionOpenRequestClientPacket(LPCHARACTER ch);
/*cli.*/bool RecvMyAuctionCloseClientPacket(LPCHARACTER ch);
/*cli.*/bool RecvMyAuctionCancelClientPacket(LPCHARACTER ch);
/*cli.*/bool RecvAuctionCreateClientPacket(LPCHARACTER ch, const TPriceInfo& startprice, const TItemPos& pos);
/*cli.*/bool RecvAuctionAddOfferClientPacket(LPCHARACTER ch, DWORD ownerid, const TPriceInfo& price);
/*cli.*/bool RecvAuctionExitFromAuction(LPCHARACTER ch, DWORD ownerid);
			 
/*cli.*/void SendAuctionListClientPacket(LPCHARACTER ch, const std::vector<TAuctionListElement>& auctionVec);
/*cli.*/void SendAuctionOpenAuctionClientPacket(LPCHARACTER ch, const CAuction& auction); 
/*cli.*/void SendAuctionOpenMyAuctionNoAuctionClientPacket(LPCHARACTER ch);
			 
			 
/*cli.*/void SendShopSafeboxRefreshClientPacket(LPCHARACTER ch, const TValutesInfo& valute, const std::vector<ITEM_HANDLE>& vec);
			 
/*cli.*/void RecvCloseMyShopBoardClientPacket(LPCHARACTER ch);
/*cli.*/void RecvCloseOfferListClientPacket(LPCHARACTER ch);
/*cli.*/void RecvCloseShopGuestClientPacket(LPCHARACTER ch);

#ifdef EXTEND_IKASHOP_PRO
/*cli.*/void SendNotificationListClientPacket(LPCHARACTER ch);
#endif
/*cli.*/void SendBoardCountersClientPacket(LPCHARACTER ch);


/*cli.*/void SendPopupMessage(LPCHARACTER ch, const std::string& message);

		//other
		void UpdateShopsDuration();
		void UpdateAuctionsDuration();

		//search time map (checking to avoid search abouse)
		void ClearSearchTimeMap();
		bool CheckSearchCooldown(DWORD pid);
		bool CheckOfferCooldown(DWORD pid);

		bool CheckSafeboxSize(LPCHARACTER ch);

		bool OpenOfflineShop(LPCHARACTER ch, const char * shopSign, TShopItemTable* pItemTable, BYTE bItemCount, BYTE bTimeIndex);

		bool IsNearShop(LPCHARACTER ch, SHOP_HANDLE shop, long maxDistance=GUEST_INTERRACT_DISTANCE);

		void Destroy();

		bool ParseShopName(const char* name, std::string& output);

		bool CanOpenOnMap(LPCHARACTER ch);
		void LoadShopLimits(const TOfflineShopLimit* limits, WORD size);

		void NotifyOwnerItemSold(LPCHARACTER ch, DWORD item_vnum, ITEM_COUNT item_count, YANG price);
		 

	private:
		void _RemoveShopOffers(SHOP_HANDLE shop);

	private:
		SHOP_LIMIT_MAP m_mapLimitShops;
		SHOPMAP m_mapShops;
		SAFEBOXMAP m_mapSafeboxs;
		OFFERSMAPBUYER m_mapOffersByBuyer;
		OFFERSMAP m_mapOffer;

		LPEVENT m_eventShopDuration;
		SEARCHTIMEMAP m_searchCooldownMap;
		SEARCHTIMEMAP m_offerCooldownMap;
		AUCTIONMAP m_mapAuctions;
#ifdef ENABLE_IKASHOP_ENTITIES
		ENTITY_MAP m_entityByVID;
#endif
#ifdef EXTEND_IKASHOP_PRO
		NOTIFICATION_MAP m_notificationMap;
#endif

		struct SOfflineShopFilter {
			DWORD itemVnum;
			int socket0;
		};
		std::map<DWORD, std::vector<SOfflineShopFilter>> m_shopSearchFilters;
	};

	ikashop::CShopManager& GetManager();
}

#endif
#endif //__INCLUDE_HEADER_OFFLINESHOP_MANAGER__

// Files shared by GameCore.top
