#ifndef __INCLUDE_NEW_OFFLINESHOP_HEADER__
#define __INCLUDE_NEW_OFFLINESHOP_HEADER__

#ifdef ENABLE_IKASHOP_RENEWAL
#define __USE_PID_AS_GUESTLIST__
#include <ranges>
#include <array>
#include "item.h"

//copyarray
template <class T, size_t size>
void CopyArray(T (&objDest)[size] , const T (&objSrc)[size]){
	if(size==0)
		return;
	memcpy(&objDest[0] , &objSrc[0], sizeof(T)*size);
}

template <class T>
void DeletePointersContainer(T& obj){
	typename T::iterator it = obj.begin();
	for(; it != obj.end(); it++)
		delete(*it);
}

namespace ikashop
{
	//patch 08-03-2020
	inline ikashop::ExpirationType GetItemExpiration(LPITEM item) {
		auto proto = item->GetProto();
		for (const auto limit : proto->aLimits) {
			if (limit.bType == LIMIT_REAL_TIME)
				return ikashop::ExpirationType::EXPIRE_REAL_TIME;
			else if (limit.bType == LIMIT_REAL_TIME_START_FIRST_USE && item->GetSocket(1) != 0)
				return ikashop::ExpirationType::EXPIRE_REAL_TIME_FIRST_USE;
		} return ikashop::ExpirationType::EXPIRE_NONE;
	}


	enum eOffshopChatPacket
	{
		CHAT_PACKET_CANNOT_CREATE_SHOP,
		CHAT_PACKET_CANNOT_CHANGE_NAME,
		CHAT_PACKET_CANNOT_FORCE_CLOSE,
		CHAT_PACKET_CANNOT_OPEN_SHOP,
		CHAT_PACKET_CANNOT_OPEN_SHOP_OWNER,

		CHAT_PACKET_CANNOT_ADD_ITEM,
		CHAT_PACKET_CANNOT_BUY_ITEM, //tofix wrong chat packet
		CHAT_PACKET_CANNOT_REMOVE_ITEM,
		CHAT_PACKET_CANNOT_EDIT_ITEM,
		CHAT_PACKET_CANNOT_REMOVE_LAST_ITEM,

		CHAT_PACKET_CANNOT_FILTER,
		CHAT_PACKET_CANNOT_SEARCH_YET,
		CHAT_PACKET_OFFER_CREATE,

		CHAT_PACKET_CANNOT_CREATE_OFFER,
		CHAT_PACKET_CANNOT_ACCEPT_OFFER,

		CHAT_PACKET_CANNOT_OPEN_SAFEBOX,
		CHAT_PACKET_CANNOT_SAFEBOX_GET_ITEM,
		CHAT_PACKET_CANNOT_SAFEBOX_GET_VALUTES,
		CHAT_PACKET_CANNOT_SAFEBOX_CLOSE,


		CHAT_PACKET_RECV_ITEM_SAFEBOX,

		//AUCTION
		CHAT_PACKET_AUCTION_CANNOT_SEND_LIST,
		CHAT_PACKET_AUCTION_CANNOT_OPEN_AUCTION,
		CHAT_PACKET_AUCTION_CANNOT_CREATE_AUCTION,
		CHAT_PACKET_AUCTION_CANNOT_ADD_OFFER,

		//GENERAL
		CHAT_PACKET_CANNOT_DO_NOW,

	};


	class CShopItem
	{
		using ItemID = DWORD;
		using OwnerID = DWORD;

	public:
		TItemTable* GetTable() const;
		LPITEM CreateItem() const;

		const TPriceInfo& GetPrice() const;
		void SetPrice(const TPriceInfo& price);
		
		const TShopItem& GetInfo() const;
		void SetInfo(const TShopItem& info);

		DWORD GetVnum() const { return m_info.vnum; }
		ItemID GetID() const;
		bool CanBuy(LPCHARACTER ch);

		void SetCell(int pos);

	protected:
		TShopItem m_info{};

	};

	template<int WIDTH, int HEIGHT>
	class CShopGrid
	{
		static constexpr auto CELL_COUNT = WIDTH*HEIGHT;

	public:
		void Clear()
		{
			m_cells = {};
		}

		void RegisterItem(int cell, int size)
		{
			const auto [col, row] = _DecomposeCell(cell);
			for(int i=0; i < size; i++)
				_SetCellState(_ComposeCell(col, row+i), true);
		}

		void UnregisterItem(int cell, int size)
		{
			const auto [col, row] = _DecomposeCell(cell);
			for (int i = 0; i < size; i++)
				_SetCellState(_ComposeCell(col, row + i), false);
		}

		bool CheckSpace(int cell, int size)
		{
			const auto [col, row] = _DecomposeCell(cell);
			for(int nrow = row; nrow < row + size; nrow++)
				if(_GetCellState(_ComposeCell(col, nrow)))
					return false;
			return true;
		}

		std::optional<int> FindSpace(int size)
		{
			for(int cell = 0; cell < CELL_COUNT; cell++)
				if(CheckSpace(cell, size))
					return cell;
			return std::nullopt;
		}

		constexpr int GetCellCount(){
			return WIDTH*HEIGHT;
		}

	private:
		std::pair<int,int> _DecomposeCell(int cell){ return { cell % WIDTH, cell / WIDTH }; }
		int _ComposeCell(int col, int row){ return row * WIDTH + col; }
		void _SetCellState(int cell, bool state){ if(cell >= 0 && cell < CELL_COUNT) m_cells[cell] = state; }
		bool _GetCellState(int cell) { return cell >= 0 && cell < CELL_COUNT ? m_cells[cell] : true; }

	private:
		std::array<bool, WIDTH*HEIGHT> m_cells{};
	};

	class ShopEntity;

	class CShop
	{
	public:
		static constexpr auto GRID_WIDTH = SHOP_PLAYER_WIDTH;
		static constexpr auto GRID_HEIGHT = SHOP_PLAYER_HEIGHT;

	public:
		using ITEM_HANDLE = std::shared_ptr<CShopItem>;
		using OFFER_HANDLE = std::shared_ptr<TOfferInfo>;

		using SHOP_ITEM_MAP = std::map<DWORD, ITEM_HANDLE>;
		using OFFER_MAP = std::map<DWORD, OFFER_HANDLE>;

		using VECGUEST = std::set<DWORD>;
		using SHOPNAME = std::string;
#ifdef ENABLE_IKASHOP_ENTITIES
		using ENTITY_HANDLE = std::shared_ptr<ShopEntity>;
#endif
	public:
		// get const 
		const SHOP_ITEM_MAP& GetItems() const;
		const OFFER_MAP& GetOffers() const;
		const VECGUEST& GetGuests() const;

		// duration
		void SetDuration(DWORD duration);
		void DecreaseDuration();
		DWORD GetDuration() const;
		DWORD GetGuestDuration() const;

		void SetOwnerOnline(bool state) { bIsOwnerOnline = state; }
		bool IsOwnerOnline() { return bIsOwnerOnline; }

		// owner pid
		void SetOwnerPID(DWORD ownerid);
		DWORD GetOwnerPID() const;

		// guests
		bool AddGuest(LPCHARACTER ch);
		bool RemoveGuest(LPCHARACTER ch);
		void KickAllGuests();

		// items
		bool AddItem(const TShopItem& rItem, bool updateGuestView=true);
		bool RemoveItem(DWORD itemid);
		bool ModifyItemPrice(DWORD itemid, const TPriceInfo& price);
		bool BuyItem(DWORD itemid);
		ITEM_HANDLE GetItem(DWORD itemid);
		YANG GetTotalYangValue();

		bool HasItem(DWORD itemVnum, int socket0=0);
		bool HasAttrItem();
		bool HasWeapon();
		bool HasArmor();
		bool HasJewelry();
		bool HasItemType(BYTE type, BYTE subtype, bool checkAttribute=false);
		bool HasSoulStoneSocket(BYTE level=0);
			 
		// offers
		bool AddOffer(OFFER_HANDLE offer);
		bool AcceptOffer(OFFER_HANDLE offer);
		void NotifyOffers(LPCHARACTER ch);
		void NotifyAcceptedOffers(LPCHARACTER ch);
		void RemoveOffer(DWORD offerid);

		// helpers
		LPCHARACTER FindOwnerCharacter();
		void Clear();
		void RefreshToOwner();
		void SetName(const SHOPNAME& name);
		void SetOwnerName(const std::string& name);
		const char* GetName() const;
		const char* GetOwnerName() const;

		// grid checks
		bool ReserveSpace(int cell, int size);

		int GetNewOffersCounter() const;

		void SetPremium(bool state) { bIsPremium = state; }
		bool IsPremium() { return bIsPremium; }
		void SetUnlockCount(BYTE unlockCount) { m_unlockCount = unlockCount; }
		BYTE GetUnlockCount() { return m_unlockCount; }

		DWORD GetLastEditTime() { return m_LastEditTime; }

		void SetEntity(ENTITY_HANDLE handle);
		ENTITY_HANDLE GetEntity() const;

#ifdef ENABLE_IKASHOP_ENTITIES
		void SetSpawn(const TShopSpawn& spawn);
		const TShopSpawn& GetSpawn() const;
#endif

		void SetEditMode(bool state);
		bool IsEditMode() { return bIsEdited; }

	private:
		void _RefreshViews(LPCHARACTER ch = nullptr);
		void _SendNotifyOffer(LPCHARACTER ch, const TOfferInfo& offer);

	private:
		ENTITY_HANDLE m_entity;
		SHOP_ITEM_MAP m_items{};
		OFFER_MAP m_offers{};
		VECGUEST m_guests{};
		DWORD m_pid{};
		DWORD m_duration{};
		SHOPNAME m_name{};
		std::string m_ownerName{};
		CShopGrid<GRID_WIDTH, GRID_HEIGHT> m_grid;
		bool bIsOwnerOnline = false;
#ifdef ENABLE_IKASHOP_ENTITIES
		TShopSpawn m_spawn;
#endif
		BYTE m_unlockCount{};
		bool bIsPremium = false;
		bool bIsEdited = false;

		DWORD m_LastEditTime{};
	};


	class CShopSafebox
	{
	public:
		static constexpr auto GRID_WIDTH = SHOP_PLAYER_WIDTH;
		static constexpr auto GRID_HEIGHT = SHOP_PLAYER_HEIGHT;

	public:
		using ITEM_HANDLE = std::shared_ptr<CShopItem>;
		using ITEM_MAP = std::map<DWORD, ITEM_HANDLE>;

	public:
		CShopSafebox() = default;
		CShopSafebox(const CShopSafebox&) = default;
		CShopSafebox(LPCHARACTER chOwner);

		void SetOwner(LPCHARACTER ch);
		void SetValuteAmount(TValutesInfo val);
			 
		bool AddItem(const TShopPlayerItem& item);
		bool RemoveItem(DWORD itemid);
			 
		void AddValute(TValutesInfo val);
		bool RemoveValute(TValutesInfo val);

		const ITEM_MAP& GetItems() const;
		const DWORD GetLeftSpace() const;
		const TValutesInfo& GetValutes() const;

		ITEM_HANDLE GetItem(DWORD itemid) const;
		LPCHARACTER GetOwner() const;

		bool RefreshToOwner(LPCHARACTER ch = nullptr);

	private:
		int _RegisterGridPosition(ITEM_HANDLE item);
		void _UnregisterGridPosition(ITEM_HANDLE item);


	private:
		ITEM_MAP m_items{};
		LPCHARACTER m_owner{};
		TValutesInfo m_valutes{};
		std::vector<CShopGrid<GRID_WIDTH, GRID_HEIGHT>> m_grids;
	};


	class CAuction
	{
	public:
		using AUCTION_OFFER_HANDLE = std::shared_ptr<TAuctionOfferInfo>;
		using AUCTION_OFFERVEC = std::vector<AUCTION_OFFER_HANDLE>;
		using GUESTVEC = std::vector<DWORD>;

	public:
		void SetInfo(const TAuctionInfo& auction);
		void SetOffers(const std::vector<TAuctionOfferInfo>& vec);

		bool AddOffer(const TAuctionOfferInfo& offer, bool check = true);
		bool AddGuest(LPCHARACTER ch);
		bool RemoveGuest(LPCHARACTER ch);
		void DecreaseDuration();

		//adding 1 minute time when new offer is done in the last minute
		void IncreaseDuration();
		
		const GUESTVEC& GetGuests() const;
		const DWORD	GetBestBuyer() const;
		const TAuctionInfo& GetInfo() const;
		const AUCTION_OFFERVEC& GetOffers() const;
		AUCTION_OFFER_HANDLE GetBestOffer() const;

		void RefreshToOwner(LPCHARACTER ch = nullptr);

		int GetNewOffersCounter() const;


	private:
		void RefreshGuestView();
		bool CheckBestOffer();

	private:
		TAuctionInfo m_info{};
		AUCTION_OFFERVEC m_offers{};
		AUCTION_OFFER_HANDLE m_bestOffer;
		GUESTVEC m_guests{};

	};

#ifdef ENABLE_IKASHOP_ENTITIES
	class ShopEntity : public CEntity
	{
	private:
		static DWORD AllocID() {
			static auto id = 0UL;
			return ++id;
		}

	public:
		using SHOP_HANDLE = std::shared_ptr<CShop>;

	public:
		ShopEntity();

		//overriding virtual CEntity methods
		void EncodeInsertPacket(LPENTITY entity);
		void EncodeRemovePacket(LPENTITY entity);
		
		const char* GetShopName() const;
		void SetShopName(const char* name);

		const char* GetOwnerName() const;
		void SetOwnerName(const char* name);

		int GetShopType() const;
		void SetShopType(int iType);
		void SetShop(SHOP_HANDLE shop);

		SHOP_HANDLE GetShop() {return m_shop;}
		void Destroy() { CEntity::Destroy(); }

		DWORD GetVID() const { return m_vid; }

	private:
		std::string m_name{};
		std::string m_ownerName{};
		int m_type{};
		SHOP_HANDLE m_shop{};
		DWORD m_vid = AllocID();
	};
#endif
}

#endif

#endif //__include



// Files shared by GameCore.top
