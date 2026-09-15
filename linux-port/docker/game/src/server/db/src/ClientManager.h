#ifndef __INC_CLIENTMANAGER_H__
#define __INC_CLIENTMANAGER_H__

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include "../../common/stl.h"
#include "../../common/building.h"

#include "Peer.h"
#include "DBManager.h"
#include "LoginData.h"
#ifdef ENABLE_IKASHOP_RENEWAL
#include <rapidjson/rapidjson.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <fmt/fmt.h>
#include "../../common/json_helper.h"
#include "IkarusShopCache.h"
#endif

#define ENABLE_PROTO_FROM_DB
#ifdef ENABLE_IKASHOP_RENEWAL
inline std::optional<rapidjson::Document> str_to_json(const char* str) {
	if (!str || str[0] == 0)
		return std::nullopt;

	rapidjson::Document ret;
	ret.Parse(str);

	if (ret.HasParseError()) {
		sys_err("Json Document has parse errors : %d (str %s)", (int)ret.GetParseError(), str);
		return std::nullopt;
	}

	return ret;
}
#endif

class CPlayerTableCache;
class CItemCache;
class CItemPriceListTableCache;

class CPacketInfo
{
    public:
	void Add(int header);
	void Reset();

	std::map<int, int> m_map_info;
};

size_t CreatePlayerSaveQuery(char * pszQuery, size_t querySize, TPlayerTable * pkTab);

class CClientManager : public CNetBase, public singleton<CClientManager>
{
    public:
	typedef std::list<CPeer *>			TPeerList;
	typedef boost::unordered_map<DWORD, CPlayerTableCache *> TPlayerTableCacheMap;
	typedef boost::unordered_map<DWORD, CItemCache *> TItemCacheMap;
	typedef boost::unordered_set<CItemCache *, boost::hash<CItemCache*> > TItemCacheSet;
	typedef boost::unordered_map<DWORD, TItemCacheSet *> TItemCacheSetPtrMap;
	typedef boost::unordered_map<DWORD, CItemPriceListTableCache*> TItemPriceListCacheMap;
	typedef boost::unordered_map<short, BYTE> TChannelStatusMap;

	// MYSHOP_PRICE_LIST

	typedef std::pair< DWORD, DWORD >		TItemPricelistReqInfo;
	// END_OF_MYSHOP_PRICE_LIST

	class ClientHandleInfo
	{
	    public:
		DWORD	dwHandle;
		DWORD	account_id;
		bool	is_bot;
		DWORD	player_id;
		BYTE	account_index;
		char	login[LOGIN_MAX_LEN + 1];
		char	safebox_password[SAFEBOX_PASSWORD_MAX_LEN + 1];
		char	ip[MAX_HOST_LENGTH + 1];

		TAccountTable * pAccountTable;
		TSafeboxTable * pSafebox;

		ClientHandleInfo(DWORD argHandle, DWORD dwPID = 0)
		{
		    dwHandle = argHandle;
		    pSafebox = NULL;
		    pAccountTable = NULL;
		    player_id = dwPID;
		    account_id = 0;
		    is_bot = false;
		};

		ClientHandleInfo(DWORD argHandle, DWORD dwPID, DWORD accountId, bool argIsBot = false)
		{
		    dwHandle = argHandle;
		    pSafebox = NULL;
		    pAccountTable = NULL;
		    player_id = dwPID;
			account_id = accountId;
			is_bot = argIsBot;
		};

		~ClientHandleInfo()
		{
		    if (pSafebox)
			{
				delete pSafebox;
				pSafebox = NULL;
			}
		}
	};

	public:
	CClientManager();
	~CClientManager();

	bool	Initialize();
	time_t	GetCurrentTime();

	void	MainLoop();
	void	Quit();

	void	GetPeerP2PHostNames(std::string& peerHostNames);
	void	SetTablePostfix(const char* c_pszTablePostfix);
	void	SetPlayerIDStart(int iIDStart);
	int		GetPlayerIDStart() { return m_iPlayerIDStart; }

	int		GetPlayerDeleteLevelLimit() { return m_iPlayerDeleteLevelLimit; }

	void	SetChinaEventServer(bool flag) { m_bChinaEventServer = flag; }
	bool	IsChinaEventServer() { return m_bChinaEventServer; }

	DWORD	GetUserCount();

	void	SendAllGuildSkillRechargePacket();
	void	SendTime();

	CPlayerTableCache *	GetPlayerCache(DWORD id);
	void			PutPlayerCache(TPlayerTable * pNew);

	void			CreateItemCacheSet(DWORD dwID);
	TItemCacheSet *		GetItemCacheSet(DWORD dwID);
	void			FlushItemCacheSet(DWORD dwID);

	CItemCache *		GetItemCache(DWORD id);
	void			PutItemCache(TPlayerItem * pNew, bool bSkipQuery = false);
	bool			DeleteItemCache(DWORD id);

	void			UpdatePlayerCache();
	void			UpdateItemCache();

	// MYSHOP_PRICE_LIST

	CItemPriceListTableCache*	GetItemPriceListCache(DWORD dwID);

	void			PutItemPriceListCache(const TItemPriceListTable* pItemPriceList);

	void			UpdateItemPriceListCache(void);
	// END_OF_MYSHOP_PRICE_LIST

	void			SendGuildSkillUsable(DWORD guild_id, DWORD dwSkillVnum, bool bUsable);

	void			SetCacheFlushCountLimit(int iLimit);

	template <class Func>
	Func		for_each_peer(Func f);

	CPeer *		GetAnyPeer();

	void			ForwardPacket(BYTE header, const void* data, int size, BYTE bChannel = 0, CPeer * except = NULL);

	void			SendNotice(const char * c_pszFormat, ...);

	// @fixme203 directly GetCommand instead of strcpy
	char*			GetCommand(char* str, char* command);
	void			ItemAward(CPeer * peer, char* login);

    protected:
	void	Destroy();

    private:
	bool		InitializeTables();
	bool		InitializeShopTable();
	bool		InitializeMobTable();
	bool		InitializeItemTable();
	bool		InitializeQuestItemTable();
	bool		InitializeSkillTable();
	bool		InitializeRefineTable();
	bool		InitializeOfflineShopLimitTable();
	bool		InitializeBanwordTable();
	bool		InitializeItemAttrTable();
	bool		InitializeItemRareTable();
	bool		InitializeLandTable();
	bool		InitializeObjectProto();
	bool		InitializeObjectTable();
	bool		InitializeMonarch();
	bool		InitializeItemShop();
	bool		InitializeCraftingTable();
	bool		InitializeQuestRewardTable();
	bool		InitializeNameReservationTable();
	bool		InitializeSpecialShopTable();
	bool		InitializeSpecialShopItemTable();

	bool		MirrorMobTableIntoDB();
	bool		MirrorItemTableIntoDB();

	void		AddPeer(socket_t fd);
	void		RemovePeer(CPeer * pPeer);
	CPeer *		GetPeer(IDENT ident);

	int		AnalyzeQueryResult(SQLMsg * msg);
	int		AnalyzeErrorMsg(CPeer * peer, SQLMsg * msg);

	int		Process();

        void            ProcessPackets(CPeer * peer);

	CLoginData *	GetLoginData(DWORD dwKey);
	CLoginData *	GetLoginDataByLogin(const char * c_pszLogin);
	CLoginData *	GetLoginDataByAID(DWORD dwAID);

	void		InsertLoginData(CLoginData * pkLD);
	void		DeleteLoginData(CLoginData * pkLD);

	bool		InsertLogonAccount(const char * c_pszLogin, DWORD dwHandle, const char * c_pszIP);
	bool		DeleteLogonAccount(const char * c_pszLogin, DWORD dwHandle);
	bool		FindLogonAccount(const char * c_pszLogin);

	void		GuildCreate(CPeer * peer, DWORD dwGuildID);
	void		GuildSkillUpdate(CPeer * peer, TPacketGuildSkillUpdate* p);
	void		GuildExpUpdate(CPeer * peer, TPacketGuildExpUpdate* p);
	void		GuildAddMember(CPeer * peer, TPacketGDGuildAddMember* p);
	void		GuildChangeGrade(CPeer * peer, TPacketGuild* p);
	void		GuildRemoveMember(CPeer * peer, TPacketGuild* p);
	void		GuildChangeMemberData(CPeer * peer, TPacketGuildChangeMemberData* p);
	void		GuildDisband(CPeer * peer, TPacketGuild * p);
	void		GuildWar(CPeer * peer, TPacketGuildWar * p);
	void		GuildWarScore(CPeer * peer, TPacketGuildWarScore * p);
	void		GuildChangeLadderPoint(TPacketGuildLadderPoint* p);
	void		GuildUseSkill(TPacketGuildUseSkill* p);
	void		GuildDepositMoney(TPacketGDGuildMoney* p);
	void		GuildWithdrawMoney(CPeer* peer, TPacketGDGuildMoney* p);
	void		GuildWithdrawMoneyGiveReply(TPacketGDGuildMoneyWithdrawGiveReply* p);
	void		GuildWarBet(TPacketGDGuildWarBet * p);
	void		GuildChangeMaster(TPacketChangeGuildMaster* p);

	void		SetGuildWarEndTime(DWORD guild_id1, DWORD guild_id2, time_t tEndTime);

	void		QUERY_BOOT(CPeer * peer, TPacketGDBoot * p);

	void		QUERY_LOGIN(CPeer * peer, DWORD dwHandle, SLoginPacket* data);
	void		QUERY_LOGOUT(CPeer * peer, DWORD dwHandle, const char *);

	void		RESULT_LOGIN(CPeer * peer, SQLMsg *msg);

	void		QUERY_PLAYER_LOAD(CPeer * peer, DWORD dwHandle, TPlayerLoadPacket*, bool bIsBot = false);
	void		RESULT_COMPOSITE_PLAYER(CPeer * peer, SQLMsg * pMsg, DWORD dwQID);
	void		RESULT_PLAYER_LOAD(CPeer * peer, MYSQL_RES * pRes, ClientHandleInfo * pkInfo);
	void		RESULT_ITEM_LOAD(CPeer * peer, MYSQL_RES * pRes, DWORD dwHandle, DWORD dwPID);
	void		RESULT_QUEST_LOAD(CPeer * pkPeer, MYSQL_RES * pRes, DWORD dwHandle, DWORD dwPID);
	// @fixme402 (RESULT_AFFECT_LOAD +dwRealPID)
	void		RESULT_AFFECT_LOAD(CPeer * pkPeer, MYSQL_RES * pRes, DWORD dwHandle, DWORD dwRealPID);
	void		RESULT_SPECIAL_FLAG_LOAD(CPeer* pkPeer, MYSQL_RES* pRes, DWORD dwHandle, DWORD dwPID);

	// PLAYER_INDEX_CREATE_BUG_FIX
	void		RESULT_PLAYER_INDEX_CREATE(CPeer *pkPeer, SQLMsg *msg);
	// END_PLAYER_INDEX_CREATE_BUG_FIX

	// MYSHOP_PRICE_LIST

	void		RESULT_PRICELIST_LOAD(CPeer* peer, SQLMsg* pMsg);

	void		RESULT_PRICELIST_LOAD_FOR_UPDATE(SQLMsg* pMsg);
	// END_OF_MYSHOP_PRICE_LIST

	void		QUERY_PLAYER_SAVE(CPeer * peer, DWORD dwHandle, TPlayerTable*);

	void		__QUERY_PLAYER_CREATE(CPeer * peer, DWORD dwHandle, TPlayerCreatePacket *);
	void		__QUERY_PLAYER_DELETE(CPeer * peer, DWORD dwHandle, TPlayerDeletePacket *);
	void		__RESULT_PLAYER_DELETE(CPeer * peer, SQLMsg* msg);

	void		QUERY_PLAYER_COUNT(CPeer * pkPeer, TPlayerCountPacket *);

	void		QUERY_ITEM_SAVE(CPeer * pkPeer, const char * c_pData);
	void		QUERY_ITEM_DESTROY(CPeer * pkPeer, const char * c_pData);
	void		QUERY_ITEM_FLUSH(CPeer * pkPeer, const char * c_pData);

	void		QUERY_QUEST_SAVE(CPeer * pkPeer, TQuestTable *, DWORD dwLen);
	void		QUERY_SPECIAL_FLAG_SAVE(CPeer* pkPeer, TSpecialFlagTable* pTable, DWORD dwLen);
	void		QUERY_ADD_AFFECT(CPeer * pkPeer, TPacketGDAddAffect * p);
	void		QUERY_REMOVE_AFFECT(CPeer * pkPeer, TPacketGDRemoveAffect * p);

	void		QUERY_SAFEBOX_LOAD(CPeer * pkPeer, DWORD dwHandle, TSafeboxLoadPacket *, bool bMall);
	void		QUERY_SAFEBOX_SAVE(CPeer * pkPeer, TSafeboxTable * pTable);
	void		QUERY_SAFEBOX_CHANGE_SIZE(CPeer * pkPeer, DWORD dwHandle, TSafeboxChangeSizePacket * p);
	void		QUERY_SAFEBOX_CHANGE_PASSWORD(CPeer * pkPeer, DWORD dwHandle, TSafeboxChangePasswordPacket * p);

	void		RESULT_SAFEBOX_LOAD(CPeer * pkPeer, SQLMsg * msg);
	void		RESULT_SAFEBOX_CHANGE_SIZE(CPeer * pkPeer, SQLMsg * msg);
	void		RESULT_SAFEBOX_CHANGE_PASSWORD(CPeer * pkPeer, SQLMsg * msg);
	void		RESULT_SAFEBOX_CHANGE_PASSWORD_SECOND(CPeer * pkPeer, SQLMsg * msg);

	void		QUERY_MOVE_CHANNEL(CPeer* pkPeer, DWORD dwHandle, TMoveChannel*);

	void		QUERY_EMPIRE_SELECT(CPeer * pkPeer, DWORD dwHandle, TEmpireSelectPacket * p);
	void		QUERY_SETUP(CPeer * pkPeer, DWORD dwHandle, const char * c_pData);

	void		SendPartyOnSetup(CPeer * peer);

	void		QUERY_FLUSH_CACHE(CPeer * pkPeer, const char * c_pData);

	void		QUERY_PARTY_CREATE(CPeer * peer, TPacketPartyCreate* p);
	void		QUERY_PARTY_DELETE(CPeer * peer, TPacketPartyDelete* p);
	void		QUERY_PARTY_ADD(CPeer * peer, TPacketPartyAdd* p);
	void		QUERY_PARTY_REMOVE(CPeer * peer, TPacketPartyRemove* p);
	void		QUERY_PARTY_STATE_CHANGE(CPeer * peer, TPacketPartyStateChange* p);
	void		QUERY_PARTY_SET_MEMBER_LEVEL(CPeer * peer, TPacketPartySetMemberLevel* p);

	void		QUERY_RELOAD_PROTO();

	void		QUERY_CHANGE_NAME(CPeer * peer, DWORD dwHandle, TPacketGDChangeName * p);
	void		GetPlayerFromRes(TPlayerTable * player_table, MYSQL_RES* res);

	void		QUERY_LOGIN_KEY(CPeer * pkPeer, TPacketGDLoginKey * p);

	void		AddGuildPriv(TPacketGiveGuildPriv* p);
	void		AddEmpirePriv(TPacketGiveEmpirePriv* p);
	void		AddCharacterPriv(TPacketGiveCharacterPriv* p);

	void		MoneyLog(TPacketMoneyLog* p);

	void		QUERY_AUTH_LOGIN(CPeer * pkPeer, DWORD dwHandle, TPacketGDAuthLogin * p);

	void		QUERY_LOGIN_BY_KEY(CPeer * pkPeer, DWORD dwHandle, TPacketGDLoginByKey * p);
	void		RESULT_LOGIN_BY_KEY(CPeer * peer, SQLMsg * msg);

	void		ChargeCash(const TRequestChargeCash * p);
	void		UseCodeVoucher(const TRequestUseCodeVoucher * p, CPeer* peer);

	void		LoadEventFlag();
	void		SetEventFlag(TPacketSetEventFlag* p);
	void		SendEventFlagsOnSetup(CPeer* peer);

	void		MarriageAdd(TPacketMarriageAdd * p);
	void		MarriageUpdate(TPacketMarriageUpdate * p);
	void		MarriageRemove(TPacketMarriageRemove * p);

	void		WeddingRequest(TPacketWeddingRequest * p);
	void		WeddingReady(TPacketWeddingReady * p);
	void		WeddingEnd(TPacketWeddingEnd * p);

	// MYSHOP_PRICE_LIST

	void		MyshopPricelistUpdate(const TItemPriceListTable* pPacket); // @fixme403 (TPacketMyshopPricelistHeader to TItemPriceListTable)

	void		MyshopPricelistRequest(CPeer* peer, DWORD dwHandle, DWORD dwPlayerID);
	// END_OF_MYSHOP_PRICE_LIST

	// Building
	void		CreateObject(TPacketGDCreateObject * p);
	void		DeleteObject(DWORD dwID);
	void		UpdateLand(DWORD * pdw);

	// BLOCK_CHAT
	void		BlockChat(TPacketBlockChat * p);
	// END_OF_BLOCK_CHAT

    private:
	int					m_looping;
	socket_t				m_fdAccept;
	TPeerList				m_peerList;

	CPeer *					m_pkAuthPeer;

	// LoginKey, LoginData pair
	typedef boost::unordered_map<DWORD, CLoginData *> TLoginDataByLoginKey;
	TLoginDataByLoginKey			m_map_pkLoginData;

	// Login LoginData pair
	typedef boost::unordered_map<std::string, CLoginData *> TLoginDataByLogin;
	TLoginDataByLogin			m_map_pkLoginDataByLogin;

	// AccountID LoginData pair
	typedef boost::unordered_map<DWORD, CLoginData *> TLoginDataByAID;
	TLoginDataByAID				m_map_pkLoginDataByAID;

	typedef boost::unordered_map<std::string, CLoginData *> TLogonAccountMap;
	TLogonAccountMap			m_map_kLogonAccount;

	int					m_iPlayerIDStart;
	int					m_iPlayerDeleteLevelLimit;
	int					m_iPlayerDeleteLevelLimitLower;
	bool					m_bChinaEventServer;

	std::vector<TMobTable>			m_vec_mobTable;
	std::vector<TItemTable>			m_vec_itemTable;
	std::map<DWORD, TItemTable *>		m_map_itemTableByVnum;

	int					m_iShopTableSize;
	TShopTable *				m_pShopTable;

	int					m_iRefineTableSize;
	TRefineTable*				m_pRefineTable;

	std::vector<TSkillTable>		m_vec_skillTable;
	std::vector<TBanwordTable>		m_vec_banwordTable;
	std::vector<TNameReservationTable>		m_vec_nameReservationTable;
	std::vector<TItemAttrTable>		m_vec_itemAttrTable;
	std::vector<TItemAttrTable>		m_vec_itemRareTable;

	std::vector<TOfflineShopLimit>		m_vec_offlineShopLimitTable;

	std::vector<building::TLand>		m_vec_kLandTable;
	std::vector<building::TObjectProto>	m_vec_kObjectProto;
	std::map<DWORD, building::TObject *>	m_map_pkObjectTable;

	std::vector<TItemShopItem>		m_vec_itemShopItemTable;
	std::vector<TCraftingItem>		m_vec_CraftingTable;
	std::vector<TQuestRewardItem>		m_vec_QuestRewardTable;

	int					m_iSpecialShopTableSize;
	TSpecialShopTable *		m_pSpecialShopTable;
	std::vector<TSpecialShopItem>		m_vec_SpecialShopItemTable;

	bool					m_bShutdowned;
	bool					m_bMaintenance;

	std::map<DWORD, TSpecialShopMetaData>	m_map_SpecialShopMetaData;

	TPlayerTableCacheMap			m_map_playerCache;

	TItemCacheMap				m_map_itemCache;
	TItemCacheSetPtrMap			m_map_pkItemCacheSetPtr;

	// MYSHOP_PRICE_LIST

	TItemPriceListCacheMap m_mapItemPriceListCache;
	// END_OF_MYSHOP_PRICE_LIST

	TChannelStatusMap m_mChannelStatus;

	struct TPartyInfo
	{
	    BYTE bRole;
	    BYTE bLevel;

		TPartyInfo() :bRole(0), bLevel(0)
		{
		}
	};

	typedef std::map<DWORD, TPartyInfo>	TPartyMember;
	typedef std::map<DWORD, TPartyMember>	TPartyMap;
	typedef std::map<BYTE, TPartyMap>	TPartyChannelMap;
	TPartyChannelMap m_map_pkChannelParty;

	typedef std::map<std::string, long>	TEventFlagMap;
	TEventFlagMap m_map_lEventFlag;

	BYTE					m_bLastHeader;
	int					m_iCacheFlushCount;
	int					m_iCacheFlushCountLimit;

    private :
	TItemIDRangeTable m_itemRange;

    public :
	bool InitializeNowItemID();
	DWORD GetItemID();
	DWORD GainItemID();
	TItemIDRangeTable GetItemRange() { return m_itemRange; }
#ifdef ENABLE_IKASHOP_RENEWAL
public:
	// forwarding
	void SendIkarusShopTable(CPeer* peer);
	//booting
	bool InitializeIkarusShopTable(DWORD owner=0);
	// shops
	bool InitializeIkarusShopShopTable(DWORD owner=0);
	bool InitializeIkarusShopShopItemsTable(DWORD owner=0);
	// offers
	bool InitializeIkarusShopOfferTable();
	// auctions
	bool InitializeIkarusShopAuctionTable();
	bool InitializeIkarusShopAuctionOfferTable();

	//packets exchanging
	void RecvIkarusShopPacket(CPeer* peer, const char* data);
	bool RecvIkarusShopBuyItemPacket(CPeer* peer, const char* data);
	bool RecvIkarusShopLockBuyItem(CPeer* peer, const char* data);
	bool RecvIkarusShopEditItemPacket(const char* data);
	bool RecvIkarusShopRemoveItemPacket(CPeer* peer, const char* data);
	bool RecvIkarusShopAddItemPacket(const char* data);

	bool RecvIkarusShopForceSoftClose(CPeer* peer, const char* data);
	bool RecvIkarusShopForceHardClose(CPeer* peer, const char* data);
	bool RecvIkarusShopCreateNew(const char* data);
	bool RecvIkarusShopChangeName(const char* data);

	bool RecvIkarusShopOfferCreate(const char* data);
	bool RecvIkarusShopOfferNotified(const char* data);
	bool RecvIkarusShopOfferAccepted(const char* data);
	bool RecvIkarusShopOfferCancel(const char* data);

	bool RecvIkarusShopSafeboxGetItem(CPeer* peer, const char* data);
	bool RecvIkarusShopSafeboxGetValutes(const char* data);
	bool RecvIkarusShopSafeboxAddItem(const char* data);
	bool RecvIkarusShopSafeboxGetItemFeedback(const char* data);

	//AUCTION
	bool RecvIkarusShopAuctionCreate(const char* data);
	bool RecvIkarusShopAuctionAddOffer(const char* data);
	bool RecvIkarusShopAuctionCancel(const char* data);
	bool RecvIkarusShopAuctionOfferSeen(const char* data);

#ifdef EXTEND_IKASHOP_PRO
	bool RecvIkarusShopNotificationSeen(const char* data);
	bool RecvIkarusShopRestoreDuration(const char* data);
#ifdef ENABLE_IKASHOP_ENTITIES
	bool RecvIkarusShopMoveShopEntity(const char* data);
#endif
#endif
	bool RecvIkarusShopLockedSlotState(const char* data);
	bool RecvIkarusShopOwnerOnline(const char* data);
	bool RecvIkarusShopLoadExpiredShop(const char* data);

	bool SendIkarusShopBuyItemPacket(DWORD dwOwner, DWORD dwGuest, DWORD dwItem, CPeer* requester);
	bool SendIkarusShopBuyLockedItemPacket(CPeer* peer,DWORD dwOwner, DWORD dwGuest, DWORD dwItem);
	bool SendIkarusShopEditItemPacket(DWORD dwOwner, DWORD dwItem, const ikashop::TPriceInfo& price);
	bool SendIkarusShopRemoveItemPacket(DWORD dwOwner, DWORD dwItem, CPeer* requester = nullptr);
	bool SendIkarusShopAddItemPacket(DWORD dwOwner, DWORD dwItemID, const ikashop::TShopItem& rInfo);

	bool SendIkarusShopForceClose(DWORD dwOwnerID);
	bool SendIkarusShopCreateNew(const ikashop::TShopInfo& shop, const ikashop::TShopItem* items);
	bool SendIkarusShopChangeName(DWORD dwOwnerID, const char* szName);
	bool SendIkarusShopShopExpired(DWORD dwOwnerID);

	bool SendIkarusShopOfferCreate(const ikashop::TOfferInfo& offer);
	bool SendIkarusShopOfferNotified(DWORD dwOwnerID, DWORD dwOfferID);
	bool SendIkarusShopOfferAccepted(DWORD dwOwnerID, DWORD dwOfferID);

	//ikashop-updated 05/08/19
	void SendIkarusShopOfferCancel(DWORD dwOwnerID, DWORD dwOfferID, bool isRemovingItem);

	void SendIkarusShopSafeboxAddItem(DWORD dwOwnerID, DWORD dwItem, const ikashop::TShopPlayerItem& item);
	void SendIkarusShopSafeboxAddValutes(DWORD dwOwnerID, const ikashop::TValutesInfo& valute);

	//patch 08-03-2020
	void SendIkarusShopSafeboxExpiredItem(DWORD dwOwnerID, DWORD itemID);
	void SendIkarusShopSafeboxGetItemConfirm(CPeer* peer, DWORD dwOwnerID, DWORD itemID);

	//AUCTION
	void SendIkarusShopAuctionCreate(const ikashop::TAuctionInfo& auction);
	void SendIkarusShopAuctionAddOffer(const ikashop::TAuctionOfferInfo& offer);
	void SendIkarusShopAuctionExpired(DWORD dwOwnerID);

	void IkarusShopResultQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo);
	void IkarusShopResultAddItemQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo, bool sendResponse=true);
	void IkarusShopResultCreateShopQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo);
	void IkarusShopResultOfferAddQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo);
	void IkarusShopResultSafeboxAddItemQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo);


	void IkarusShopDurationProcess();
	void IkarusShopExpiredShop(DWORD dwID
	);
	void IkarusShopExpiredAuction(DWORD dwID);
	void IkarusShopLoadShopSafebox(CPeer* peer ,DWORD dwID);
#ifdef EXTEND_IKASHOP_PRO
	void IkarusShopLoadNotifications(CPeer* peer, DWORD owner);
	void AppendIkarusShopNotification(ikashop::ENotificationType type, DWORD owner, DWORD what, const std::string& who, const std::string& format);
#endif
	//patch 08-03-2020
	void IkarusShopExpiredAuctionItem(DWORD dwOwnerID);

	//updated 15-01-2020
	bool IsUsingIkarusShopSystem(DWORD dwID);

	//patch offerlist loading fix
	void RemoveOfferOnShopItem(DWORD dwOwnerID, DWORD dwItemID);
	void RemoveOfferOnShop(DWORD dwOwnerID);

#ifdef ENABLE_IKASHOP_LOGS
	void IkarusShopLog(DWORD who, DWORD itemid, std::string_view what, DWORD shopOwner, const std::string& extra = "", DWORD vnum = 0, DWORD count = 0, int64_t yang = 0, int cheque = 0);
#endif

private:
	ikashop::CShopCache			m_offlineshopShopCache;
	ikashop::CSafeboxCache		m_offlineshopSafeboxCache;
	ikashop::COfferCache		m_offlineshopOfferCache;
	ikashop::CAuctionCache		m_offlineshopAuctionCache;
#endif

	//BOOT_LOCALIZATION
    public:

	bool InitializeLocalization();

    private:
	std::vector<tLocale> m_vec_Locale;
	//END_BOOT_LOCALIZATION
	//ADMIN_MANAGER

	bool __GetAdminInfo(const char *szIP, std::vector<tAdminInfo> & rAdminVec);
	bool __GetHostInfo(std::vector<std::string> & rIPVec);
	//END_ADMIN_MANAGER

	//RELOAD_ADMIN
	void ReloadAdmin(CPeer * peer, TPacketReloadAdmin * p);
	//END_RELOAD_ADMIN
	void ReloadItemShop();
	void BreakMarriage(CPeer * peer, const char * data);

	struct TLogoutPlayer
	{
	    DWORD	pid;
	    time_t	time;

	    bool operator < (const TLogoutPlayer & r)
	    {
		return (pid < r.pid);
	    }
	};

	typedef boost::unordered_map<DWORD, TLogoutPlayer*> TLogoutPlayerMap;
	TLogoutPlayerMap m_map_logout;

	void InsertLogoutPlayer(DWORD pid);
	void DeleteLogoutPlayer(DWORD pid);
	void UpdateLogoutPlayer();
	void UpdateItemCacheSet(DWORD pid);

	void FlushPlayerCacheSet(DWORD pid);

	//MONARCH
	void Election(CPeer * peer, DWORD dwHandle, const char * p);
	void Candidacy(CPeer * peer, DWORD dwHandle, const char * p);
	void AddMonarchMoney(CPeer * peer, DWORD dwHandle, const char * p);
	void TakeMonarchMoney(CPeer * peer, DWORD dwHandle, const char * p);
	void ComeToVote(CPeer * peer, DWORD dwHandle, const char * p);
	void RMCandidacy(CPeer * peer, DWORD dwHandle, const char * p);
	void SetMonarch(CPeer * peer, DWORD dwHandle, const char * p);
	void RMMonarch(CPeer * peer, DWORD dwHandle, const char * p);

	void DecMonarchMoney(CPeer * peer, DWORD dwHandle, const char * p);
	//END_MONARCH

	void ChangeMonarchLord(CPeer* peer, DWORD dwHandle, TPacketChangeMonarchLord* info);

	void SendSpareItemIDRange(CPeer* peer);

	void UpdateHorseName(TPacketUpdateHorseName* data, CPeer* peer);
	void AckHorseName(DWORD dwPID, CPeer* peer);
	void SetLoginPremiumPass(CPeer* peer, TPacketSetPremiumPass* data);
	void DeleteLoginKey(TPacketDC *data);
	void ResetLastPlayerID(const TPacketNeedLoginLogInfo* data);
	//delete gift notify icon
	void DeleteAwardId(TPacketDeleteAwardID* data);
	void UpdateChannelStatus(TChannelStatus* pData);
	void RequestChannelStatus(CPeer* peer, DWORD dwHandle);
	void Maintenance(TPacketGDMaintenance* data);

	TSpecialShopMetaData GetSpecialShopMetaCache(DWORD shop_vnum);
	void RecvSpecialShopSeed(CPeer* peer, TPacketGDSpecialShopSeed* data);
	void RecvUsage(CPeer* peer, TPacketGDUsage* data);
#ifdef ENABLE_PROTO_FROM_DB
	public:
	bool		InitializeMobTableFromDB();
	bool		InitializeItemTableFromDB();
	protected:
	bool		bIsProtoReadFromDB;
#endif
};

template<class Func>
Func CClientManager::for_each_peer(Func f)
{
    TPeerList::iterator it;
    for (it = m_peerList.begin(); it!=m_peerList.end();++it)
    {
	f(*it);
    }
    return f;
}
#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
