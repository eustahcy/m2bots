#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "utils.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#ifdef ENABLE_IKASHOP_RENEWAL
#include "ikarus_shop.h"
#include "ikarus_shop_manager.h"
#endif
#include "item_manager.h"
#include "packet.h"
#include "protocol.h"
#include "mob_manager.h"
#include "shop_manager.h"
#include "sectree_manager.h"
#include "skill.h"
#include "questmanager.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "start_position.h"
#include "party.h"
#include "refine.h"
#include "banword.h"
#include "priv_manager.h"
#include "db.h"
#include "building.h"
#include "login_sim.h"
#include "wedding.h"
#include "login_data.h"
#include "unique_item.h"
#include "playerbot_manager.h"
#include "playerbot_empire_rules.h"

#include <cstdlib>

#include "monarch.h"
#include "affect.h"
#include "castle.h"
#include "motion.h"

#include "log.h"

#include "horsename_manager.h"
#include "gm.h"
#include "panama.h"
#include "map_location.h"
#include "DragonSoul.h"
#include "questmanager.h"

#include "itemshop_manager.h"
#include "shutdown_manager.h"
#include "crafting_manager.h"
#include "name_reservation.h"
#include "SpecialSpawnManager.h"
#include "special_shop_manager.h"
#include "../../common/CommonDefines.h"

#define MAPNAME_DEFAULT	"none"

bool GetServerLocation(TAccountTable & rTab, BYTE bEmpire)
{
	bool bFound = false;

	for (int i = 0; i < PLAYER_PER_ACCOUNT; ++i)
	{
		if (0 == rTab.players[i].dwID)
			continue;

		bFound = true;
		long lIndex = 0;

		#ifdef ENABLE_MOVE_CHANNEL
		if (!CMapLocation::instance().Get(rTab.players[i].x, rTab.players[i].y, lIndex, rTab.players[i].lAddr, rTab.players[i].wPort, g_bChannel))
		#else
		if (!CMapLocation::instance().Get(rTab.players[i].x, rTab.players[i].y, lIndex, rTab.players[i].lAddr, rTab.players[i].wPort))
		#endif
		{
			sys_err("location error name %s mapindex %d %d x %d empire %d",
					rTab.players[i].szName, lIndex, rTab.players[i].x, rTab.players[i].y, rTab.bEmpire);

			rTab.players[i].x = EMPIRE_START_X(rTab.bEmpire);
			rTab.players[i].y = EMPIRE_START_Y(rTab.bEmpire);

			lIndex = 0;

			#ifdef ENABLE_MOVE_CHANNEL
			if (!CMapLocation::instance().Get(rTab.players[i].x, rTab.players[i].y, lIndex, rTab.players[i].lAddr, rTab.players[i].wPort, g_bChannel))
			#else
			if (!CMapLocation::instance().Get(rTab.players[i].x, rTab.players[i].y, lIndex, rTab.players[i].lAddr, rTab.players[i].wPort))
			#endif
			{
				sys_err("cannot find server for mapindex %d %d x %d (name %s)",
						lIndex,
						rTab.players[i].x,
						rTab.players[i].y,
						rTab.players[i].szName);
#ifdef ENABLE_NEWSTUFF
				if (!g_stProxyIP.empty())
					rTab.players[i].lAddr=inet_addr(g_stProxyIP.c_str());
#endif
				continue;
			}
		}
#ifdef ENABLE_NEWSTUFF
		if (!g_stProxyIP.empty())
			rTab.players[i].lAddr=inet_addr(g_stProxyIP.c_str());
#endif
		struct in_addr in;
		in.s_addr = rTab.players[i].lAddr;
		sys_log(0, "success to %s:%d", inet_ntoa(in), rTab.players[i].wPort);
	}

	return bFound;
}

extern std::map<DWORD, CLoginSim *> g_sim;
extern std::map<DWORD, CLoginSim *> g_simByPID;

void CInputDB::LoginSuccess(DWORD dwHandle, const char *data)
{
	sys_log(0, "LoginSuccess");

	TAccountTable * pTab = (TAccountTable *) data;

	itertype(g_sim) it = g_sim.find(pTab->id);
	if (g_sim.end() != it)
	{
		sys_log(0, "CInputDB::LoginSuccess - already exist sim [%s]", pTab->login);
		it->second->SendLoad();
		return;
	}

	LPDESC d = DESC_MANAGER::instance().FindByHandle(dwHandle);

	if (!d)
	{
		sys_log(0, "CInputDB::LoginSuccess - cannot find handle [%s]", pTab->login);

		TLogoutPacket pack;

		strlcpy(pack.login, pTab->login, sizeof(pack.login));
		db_clientdesc->DBPacket(HEADER_GD_LOGOUT, dwHandle, &pack, sizeof(pack));
		return;
	}

	if (strcmp(pTab->status, "OK"))
	{
		sys_log(0, "CInputDB::LoginSuccess - status[%s] is not OK [%s]", pTab->status, pTab->login);

		TLogoutPacket pack;

		strlcpy(pack.login, pTab->login, sizeof(pack.login));
		db_clientdesc->DBPacket(HEADER_GD_LOGOUT, dwHandle, &pack, sizeof(pack));

		LoginFailure(d, pTab->status);
		return;
	}

	if (g_bChannel != 99 && g_bChannel != pTab->bDestChannel)
	{ 
		sys_log(0, "CInputDB::LoginSuccess - channel switch hack? (current channel %d, user dest channel %d)", g_bChannel, pTab->bDestChannel);
		TLogoutPacket pack;

		strlcpy(pack.login, pTab->login, sizeof(pack.login));
		db_clientdesc->DBPacket(HEADER_GD_LOGOUT, dwHandle, &pack, sizeof(pack));

		LoginFailure(d, "WRONGKEY");
		return;
	}

	if (g_bPremiumChannel && (get_global_time() > pTab->iPremium && get_global_time() > pTab->iMarriagePremiumChannelPass))
	{
		sys_log(0, "CInputDB::LoginSuccess - Premium is not active (%d, marriagePremiumChannelPass: %d, current_time: %d).", pTab->iPremium, pTab->iMarriagePremiumChannelPass, get_global_time());
		TLogoutPacket pack;

		strlcpy(pack.login, pTab->login, sizeof(pack.login));
		db_clientdesc->DBPacket(HEADER_GD_LOGOUT, dwHandle, &pack, sizeof(pack));

		LoginFailure(d, "NOPREMIUM");
		return;
	}


	if (g_bIsMaintenance && !gm_is_login_implementor(pTab->login))
	{
		TLogoutPacket pack;
		strlcpy(pack.login, pTab->login, sizeof(pack.login));
		db_clientdesc->DBPacket(HEADER_GD_LOGOUT, dwHandle, &pack, sizeof(pack));
		LoginFailure(d, "MAINTENA");
		return;
	}

	for (int i = 0; i != PLAYER_PER_ACCOUNT; ++i)
	{
		TSimplePlayer& player = pTab->players[i];
		sys_log(0, "\tplayer(%s).job(%d)", player.szName, player.byJob);
	}

	bool bFound = GetServerLocation(*pTab, pTab->bEmpire);

	d->BindAccountTable(pTab);

	if (!bFound)
	{
		TPacketGCEmpire pe;
		pe.bHeader = HEADER_GC_EMPIRE;
		pe.bEmpire = number(1, 3);
		d->Packet(&pe, sizeof(pe));
	}
	else
	{
		TPacketGCEmpire pe;
		pe.bHeader = HEADER_GC_EMPIRE;
		pe.bEmpire = d->GetEmpire();
		d->Packet(&pe, sizeof(pe));
	}

	d->SetPhase(PHASE_SELECT);
	d->SendLoginSuccessPacket();

	// __SHUTDOWN::Shutdown Register
	CShutdownManager::Instance().AddDesc(d);

	sys_log(0, "InputDB::login_success: %s", pTab->login);
}

void CInputDB::PlayerCreateFailure(LPDESC d, BYTE bType)
{
	if (!d)
		return;

	TPacketGCCreateFailure pack;

	pack.header	= HEADER_GC_CHARACTER_CREATE_FAILURE;
	pack.bType	= bType;

	d->Packet(&pack, sizeof(pack));
}

void CInputDB::PlayerCreateSuccess(LPDESC d, const char * data)
{
	if (!d)
		return;

	TPacketDGCreateSuccess * pPacketDB = (TPacketDGCreateSuccess *) data;

	if (pPacketDB->bAccountCharacterIndex >= PLAYER_PER_ACCOUNT)
	{
		d->Packet(encode_byte(HEADER_GC_CHARACTER_CREATE_FAILURE), 1);
		return;
	}

	long lIndex = 0;

	#ifdef ENABLE_MOVE_CHANNEL
	if (!CMapLocation::instance().Get(pPacketDB->player.x, pPacketDB->player.y, lIndex, pPacketDB->player.lAddr, pPacketDB->player.wPort, g_bChannel))
	#else
	if (!CMapLocation::instance().Get(pPacketDB->player.x, pPacketDB->player.y, lIndex, pPacketDB->player.lAddr, pPacketDB->player.wPort))
	#endif
	{
		sys_err("InputDB::PlayerCreateSuccess: cannot find server for mapindex %d %d x %d (name %s)",
				lIndex,
				pPacketDB->player.x,
				pPacketDB->player.y,
				pPacketDB->player.szName);
	}

	TAccountTable & r_Tab = d->GetAccountTable();
	r_Tab.players[pPacketDB->bAccountCharacterIndex] = pPacketDB->player;

	TPacketGCPlayerCreateSuccess pack;

	pack.header = HEADER_GC_CHARACTER_CREATE_SUCCESS;
	pack.bAccountCharacterIndex = pPacketDB->bAccountCharacterIndex;
	pack.player = pPacketDB->player;
#ifdef ENABLE_NEWSTUFF
	if (!g_stProxyIP.empty())
		pack.player.lAddr=inet_addr(g_stProxyIP.c_str());
#endif
	d->Packet(&pack, sizeof(TPacketGCPlayerCreateSuccess));

	TPlayerItem t;
	memset(&t, 0, sizeof(t));

	if (china_event_server)
	{
		t.window	= INVENTORY;
		t.count	= 1;
		t.owner	= r_Tab.players[pPacketDB->bAccountCharacterIndex].dwID;

		struct SInitialItem
		{
			DWORD dwVnum;
			BYTE pos;
		};

		const int MAX_INITIAL_ITEM = 9;

		static SInitialItem initialItems[JOB_MAX_NUM][MAX_INITIAL_ITEM] =
		{
			{ {11243,	2}, {12223,	3}, {15103,	4}, {   93,	1}, {16143,	8}, {17103,	9}, { 3083,	0}, {13193,	11}, {14103, 12}, },
			{ {11443,	0}, {12363,	3}, {15103,	4}, { 1053,	2}, { 2083,	1}, {16083,	7}, {17083,	8}, {13193,	9}, {14103,	10}, },
			{ {11643,	0}, {12503,	2}, {15103,	3}, {   93,	1}, {16123,	4}, {17143,	7}, {13193,	8}, {14103,	9}, {    0,	0}, },
			{ {11843,	0}, {12643,	1}, {15103,	2}, { 7083,	3}, { 5053,	4}, {16123,	6}, {17143,	7}, {13193,	8}, {14103,	9}, },
#ifdef ENABLE_WOLFMAN_CHARACTER
			{ {21023,	2}, {12223,	3}, {21513,	4}, { 6023,	1}, {16143,	8}, {17103,	9}, { 0,	0}, {13193,	11}, {14103, 12}, },
#endif
		};

		int job = pPacketDB->player.byJob;
		for (int i=0; i < MAX_INITIAL_ITEM; i++)
		{
			if (initialItems[job][i].dwVnum == 0)
				continue;

			t.id	= ITEM_MANAGER::instance().GetNewID();
			t.pos	= initialItems[job][i].pos;
			t.vnum	= initialItems[job][i].dwVnum;

			db_clientdesc->DBPacketHeader(HEADER_GD_ITEM_SAVE, 0, sizeof(TPlayerItem));
			db_clientdesc->Packet(&t, sizeof(TPlayerItem));
		}
	}

	LogManager::instance().CharLog(pack.player.dwID, 0, 0, 0, "CREATE PLAYER", "", d->GetHostName());
}

void CInputDB::PlayerDeleteSuccess(LPDESC d, const char * data)
{
	if (!d)
		return;

	BYTE account_index;
	account_index = decode_byte(data);
	d->BufferedPacket(encode_byte(HEADER_GC_CHARACTER_DELETE_SUCCESS),	1);
	d->Packet(encode_byte(account_index),			1);

	d->GetAccountTable().players[account_index].dwID = 0;
}

void CInputDB::PlayerDeleteFail(LPDESC d, const char* c_pData)
{
	if (!d)
		return;

	auto bError = decode_byte(c_pData);
	c_pData += sizeof(BYTE);
	auto bAccountIndex = decode_byte(c_pData);
	c_pData += sizeof(BYTE);

	TPacketGCDeleteCharacterError p{};
	p.header = HEADER_GC_CHARACTER_DELETE_WRONG_SOCIAL_ID;
	p.type = bError;

	d->Packet(&p,	sizeof(p));

	//d->Packet(encode_byte(account_index),			1);
	//d->GetAccountTable().players[account_index].dwID = 0;
}

void CInputDB::ChangeName(LPDESC d, const char * data)
{
	if (!d)
		return;

	TPacketDGChangeName * p = (TPacketDGChangeName *) data;

	TAccountTable & r = d->GetAccountTable();

	if (!r.id)
		return;

	for (size_t i = 0; i < PLAYER_PER_ACCOUNT; ++i)
		if (r.players[i].dwID == p->pid)
		{
			strlcpy(r.players[i].szName, p->name, sizeof(r.players[i].szName));
			r.players[i].bChangeName = 0;

			TPacketGCChangeName pgc;

			pgc.header = HEADER_GC_CHANGE_NAME;
			pgc.pid = p->pid;
			strlcpy(pgc.name, p->name, sizeof(pgc.name));

			d->Packet(&pgc, sizeof(TPacketGCChangeName));
			break;
		}
}

#define ENABLE_GOHOME_IF_MAP_NOT_EXIST
void CInputDB::PlayerLoad(LPDESC d, const char * data)
{
	TPlayerTable * pTab = (TPlayerTable *) data;

	if (!d)
		return;

	long lMapIndex = pTab->lMapIndex;
	PIXEL_POSITION pos;

	if (lMapIndex == 0)
	{
		lMapIndex = SECTREE_MANAGER::instance().GetMapIndex(pTab->x, pTab->y);

		if (lMapIndex == 0)
		{
			lMapIndex = EMPIRE_START_MAP(d->GetAccountTable().bEmpire);
			pos.x = EMPIRE_START_X(d->GetAccountTable().bEmpire);
			pos.y = EMPIRE_START_Y(d->GetAccountTable().bEmpire);
		}
		else
		{
			pos.x = pTab->x;
			pos.y = pTab->y;
		}
	}
	pTab->lMapIndex = lMapIndex;

	if (!SECTREE_MANAGER::instance().GetValidLocation(pTab->lMapIndex, pTab->x, pTab->y, lMapIndex, pos, d->GetEmpire()))
	{
		sys_err("InputDB::PlayerLoad : cannot find valid location %d x %d (name: %s)", pTab->x, pTab->y, pTab->name);
#ifdef ENABLE_GOHOME_IF_MAP_NOT_EXIST
		lMapIndex = EMPIRE_START_MAP(d->GetAccountTable().bEmpire);
		pos.x = EMPIRE_START_X(d->GetAccountTable().bEmpire);
		pos.y = EMPIRE_START_Y(d->GetAccountTable().bEmpire);
#else
		d->SetPhase(PHASE_CLOSE);
		return;
#endif
	}

	pTab->x = pos.x;
	pTab->y = pos.y;
	pTab->lMapIndex = lMapIndex;

	if (d->GetCharacter() || d->IsPhase(PHASE_GAME))
	{
		LPCHARACTER p = d->GetCharacter();
		sys_err("login state already has main state (character %s %p)", p->GetName(), get_pointer(p));
		return;
	}

	if (NULL != CHARACTER_MANAGER::Instance().FindPC(pTab->name))
	{
		sys_err("InputDB: PlayerLoad : %s already exist in game", pTab->name);
		return;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().CreateCharacter(pTab->name, pTab->id);

	ch->BindDesc(d);
	ch->SetPlayerProto(pTab);
	ch->SetEmpire(d->GetEmpire());

	d->BindCharacter(ch);

	{
		// P2P Login
		TPacketGGLogin p;

		p.bHeader = HEADER_GG_LOGIN;
		strlcpy(p.szName, ch->GetName(), sizeof(p.szName));
		p.dwPID = ch->GetPlayerID();
		p.bEmpire = ch->GetEmpire();
		p.lMapIndex = SECTREE_MANAGER::instance().GetMapIndex(ch->GetX(), ch->GetY());
		p.bChannel = g_bChannel;
		p.lPremium = get_global_time() + ch->GetPremiumSubscriptionRemainSeconds();

		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGLogin));
		LogManager::instance().LoginLog(true, ch);
	}

	d->SetPhase(PHASE_LOADING);
	ch->MainCharacterPacket();

	long lPublicMapIndex = lMapIndex >= 10000 ? lMapIndex / 10000 : lMapIndex;

	//Send Supplementary Data Block if new map requires security packages in loading this map
	const TMapRegion * rMapRgn = SECTREE_MANAGER::instance().GetMapRegion(lPublicMapIndex);
	if( rMapRgn )
	{
		DESC_MANAGER::instance().SendClientPackageSDBToLoadMap( d, rMapRgn->strMapName.c_str() );
	}
	//if (!map_allow_find(lMapIndex >= 10000 ? lMapIndex / 10000 : lMapIndex) || !CheckEmpire(ch, lMapIndex))
	if (!map_allow_find(lPublicMapIndex))
	{
		sys_err("InputDB::PlayerLoad : entering %d map is not allowed here (name: %s, empire %u)",
				lMapIndex, pTab->name, d->GetEmpire());

		ch->SetWarpLocation(EMPIRE_START_MAP(d->GetEmpire()),
				EMPIRE_START_X(d->GetEmpire()) / 100,
				EMPIRE_START_Y(d->GetEmpire()) / 100);

		d->SetPhase(PHASE_CLOSE);
		return;
	}

	quest::CQuestManager::instance().BroadcastEventFlagOnLogin(ch);

	for (int i = 0; i < QUICKSLOT_MAX_NUM; ++i)
		ch->SetQuickslot(i, pTab->quickslot[i]);

	ch->PointsPacket();
	ch->SkillLevelPacket();

	sys_log(0, "InputDB: player_load %s %dx%dx%d LEVEL %d MOV_SPEED %d JOB %d ATG %d DFG %d GMLv %d",
			pTab->name,
			ch->GetX(), ch->GetY(), ch->GetZ(),
			ch->GetLevel(),
			ch->GetPoint(POINT_MOV_SPEED),
			ch->GetJob(),
			ch->GetPoint(POINT_ATT_GRADE),
			ch->GetPoint(POINT_DEF_GRADE),
			ch->GetGMLevel());

	if (!d->IsBot())
		ch->QuerySafeboxSize();

	if (d->IsBot())
		CPlayerBotManager::instance().OnPlayerLoaded(d);
}

void CInputDB::Boot(const char* data)
{
	signal_timer_disable();

	DWORD dwPacketSize = decode_4bytes(data);
	data += 4;

	BYTE bVersion = decode_byte(data);
	data += 1;

	sys_log(0, "BOOT: PACKET: %d", dwPacketSize);
	sys_log(0, "BOOT: VERSION: %d", bVersion);
	if (bVersion != 6)
	{
		sys_err("boot version error");
		thecore_shutdown();
	}

	sys_log(0, "sizeof(TMobTable) = %d", sizeof(TMobTable));
	sys_log(0, "sizeof(TItemTable) = %d", sizeof(TItemTable));
	sys_log(0, "sizeof(TShopTable) = %d", sizeof(TShopTable));
	sys_log(0, "sizeof(TSkillTable) = %d", sizeof(TSkillTable));
	sys_log(0, "sizeof(TRefineTable) = %d", sizeof(TRefineTable));
	sys_log(0, "sizeof(TItemAttrTable) = %d", sizeof(TItemAttrTable));
	sys_log(0, "sizeof(TItemRareTable) = %d", sizeof(TItemAttrTable));
	sys_log(0, "sizeof(TBanwordTable) = %d", sizeof(TBanwordTable));
	sys_log(0, "sizeof(TLand) = %d", sizeof(building::TLand));
	sys_log(0, "sizeof(TObjectProto) = %d", sizeof(building::TObjectProto));
	sys_log(0, "sizeof(TObject) = %d", sizeof(building::TObject));
	//ADMIN_MANAGER
	sys_log(0, "sizeof(TAdminManager) = %d", sizeof (TAdminInfo) );
	//END_ADMIN_MANAGER

	WORD size;

	/*
	 * MOB
	 */

	if (decode_2bytes(data)!=sizeof(TMobTable))
	{
		sys_err("mob table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;
	sys_log(0, "BOOT: MOB: %d", size);

	if (size)
	{
		CMobManager::instance().Initialize((TMobTable *) data, size);
		data += size * sizeof(TMobTable);
	}

	/*
	 * ITEM
	 */

	if (decode_2bytes(data) != sizeof(TItemTable))
	{
		sys_err("item table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;
	sys_log(0, "BOOT: ITEM: %d", size);

	if (size)
	{
		ITEM_MANAGER::instance().Initialize((TItemTable *) data, size);
		data += size * sizeof(TItemTable);
	}

	/*
	 * SHOP
	 */

	if (decode_2bytes(data) != sizeof(TShopTable))
	{
		sys_err("shop table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;
	sys_log(0, "BOOT: SHOP: %d", size);

	if (size)
	{
		if (!CShopManager::instance().Initialize((TShopTable *) data, size))
		{
			sys_err("shop table Initialize error");
			thecore_shutdown();
			return;
		}
		data += size * sizeof(TShopTable);
	}

	/*
	 * SKILL
	 */

	if (decode_2bytes(data) != sizeof(TSkillTable))
	{
		sys_err("skill table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;
	sys_log(0, "BOOT: SKILL: %d", size);

	if (size)
	{
		if (!CSkillManager::instance().Initialize((TSkillTable *) data, size))
		{
			sys_err("cannot initialize skill table");
			thecore_shutdown();
			return;
		}

		data += size * sizeof(TSkillTable);
	}
	/*
	 * REFINE RECIPE
	 */
	if (decode_2bytes(data) != sizeof(TRefineTable))
	{
		sys_err("refine table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;
	sys_log(0, "BOOT: REFINE: %d", size);

	if (size)
	{
		CRefineManager::instance().Initialize((TRefineTable*) data, size);
		data += size * sizeof(TRefineTable);
	}

	/*
	 * ITEM ATTR
	 */
	if (decode_2bytes(data) != sizeof(TItemAttrTable))
	{
		sys_err("item attr table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;
	sys_log(0, "BOOT: ITEM_ATTR: %d", size);

	if (size)
	{
		TItemAttrTable * p = (TItemAttrTable *) data;

		for (int i = 0; i < size; ++i, ++p)
		{
			if (p->dwApplyIndex >= POINT_MAX_NUM)
				continue;

			g_map_itemAttr[p->dwApplyIndex] = *p;
			sys_log(0, "ITEM_ATTR[%d]: %s %u", p->dwApplyIndex, p->szApply, p->dwProb);
		}
	}

	data += size * sizeof(TItemAttrTable);

	/*
     * ITEM RARE
     */
	if (decode_2bytes(data) != sizeof(TItemAttrTable))
	{
		sys_err("item rare table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;
	sys_log(0, "BOOT: ITEM_RARE: %d", size);

	if (size)
	{
		TItemAttrTable * p = (TItemAttrTable *) data;

		for (int i = 0; i < size; ++i, ++p)
		{
			if (p->dwApplyIndex >= POINT_MAX_NUM)
				continue;

			g_map_itemRare[p->dwApplyIndex] = *p;
			sys_log(0, "ITEM_RARE[%d]: %s %u", p->dwApplyIndex, p->szApply, p->dwProb);
		}
	}

	data += size * sizeof(TItemAttrTable);

	/*
	 * BANWORDS
	 */

	if (decode_2bytes(data) != sizeof(TBanwordTable))
	{
		sys_err("ban word table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;

	CBanwordManager::instance().Initialize((TBanwordTable *) data, size);
	data += size * sizeof(TBanwordTable);

	/*
	 * NAME RESERVATION
	 */

	if (decode_2bytes(data) != sizeof(TNameReservationTable))
	{
		sys_err("name reservation table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;

	CNameReservationManager::instance().Initialize((TNameReservationTable*)data, size);
	data += size * sizeof(TNameReservationTable);

	{
		using namespace building;

		/*
		 * LANDS
		 */

		if (decode_2bytes(data) != sizeof(TLand))
		{
			sys_err("land table size error");
			thecore_shutdown();
			return;
		}
		data += 2;

		size = decode_2bytes(data);
		data += 2;

		TLand * kLand = (TLand *) data;
		data += size * sizeof(TLand);

		for (WORD i = 0; i < size; ++i, ++kLand)
			CManager::instance().LoadLand(kLand);

		/*
		 * OBJECT PROTO
		 */

		if (decode_2bytes(data) != sizeof(TObjectProto))
		{
			sys_err("object proto table size error");
			thecore_shutdown();
			return;
		}
		data += 2;

		size = decode_2bytes(data);
		data += 2;

		CManager::instance().LoadObjectProto((TObjectProto *) data, size);
		data += size * sizeof(TObjectProto);

		/*
		 * OBJECT
		 */
		if (decode_2bytes(data) != sizeof(TObject))
		{
			sys_err("object table size error");
			thecore_shutdown();
			return;
		}
		data += 2;

		size = decode_2bytes(data);
		data += 2;

		TObject * kObj = (TObject *) data;
		data += size * sizeof(TObject);

		for (WORD i = 0; i < size; ++i, ++kObj)
			CManager::instance().LoadObject(kObj, true);

		/*
		 * OFFLINE SHOP LIMIT
		 */
		if (decode_2bytes(data) != sizeof(TOfflineShopLimit))
		{
			sys_err("offlineshop limit table size error");
			thecore_shutdown();
			return;
		}
		data += 2;

		size = decode_2bytes(data);
		data += 2;

		ikashop::CShopManager::instance().LoadShopLimits((TOfflineShopLimit*)data, size);
		data += size * sizeof(TOfflineShopLimit);

		/*
		 * ITEM SHOP
		 */
		if (decode_2bytes(data) != sizeof(TItemShopItem))
		{
			sys_err("item shop table size error");
			thecore_shutdown();
			return;
		}
		data += 2;

		size = decode_2bytes(data);
		data += 2;
		sys_log(0, "BOOT: ITEMSHOP: %d", size);

		if (size)
		{
			TItemShopItem* p = (TItemShopItem*)data;
			CItemShopManager::instance().Initialize(p, size);
		}

		data += size * sizeof(TItemShopItem);

		/*
		 * CRAFTING
		 */
		if (decode_2bytes(data) != sizeof(TCraftingItem))
		{
			sys_err("crafting table size error");
			thecore_shutdown();
			return;
		}
		data += 2;

		size = decode_2bytes(data);
		data += 2;
		sys_log(0, "BOOT: CRAFTING: %d", size);

		if (size)
		{
			TCraftingItem* p = (TCraftingItem*)data;
			CCraftingManager::instance().Initialize(p, size);
		}

		data += size * sizeof(TCraftingItem);

		/*
		 * SPECIAL SHOP ITEMS
		 */
		if (decode_2bytes(data) != sizeof(TSpecialShopItem))
		{
			sys_err("special shop proto table size error");
			thecore_shutdown();
			return;
		}
		data += 2;

		size = decode_2bytes(data);
		data += 2;
		sys_log(0, "BOOT: SPECIAL SHOP ITEMS: %d", size);

		if (size)
		{
			TSpecialShopItem* p = (TSpecialShopItem*)data;
			CSpecialShopManager::instance().InitializeItems(p, size);
		}

		data += size * sizeof(TSpecialShopItem);

		/*
		 * SPECIAL SHOP ITEMS
		 */
		if (decode_2bytes(data) != sizeof(TSpecialShopTable))
		{
			sys_err("special shop table size error");
			thecore_shutdown();
			return;
		}
		data += 2;

		size = decode_2bytes(data);
		data += 2;
		sys_log(0, "BOOT: SPECIAL SHOP: %d", size);

		if (size)
		{
			TSpecialShopTable* p = (TSpecialShopTable*)data;
			CSpecialShopManager::instance().InitializeShops(p, size);
		}

		data += size * sizeof(TSpecialShopTable);

		// special shop meta
		if (decode_2bytes(data) != sizeof(TSpecialShopMetaData))
		{
			sys_err("shop meta data size error");
			thecore_shutdown();
			return;
		}
		data += 2;
		
		size = decode_2bytes(data);
		data += 2;
		
		if (size)
		{
			for (WORD i = 0; i < size; ++i)
			{
				DWORD shop_vnum = *(DWORD*)data;
				data += sizeof(DWORD);
		
				TSpecialShopMetaData meta{};
				memcpy(&meta, (TSpecialShopMetaData*)data, sizeof(TSpecialShopMetaData));
				CSpecialShopManager::instance().SetShopMeta(shop_vnum, meta);
				data += sizeof(TSpecialShopMetaData);
			}
		}

		/*
		 * QUEST_REWARD
		 */
		if (decode_2bytes(data) != sizeof(TQuestRewardItem))
		{
			sys_err("quest reward table size error");
			thecore_shutdown();
			return;
		}
		data += 2;

		size = decode_2bytes(data);
		data += 2;
		sys_log(0, "BOOT: QUEST_REWARD: %d", size);

		if (size)
		{
			TQuestRewardItem* p = (TQuestRewardItem*)data;
			quest::CQuestManager::instance().InitializeQuestReward(p, size);
		}

		data += size * sizeof(TQuestRewardItem);
	}
	set_global_time(*(time_t *) data);
	data += sizeof(time_t);

	if (decode_2bytes(data) != sizeof(TItemIDRangeTable) )
	{
		sys_err("ITEM ID RANGE size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;

	TItemIDRangeTable* range = (TItemIDRangeTable*) data;
	data += size * sizeof(TItemIDRangeTable);

	TItemIDRangeTable* rangespare = (TItemIDRangeTable*) data;
	data += size * sizeof(TItemIDRangeTable);

	//ADMIN_MANAGER

	int ChunkSize = decode_2bytes(data );
	data += 2;
	int HostSize = decode_2bytes(data );
	data += 2;
	sys_log(0, "GM Value Count %d %d", HostSize, ChunkSize  );
	for (int n = 0; n < HostSize; ++n )
	{
		gm_new_host_inert(data );
		sys_log(0, "GM HOST : IP[%s] ", data );
		data += ChunkSize;
	}

	data += 2;
	int adminsize = decode_2bytes(data );
	data += 2;

	for (int n = 0; n < adminsize; ++n )
	{
		tAdminInfo& rAdminInfo = *(tAdminInfo*)data;

		gm_new_insert(rAdminInfo );

		data += sizeof(rAdminInfo );
	}

	//END_ADMIN_MANAGER

	//MONARCH
	data += 2;
	data += 2;

	TMonarchInfo& p = *(TMonarchInfo *) data;
	data += sizeof(TMonarchInfo);

	CMonarch::instance().SetMonarchInfo(&p);

	for (int n = 1; n < 4; ++n)
	{
		if (p.name[n] && *p.name[n])
			sys_log(0, "[MONARCH] Empire %d Pid %d Money %d %s", n, p.pid[n], p.money[n], p.name[n]);
	}

	int CandidacySize = decode_2bytes(data);
	data += 2;

	int CandidacyCount = decode_2bytes(data);
	data += 2;

	if (test_server)
		sys_log (0, "[MONARCH] Size %d Count %d", CandidacySize, CandidacyCount);

	data += CandidacySize * CandidacyCount;

	//END_MONARCH

	// MAINTENANCE
	data += 2;
	int bMaintenance = decode_2bytes(data);
	data += 2;

	//sys_log(0, "otrzymalem ze maintenance %d", bMaintenance);
	g_bIsMaintenance = bMaintenance;
	// END OF MAINTENANCE

	WORD endCheck=decode_2bytes(data);
	if (endCheck != 0xffff)
	{
		sys_err("boot packet end check error [%x]!=0xffff", endCheck);
		thecore_shutdown();
		return;
	}
	else
		sys_log(0, "boot packet end check ok [%x]==0xffff", endCheck );
	data +=2;

	if (!ITEM_MANAGER::instance().SetMaxItemID(*range))
	{
		sys_err("not enough item id contact your administrator!");
		thecore_shutdown();
		return;
	}

	if (!ITEM_MANAGER::instance().SetMaxSpareItemID(*rangespare))
	{
		sys_err("not enough item id for spare contact your administrator!");
		thecore_shutdown();
		return;
	}

	// LOCALE_SERVICE
	const int FILE_NAME_LEN = 256;
	char szCommonDropItemFileName[FILE_NAME_LEN];
	char szETCDropItemFileName[FILE_NAME_LEN];
	char szMOBDropItemFileName[FILE_NAME_LEN];
	char szDropItemGroupFileName[FILE_NAME_LEN];
	char szSpecialItemGroupFileName[FILE_NAME_LEN];
	char szMapIndexFileName[FILE_NAME_LEN];
	char szItemVnumMaskTableFileName[FILE_NAME_LEN];
	char szDragonSoulTableFileName[FILE_NAME_LEN];

	snprintf(szCommonDropItemFileName, sizeof(szCommonDropItemFileName),
			"%s/common_drop_item.txt", LocaleService_GetBasePath().c_str());
	snprintf(szETCDropItemFileName, sizeof(szETCDropItemFileName),
			"%s/etc_drop_item.txt", LocaleService_GetBasePath().c_str());
	snprintf(szMOBDropItemFileName, sizeof(szMOBDropItemFileName),
			"%s/mob_drop_item.txt", LocaleService_GetBasePath().c_str());
	snprintf(szSpecialItemGroupFileName, sizeof(szSpecialItemGroupFileName),
			"%s/special_item_group.txt", LocaleService_GetBasePath().c_str());
	snprintf(szDropItemGroupFileName, sizeof(szDropItemGroupFileName),
			"%s/drop_item_group.txt", LocaleService_GetBasePath().c_str());
	snprintf(szMapIndexFileName, sizeof(szMapIndexFileName),
			"%s/index", LocaleService_GetMapPath().c_str());
	snprintf(szItemVnumMaskTableFileName, sizeof(szItemVnumMaskTableFileName),
			"%s/ori_to_new_table.txt", LocaleService_GetBasePath().c_str());
	snprintf(szDragonSoulTableFileName, sizeof(szDragonSoulTableFileName),
			"%s/dragon_soul_table.txt", LocaleService_GetBasePath().c_str());

	sys_log(0, "Initializing Informations of Cube System");
	Cube_InformationInitialize();

	sys_log(0, "LoadLocaleFile: CommonDropItem: %s", szCommonDropItemFileName);
	if (!ITEM_MANAGER::instance().ReadCommonDropItemFile(szCommonDropItemFileName))
	{
		sys_err("cannot load CommonDropItem: %s", szCommonDropItemFileName);
		thecore_shutdown();
		return;
	}

	sys_log(0, "LoadLocaleFile: ETCDropItem: %s", szETCDropItemFileName);
	if (!ITEM_MANAGER::instance().ReadEtcDropItemFile(szETCDropItemFileName))
	{
		sys_err("cannot load ETCDropItem: %s", szETCDropItemFileName);
		thecore_shutdown();
		return;
	}

	sys_log(0, "LoadLocaleFile: DropItemGroup: %s", szDropItemGroupFileName);
	if (!ITEM_MANAGER::instance().ReadDropItemGroup(szDropItemGroupFileName))
	{
		sys_err("cannot load DropItemGroup: %s", szDropItemGroupFileName);
		thecore_shutdown();
		return;
	}

	sys_log(0, "LoadLocaleFile: SpecialItemGroup: %s", szSpecialItemGroupFileName);
	if (!ITEM_MANAGER::instance().ReadSpecialDropItemFile(szSpecialItemGroupFileName))
	{
		sys_err("cannot load SpecialItemGroup: %s", szSpecialItemGroupFileName);
		thecore_shutdown();
		return;
	}

	sys_log(0, "LoadLocaleFile: ItemVnumMaskTable : %s", szItemVnumMaskTableFileName);
	if (!ITEM_MANAGER::instance().ReadItemVnumMaskTable(szItemVnumMaskTableFileName))
	{
		sys_log(0, "Could not open MaskItemTable");
	}

	sys_log(0, "LoadLocaleFile: MOBDropItemFile: %s", szMOBDropItemFileName);
	if (!ITEM_MANAGER::instance().ReadMonsterDropItemGroup(szMOBDropItemFileName))
	{
		sys_err("cannot load MOBDropItemFile: %s", szMOBDropItemFileName);
		thecore_shutdown();
		return;
	}

	sys_log(0, "LoadLocaleFile: MapIndex: %s", szMapIndexFileName);
	if (!SECTREE_MANAGER::instance().Build(szMapIndexFileName, LocaleService_GetMapPath().c_str()))
	{
		sys_err("cannot load MapIndex: %s", szMapIndexFileName);
		thecore_shutdown();
		return;
	}

	SpecialSpawnManager::instance().Setup();

	sys_log(0, "LoadLocaleFile: DragonSoulTable: %s", szDragonSoulTableFileName);
	if (!DSManager::instance().ReadDragonSoulTableFile(szDragonSoulTableFileName))
	{
		sys_err("cannot load DragonSoulTable: %s", szDragonSoulTableFileName);
		//thecore_shutdown();
		//return;
	}

	// END_OF_LOCALE_SERVICE

	building::CManager::instance().FinalizeBoot();

	CMotionManager::instance().Build();

	signal_timer_enable(30);

	if (test_server)
		CMobManager::instance().DumpRegenCount("mob_count");

	// castle_boot
	castle_boot();

#ifdef ENABLE_QUEST_BOOT_EVENT
	quest::CQuestManager::instance().Boot();
#endif
}

EVENTINFO(quest_login_event_info)
{
	DWORD dwPID;

	quest_login_event_info()
	: dwPID( 0 )
	{
	}
};

EVENTFUNC(quest_login_event)
{
	quest_login_event_info* info = dynamic_cast<quest_login_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "quest_login_event> <Factor> Null pointer" );
		return 0;
	}

	DWORD dwPID = info->dwPID;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwPID);

	if (!ch)
		return 0;

	LPDESC d = ch->GetDesc();

	if (!d)
		return 0;

	if (d->IsPhase(PHASE_HANDSHAKE) ||
		d->IsPhase(PHASE_LOGIN) ||
		d->IsPhase(PHASE_SELECT) ||
		d->IsPhase(PHASE_DEAD) ||
		d->IsPhase(PHASE_LOADING))
	{
		return PASSES_PER_SEC(1);
	}
	else if (d->IsPhase(PHASE_CLOSE))
	{
		return 0;
	}
	else if (d->IsPhase(PHASE_GAME))
	{
		sys_log(0, "QUEST_LOAD: Login pc %d by event", ch->GetPlayerID());
		if (g_bPremiumChannel) {
			ch->ChatDebug("witaj na premium channelu");
		}
		quest::CQuestManager::instance().Login(ch->GetPlayerID());
		return 0;
	}
	else
	{
		sys_err(0, "input_db.cpp:quest_login_event INVALID PHASE pid %d", ch->GetPlayerID());
		return 0;
	}
}

void CInputDB::MoveChannelRespond(LPDESC d, const char* c_pData)
{
	if (d == nullptr)
		return;

	const LPCHARACTER ch = d->GetCharacter();
	if (ch == nullptr)
		return;

	ch->ChangeChannel(reinterpret_cast<const TRespondMoveChannel*>(c_pData));
}

void CInputDB::QuestLoad(LPDESC d, const char * c_pData)
{
	if (NULL == d)
		return;

	LPCHARACTER ch = d->GetCharacter();

	if (NULL == ch)
		return;

	const DWORD dwCount = decode_4bytes(c_pData);

	const TQuestTable* pQuestTable = reinterpret_cast<const TQuestTable*>(c_pData+4);

	if (NULL != pQuestTable)
	{
		if (dwCount != 0)
		{
			if (ch->GetPlayerID() != pQuestTable[0].dwPID)
			{
				sys_err("PID differs %u %u", ch->GetPlayerID(), pQuestTable[0].dwPID);
				return;
			}
		}

		sys_log(0, "QUEST_LOAD: count %d", dwCount);

		quest::PC * pkPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());

		if (!pkPC)
		{
			sys_err("null quest::PC with id %u", pQuestTable[0].dwPID);
			return;
		}

		if (pkPC->IsLoaded())
			return;

		for (unsigned int i = 0; i < dwCount; ++i)
		{
			std::string st(pQuestTable[i].szName);

			st += ".";
			st += pQuestTable[i].szState;

			sys_log(0,  "            %s %d", st.c_str(), pQuestTable[i].lValue);
			pkPC->SetFlag(st.c_str(), pQuestTable[i].lValue, false);
		}

		pkPC->SetLoaded();
		pkPC->Build();

		if (ch->GetDesc()->IsPhase(PHASE_GAME))
		{
			sys_log(0, "QUEST_LOAD: Login pc %d", pQuestTable[0].dwPID);
			quest::CQuestManager::instance().Login(pQuestTable[0].dwPID);
		}
		else
		{
			quest_login_event_info* info = AllocEventInfo<quest_login_event_info>();
			info->dwPID = ch->GetPlayerID();

			event_create(quest_login_event, info, PASSES_PER_SEC(1));
		}
	}
}

void CInputDB::SpecialFlagLoad(LPDESC d, const char* c_pData)
{
	if (NULL == d)
		return;

	LPCHARACTER ch = d->GetCharacter();

	if (NULL == ch)
		return;

	const DWORD dwCount = decode_4bytes(c_pData);

	const TSpecialFlagTable* pSpecialFlagTable = reinterpret_cast<const TSpecialFlagTable*>(c_pData + 4);

	if (NULL != pSpecialFlagTable)
	{
		if (dwCount != 0)
		{
			DWORD pid = ch->GetPlayerID();
			DWORD aid = d->GetAccountTable().id;
			if (pid != pSpecialFlagTable[0].dwPID && aid != pSpecialFlagTable[0].dwAID)
			{
				sys_err("PID AID differs %u=%u %u=%u", pid, pSpecialFlagTable[0].dwPID, aid, pSpecialFlagTable[0].dwAID);
				return;
			}
		}

		sys_log(0, "SPECIAL_FLAG_LOAD: count %d", dwCount);

		for (unsigned int i = 0; i < dwCount; ++i)
		{
			std::string flag(pSpecialFlagTable[i].szFlag);

			sys_log(0, "            %s %lld", flag.c_str(), pSpecialFlagTable[i].llValue);
			ch->SetSpecialFlag(flag.c_str(), pSpecialFlagTable[i].llValue, true, d->IsPhase(PHASE_GAME));
		}
	}
}

void CInputDB::SafeboxLoad(LPDESC d, const char * c_pData)
{
	if (!d)
		return;

	TSafeboxTable * p = (TSafeboxTable *) c_pData;

	if (d->GetAccountTable().id != p->dwID)
	{
		sys_err("SafeboxLoad: safebox has different id %u != %u", d->GetAccountTable().id, p->dwID);
		return;
	}

	if (!d->GetCharacter())
		return;

	LPCHARACTER ch = d->GetCharacter();

	//PREVENT_TRADE_WINDOW
	if (ch->IsBusy(BUSY_SAFEBOX))
	{
		d->GetCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot open a Storeroom while another window is open." ) );
		d->GetCharacter()->CancelSafeboxLoad();
		return;
	}
	//END_PREVENT_TRADE_WINDOW

	if (quest::CQuestManager::instance().GetEventFlag("block_safebox") > 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This feature is temporarily unavailable."));
		return;
	}

	int iSize = ch->GetSafeboxSize();
	if (iSize < 1)
	{
		iSize = 1;
	}

	// ADD_PREMIUM
	if (d->GetCharacter()->GetPremiumRemainSeconds(PREMIUM_SAFEBOX) > 0 ||
		d->GetCharacter()->IsEquipUniqueGroup(UNIQUE_GROUP_LARGE_SAFEBOX))
	{
		iSize = MIN(iSize + SAFEBOX_PREMIUM_PAGE_COUNT, SAFEBOX_PAGE_COUNT);
	}
	// END_OF_ADD_PREMIUM

	d->GetCharacter()->LoadSafebox(iSize, p->dwGold, p->wItemCount, (TPlayerItem *) (c_pData + sizeof(TSafeboxTable)));
}

void CInputDB::SafeboxChangeSize(LPDESC d, const char * c_pData)
{
	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	BYTE bSize = *(BYTE*)c_pData;
	d->GetCharacter()->ChangeSafeboxSize(static_cast<int>(bSize));
}

//
//
void CInputDB::SafeboxWrongPassword(LPDESC d)
{
	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	TPacketCGSafeboxWrongPassword p;
	p.bHeader = HEADER_GC_SAFEBOX_WRONG_PASSWORD;
	d->Packet(&p, sizeof(p));

	d->GetCharacter()->CancelSafeboxLoad();
}

void CInputDB::SafeboxChangePasswordAnswer(LPDESC d, const char* c_pData)
{
	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	TSafeboxChangePasswordPacketAnswer* p = (TSafeboxChangePasswordPacketAnswer*) c_pData;
	if (p->flag)
	{
		d->GetCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Storeroom] Storeroom password has been changed."));
	}
	else
	{
		d->GetCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Storeroom] You have entered the wrong password."));
	}
}

void CInputDB::MallLoad(LPDESC d, const char * c_pData)
{
	if (!d)
		return;

	TSafeboxTable * p = (TSafeboxTable *) c_pData;

	if (d->GetAccountTable().id != p->dwID)
	{
		sys_err("safebox has different id %u != %u", d->GetAccountTable().id, p->dwID);
		return;
	}

	if (!d->GetCharacter())
		return;

	d->GetCharacter()->LoadMall(p->wItemCount, (TPlayerItem *) (c_pData + sizeof(TSafeboxTable)));
}

void CInputDB::LoginAlready(LPDESC d, const char * c_pData)
{
	if (!d)
		return;

	{
		TPacketDGLoginAlready * p = (TPacketDGLoginAlready *) c_pData;

		LPDESC d2 = DESC_MANAGER::instance().FindByLoginName(p->szLogin);

		if (d2)
			d2->DisconnectOfSameLogin();
		else
		{
			TPacketGGDisconnect pgg;

			pgg.bHeader = HEADER_GG_DISCONNECT;
			strlcpy(pgg.szLogin, p->szLogin, sizeof(pgg.szLogin));

			P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGDisconnect));
		}
	}
	// END_OF_INTERNATIONAL_VERSION

	LoginFailure(d, "ALREADY");
}

void CInputDB::EmpireSelect(LPDESC d, const char * c_pData)
{
	sys_log(0, "EmpireSelect %p", get_pointer(d));

	if (!d)
		return;

	TAccountTable & rTable = d->GetAccountTable();
	rTable.bEmpire = *(BYTE *) c_pData;

	TPacketGCEmpire pe;
	pe.bHeader = HEADER_GC_EMPIRE;
	pe.bEmpire = rTable.bEmpire;
	d->Packet(&pe, sizeof(pe));

	for (int i = 0; i < PLAYER_PER_ACCOUNT; ++i)
		if (rTable.players[i].dwID)
		{
			rTable.players[i].x = EMPIRE_START_X(rTable.bEmpire);
			rTable.players[i].y = EMPIRE_START_Y(rTable.bEmpire);
		}

	GetServerLocation(d->GetAccountTable(), rTable.bEmpire);

	d->SendLoginSuccessPacket();
}

void CInputDB::MapLocations(const char * c_pData)
{
	BYTE bCount = *(BYTE *) (c_pData++);

	sys_log(0, "InputDB::MapLocations %d", bCount);

	TMapLocation * pLoc = (TMapLocation *) c_pData;

	while (bCount--)
	{
		for (int i = 0; i < MAP_ALLOW_LIMIT; ++i)
		{
			if (0 == pLoc->alMaps[i])
				break;

			#ifdef ENABLE_MOVE_CHANNEL
			CMapLocation::instance().Insert(pLoc->alMaps[i], pLoc->szHost, pLoc->wPort, pLoc->channel);
			#else
			CMapLocation::instance().Insert(pLoc->alMaps[i], pLoc->szHost, pLoc->wPort);
			#endif
		}

		pLoc++;
	}

	// MapLocations is the first point at which this core knows which maps it
	// hosts. Spawn the initial descriptors here; OnPlayerLoaded then starts the
	// regular manager update event.  Keeping this in Update() creates a startup
	// deadlock, because there is no update event before the first bot has been
	// loaded.
	//
	// Each kingdom's four maps sit on one core (m2-render-config: Shinsoo on
	// first, Chunjo on game1, Jinno on game2), so a core starts the kingdoms
	// whose village map it holds and no others. The budget is the operator's
	// single number, split between the kingdoms that have registered identities.
	if (CPlayerBotManager::instance().GetCount() == 0)
	{
		int autoSpawnCount = 350;
		const char* configuredCount = std::getenv("PLAYERBOT_AUTOSPAWN_COUNT");
		if (configuredCount && *configuredCount)
			autoSpawnCount = std::atoi(configuredCount);
		// The ceiling is the launcher slider's; the real guard is below:
		// SplitPopulation caps each kingdom at the identities it has, and
		// SpawnRegistered at what LoadRegisteredBots accepted.
		const int autoSpawnCeiling = 2500;
		if (autoSpawnCount < 0)
			autoSpawnCount = 0;
		else if (autoSpawnCount > autoSpawnCeiling)
		{
			sys_log(0, "PLAYERBOT: autospawn asked=%d, cut to the ceiling %d",
					autoSpawnCount, autoSpawnCeiling);
			autoSpawnCount = autoSpawnCeiling;
		}

		int registered[playerbot_empire_rules::EMPIRE_COUNT];
		int want[playerbot_empire_rules::EMPIRE_COUNT];
		CPlayerBotManager::instance().CountRegisteredPerEmpire(
				registered, playerbot_empire_rules::EMPIRE_COUNT);
		// M2_PLAYERBOT_KINGDOMS=0 used to reach only the seed: a world that had
		// once run with 1 kept its Shinsoo and Jinno identities registered, and
		// every core went on starting them ("ustawilem KINGDOMS=0, a boty i tak
		// pojawiaja sie w Jinno i Shinsoo"). The switch comes to the core
		// through the game service's environment now, and a 0 leaves the two
		// new kingdoms with no budget. Their bots stay in the database as they
		// are; a 1 later starts them again.
		const char* kingdomsSwitch = std::getenv("M2_PLAYERBOT_KINGDOMS");
		if (kingdomsSwitch && *kingdomsSwitch && std::atoi(kingdomsSwitch) == 0)
		{
			sys_log(0, "PLAYERBOT: M2_PLAYERBOT_KINGDOMS=0, Shinsoo (%d) and Jinno (%d) are not started",
					registered[playerbot_empire_rules::EMPIRE_SHINSOO],
					registered[playerbot_empire_rules::EMPIRE_JINNO]);
			registered[playerbot_empire_rules::EMPIRE_SHINSOO] = 0;
			registered[playerbot_empire_rules::EMPIRE_JINNO] = 0;
		}
		playerbot_empire_rules::SplitPopulation(autoSpawnCount, registered, want);

		for (int empire = playerbot_empire_rules::EMPIRE_SHINSOO;
				empire <= playerbot_empire_rules::EMPIRE_JINNO; ++empire)
		{
			const long lVillage = playerbot_empire_rules::GetHomeMap(
					empire, playerbot_empire_rules::MAP_ROLE_M1);
			if (lVillage == 0 || !map_allow_find(lVillage) || want[empire] <= 0)
				continue;
			const size_t spawned = CPlayerBotManager::instance().SpawnRegistered(
					(size_t)want[empire], (BYTE)empire);
			sys_log(0, "PLAYERBOT: autospawn empire=%d village=%ld requested=%d registered=%d started=%u",
					empire, lVillage, want[empire], registered[empire],
					(unsigned int)spawned);
		}
	}
}

void CInputDB::P2P(const char * c_pData)
{
	extern LPFDWATCH main_fdw;

	TPacketDGP2P * p = (TPacketDGP2P *) c_pData;

	P2P_MANAGER& mgr = P2P_MANAGER::instance();

	if (false == DESC_MANAGER::instance().IsP2PDescExist(p->szHost, p->wPort))
	{
	    LPCLIENT_DESC pkDesc = NULL;
		sys_log(0, "InputDB:P2P %s:%u", p->szHost, p->wPort);
	    pkDesc = DESC_MANAGER::instance().CreateConnectionDesc(main_fdw, p->szHost, p->wPort, PHASE_P2P, false);
		mgr.RegisterConnector(pkDesc);
		pkDesc->SetP2P(p->szHost, p->wPort, p->bChannel);
	}
}

void CInputDB::GuildLoad(const char * c_pData)
{
	CGuildManager::instance().LoadGuild(*(DWORD *) c_pData);
}

void CInputDB::GuildSkillUpdate(const char* c_pData)
{
	TPacketGuildSkillUpdate * p = (TPacketGuildSkillUpdate *) c_pData;

	CGuild * g = CGuildManager::instance().TouchGuild(p->guild_id);

	if (g)
	{
		g->UpdateSkill(p->skill_point, p->skill_levels);
		g->GuildPointChange(POINT_SP, p->amount, p->save?true:false);
	}
}

void CInputDB::GuildWar(const char* c_pData)
{
	TPacketGuildWar * p = (TPacketGuildWar*) c_pData;

	sys_log(0, "InputDB::GuildWar %u %u state %d", p->dwGuildFrom, p->dwGuildTo, p->bWar);

	switch (p->bWar)
	{
		case GUILD_WAR_SEND_DECLARE:
		case GUILD_WAR_RECV_DECLARE:
			CGuildManager::instance().DeclareWar(p->dwGuildFrom, p->dwGuildTo, p->bType);
			break;

		case GUILD_WAR_REFUSE:
			CGuildManager::instance().RefuseWar(p->dwGuildFrom, p->dwGuildTo);
			break;

		case GUILD_WAR_WAIT_START:
			CGuildManager::instance().WaitStartWar(p->dwGuildFrom, p->dwGuildTo);
			break;

		case GUILD_WAR_CANCEL:
			CGuildManager::instance().CancelWar(p->dwGuildFrom, p->dwGuildTo);
			break;

		case GUILD_WAR_ON_WAR:
			CGuildManager::instance().StartWar(p->dwGuildFrom, p->dwGuildTo);
			break;

		case GUILD_WAR_END:
			CGuildManager::instance().EndWar(p->dwGuildFrom, p->dwGuildTo);
			break;

		case GUILD_WAR_OVER:
			CGuildManager::instance().WarOver(p->dwGuildFrom, p->dwGuildTo, p->bType);
			break;

		case GUILD_WAR_RESERVE:
			CGuildManager::instance().ReserveWar(p->dwGuildFrom, p->dwGuildTo, p->bType);
			break;

		default:
			sys_err("Unknown guild war state");
			break;
	}
}

void CInputDB::GuildWarScore(const char* c_pData)
{
	TPacketGuildWarScore* p = (TPacketGuildWarScore*) c_pData;
	CGuild * g = CGuildManager::instance().TouchGuild(p->dwGuildGainPoint);
	g->SetWarScoreAgainstTo(p->dwGuildOpponent, p->lScore);
}

void CInputDB::GuildSkillRecharge()
{
	CGuildManager::instance().SkillRecharge();
}

void CInputDB::GuildExpUpdate(const char* c_pData)
{
	TPacketGuildSkillUpdate * p = (TPacketGuildSkillUpdate *) c_pData;
	sys_log(1, "GuildExpUpdate %d", p->amount);

	CGuild * g = CGuildManager::instance().TouchGuild(p->guild_id);

	if (g)
		g->GuildPointChange(POINT_EXP, p->amount);
}

void CInputDB::GuildAddMember(const char* c_pData)
{
	TPacketDGGuildMember * p = (TPacketDGGuildMember *) c_pData;
	CGuild * g = CGuildManager::instance().TouchGuild(p->dwGuild);

	if (g)
		g->AddMember(p);
}

void CInputDB::GuildRemoveMember(const char* c_pData)
{
	TPacketGuild* p=(TPacketGuild*)c_pData;
	CGuild* g = CGuildManager::instance().TouchGuild(p->dwGuild);

	if (g)
		g->RemoveMember(p->dwInfo);
}

void CInputDB::GuildChangeGrade(const char* c_pData)
{
	TPacketGuild* p=(TPacketGuild*)c_pData;
	CGuild* g = CGuildManager::instance().TouchGuild(p->dwGuild);

	if (g)
		g->P2PChangeGrade((BYTE)p->dwInfo);
}

void CInputDB::GuildChangeMemberData(const char* c_pData)
{
	sys_log(0, "Recv GuildChangeMemberData");
	TPacketGuildChangeMemberData * p = (TPacketGuildChangeMemberData *) c_pData;
	CGuild * g = CGuildManager::instance().TouchGuild(p->guild_id);

	if (g)
		g->ChangeMemberData(p->pid, p->offer, p->level, p->grade);
}

void CInputDB::GuildDisband(const char* c_pData)
{
	TPacketGuild * p = (TPacketGuild*) c_pData;
	CGuildManager::instance().DisbandGuild(p->dwGuild);
}

void CInputDB::GuildLadder(const char* c_pData)
{
	TPacketGuildLadder* p = (TPacketGuildLadder*) c_pData;
	sys_log(0, "Recv GuildLadder %u %d / w %d d %d l %d", p->dwGuild, p->lLadderPoint, p->lWin, p->lDraw, p->lLoss);
	CGuild * g = CGuildManager::instance().TouchGuild(p->dwGuild);

	g->SetLadderPoint(p->lLadderPoint);
	g->SetWarData(p->lWin, p->lDraw, p->lLoss);
}

void CInputDB::ItemLoad(LPDESC d, const char * c_pData)
{
	LPCHARACTER ch;

	if (!d || !(ch = d->GetCharacter()))
		return;

	if (ch->IsItemLoaded())
		return;

	DWORD dwCount = decode_4bytes(c_pData);
	c_pData += sizeof(DWORD);

	sys_log(0, "ITEM_LOAD: COUNT %s %u", ch->GetName(), dwCount);

	std::vector<LPITEM> v;

	TPlayerItem * p = (TPlayerItem *) c_pData;

	for (DWORD i = 0; i < dwCount; ++i, ++p)
	{
		LPITEM item = ITEM_MANAGER::instance().CreateItem(p->vnum, p->count, p->id);

		if (!item)
		{
			sys_err("cannot create item by vnum %u (name %s id %u)", p->vnum, ch->GetName(), p->id);
			continue;
		}

		item->SetSkipSave(true);
		item->SetSockets(p->alSockets);
		item->SetAttributes(p->aAttr);

#ifdef ENABLE_HIGHLIGHT_NEW_ITEM
		item->SetLastOwnerPID(p->owner);
#endif

#ifdef ENABLE_BELT_INVENTORY_EX
		if (p->window == BELT_INVENTORY)
		{
			p->window = INVENTORY;
			p->pos = p->pos + BELT_INVENTORY_SLOT_START;
		}
#endif

		if ((p->window == INVENTORY && ch->GetInventoryItem(p->pos)) ||
				(p->window == EQUIPMENT && ch->GetWear(p->pos)))
		{
			sys_log(0, "ITEM_RESTORE: %s %s", ch->GetName(), item->GetName());
			v.emplace_back(item);
		}
		else
		{
			switch (p->window)
			{
				case INVENTORY:
				case DRAGON_SOUL_INVENTORY:
					item->AddToCharacter(ch, TItemPos(p->window, p->pos));
					break;

				case EQUIPMENT:
					if (item->CheckItemUseLevel(ch->GetLevel()) == true )
					{
						if (item->EquipTo(ch, p->pos) == false )
						{
							v.emplace_back(item);
						}
					}
					else
					{
						v.emplace_back(item);
					}
					break;
			}
		}

		if (false == item->OnAfterCreatedItem())
			sys_err("Failed to call ITEM::OnAfterCreatedItem (vnum: %d, id: %d)", item->GetVnum(), item->GetID());

		item->SetSkipSave(false);
	}

	itertype(v) it = v.begin();

	while (it != v.end())
	{
		LPITEM item = *(it++);

		int pos = ch->GetEmptyInventory(item->GetSize());

		if (pos < 0)
		{
			PIXEL_POSITION coord;
			coord.x = ch->GetX();
			coord.y = ch->GetY();

			item->AddToGround(ch->GetMapIndex(), coord);
			item->SetOwnership(ch, 180);
			item->StartDestroyEvent();
		}
		else
			item->AddToCharacter(ch, TItemPos(INVENTORY, pos));
	}

	ch->CheckMaximumPoints();
	ch->PointsPacket();

	ch->SetItemLoaded();
}

void CInputDB::AffectLoad(LPDESC d, const char * c_pData)
{
	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	LPCHARACTER ch = d->GetCharacter();

	DWORD dwPID = decode_4bytes(c_pData);
	c_pData += sizeof(DWORD);

	DWORD dwCount = decode_4bytes(c_pData);
	c_pData += sizeof(DWORD);

	if (ch->GetPlayerID() != dwPID)
		return;

	ch->LoadAffect(dwCount, (TPacketAffectElement *) c_pData);
}

void CInputDB::PartyCreate(const char* c_pData)
{
	TPacketPartyCreate* p = (TPacketPartyCreate*) c_pData;
	CPartyManager::instance().P2PCreateParty(p->dwLeaderPID);
}

void CInputDB::PartyDelete(const char* c_pData)
{
	TPacketPartyDelete* p = (TPacketPartyDelete*) c_pData;
	CPartyManager::instance().P2PDeleteParty(p->dwLeaderPID);
}

void CInputDB::PartyAdd(const char* c_pData)
{
	TPacketPartyAdd* p = (TPacketPartyAdd*) c_pData;
	CPartyManager::instance().P2PJoinParty(p->dwLeaderPID, p->dwPID, p->bState);
}

void CInputDB::PartyRemove(const char* c_pData)
{
	TPacketPartyRemove* p = (TPacketPartyRemove*) c_pData;
	CPartyManager::instance().P2PQuitParty(p->dwPID);
}

void CInputDB::PartyStateChange(const char* c_pData)
{
	TPacketPartyStateChange * p = (TPacketPartyStateChange *) c_pData;
	LPPARTY pParty = CPartyManager::instance().P2PCreateParty(p->dwLeaderPID);

	if (!pParty)
		return;

	pParty->SetRole(p->dwPID, p->bRole, p->bFlag);
}

void CInputDB::PartySetMemberLevel(const char* c_pData)
{
	TPacketPartySetMemberLevel* p = (TPacketPartySetMemberLevel*) c_pData;
	LPPARTY pParty = CPartyManager::instance().P2PCreateParty(p->dwLeaderPID);

	if (!pParty)
		return;

	pParty->P2PSetMemberLevel(p->dwPID, p->bLevel);
}

void CInputDB::Time(const char * c_pData)
{
	set_global_time(*(time_t *) c_pData);
}

void CInputDB::ReloadProto(const char * c_pData)
{
	WORD wSize;

	/*
	 * Skill
	 */
	wSize = decode_2bytes(c_pData);
	c_pData += sizeof(WORD);
	if (wSize) CSkillManager::instance().Initialize((TSkillTable *) c_pData, wSize);
	c_pData += sizeof(TSkillTable) * wSize;

	/*
	 * Banwords
	 */

	wSize = decode_2bytes(c_pData);
	c_pData += sizeof(WORD);
	CBanwordManager::instance().Initialize((TBanwordTable *) c_pData, wSize);
	c_pData += sizeof(TBanwordTable) * wSize;

	/*
	 * ITEM
	 */
	wSize = decode_2bytes(c_pData);
	c_pData += 2;
	sys_log(0, "RELOAD: ITEM: %d", wSize);

	if (wSize)
	{
		ITEM_MANAGER::instance().Initialize((TItemTable *) c_pData, wSize);
		c_pData += wSize * sizeof(TItemTable);
	}

	/*
	 * MONSTER
	 */
	wSize = decode_2bytes(c_pData);
	c_pData += 2;
	sys_log(0, "RELOAD: MOB: %d", wSize);

	if (wSize)
	{
		CMobManager::instance().Initialize((TMobTable *) c_pData, wSize);
		c_pData += wSize * sizeof(TMobTable);
	}

	/*
	 * CRAFTING
	 */
	wSize = decode_2bytes(c_pData);
	c_pData += 2;
	sys_log(0, "RELOAD: CRAFTING: %d", wSize);

	if (wSize)
	{
		CCraftingManager::instance().Initialize((TCraftingItem*)c_pData, wSize);
		c_pData += wSize * sizeof(TCraftingItem);
	}

	///*
	// * special shop item
	// */
	//wSize = decode_2bytes(c_pData);
	//c_pData += 2;
	//sys_log(0, "RELOAD: SPECIAL SHOP ITEM: %d", wSize);

	//if (wSize)
	//{
	//	CSpecialShopManager::instance().InitializeItems((TSpecialShopItem*)c_pData, wSize);
	//	c_pData += wSize * sizeof(TSpecialShopItem);
	//}

	///*
	// * special shop shop
	// */
	//wSize = decode_2bytes(c_pData);
	//c_pData += 2;
	//sys_log(0, "RELOAD: SPECIAL SHOP: %d", wSize);

	//if (wSize)
	//{
	//	CSpecialShopManager::instance().InitializeShops((TSpecialShopTable*)c_pData, wSize);
	//	c_pData += wSize * sizeof(TSpecialShopTable);
	//}

	/*
	 * QUEST_REWARD
	 */
	wSize = decode_2bytes(c_pData);
	c_pData += 2;
	sys_log(0, "RELOAD: QUEST_REWARD: %d", wSize);

	if (wSize)
	{
		quest::CQuestManager::instance().InitializeQuestReward((TQuestRewardItem*)c_pData, wSize);
		c_pData += wSize * sizeof(TQuestRewardItem);
	}

	CMotionManager::instance().Build();

	CHARACTER_MANAGER::instance().for_each_pc(std::mem_fn(&CHARACTER::ComputePoints));
}

void CInputDB::ReloadItemShop(const char* c_pData)
{
	WORD wSize;

	wSize = decode_2bytes(c_pData);
	c_pData += 2;
	sys_log(0, "RELOAD: ITEMSHOP: %d", wSize);

	if (wSize)
	{
		CItemShopManager::instance().Initialize((TItemShopItem*)c_pData, wSize);
		c_pData += wSize * sizeof(TItemShopItem);
	}

	// update kazdemu zeby IS odswiezyc
	CHARACTER_MANAGER::instance().for_each_pc(std::mem_fn(&CHARACTER::RefreshItemShopData));
}

void CInputDB::GuildSkillUsableChange(const char* c_pData)
{
	TPacketGuildSkillUsableChange* p = (TPacketGuildSkillUsableChange*) c_pData;

	CGuild* g = CGuildManager::instance().TouchGuild(p->dwGuild);

	g->SkillUsableChange(p->dwSkillVnum, p->bUsable?true:false);
}

void CInputDB::AuthLogin(LPDESC d, const char * c_pData)
{
	if (!d)
		return;

	BYTE bResult = *(BYTE *) c_pData;

	TPacketGCAuthSuccess ptoc;

	ptoc.bHeader = HEADER_GC_AUTH_SUCCESS;

	if (bResult)
	{
		SendPanamaList(d);
		ptoc.dwLoginKey = d->GetLoginKey();

		//Send Client Package CryptKey
		{
			DESC_MANAGER::instance().SendClientPackageCryptKey(d);
			DESC_MANAGER::instance().SendClientPackageSDBToLoadMap(d, MAPNAME_DEFAULT);
		}
	}
	else
	{
		ptoc.dwLoginKey = 0;
	}

	ptoc.bResult = bResult;

	// Validating handshake
	TPacketGGHandshakeValidate pack;
	pack.header = HEADER_GG_HANDSHAKE_VALIDATION;
	strlcpy(pack.sUserIP, d->GetHostName(), sizeof(pack.sUserIP));
	P2P_MANAGER::instance().Send(&pack, sizeof(pack));

	d->Packet(&ptoc, sizeof(TPacketGCAuthSuccess));
	sys_log(0, "AuthLogin result %u key %u", bResult, d->GetLoginKey());
}

void CInputDB::ChangeEmpirePriv(const char* c_pData)
{
	TPacketDGChangeEmpirePriv* p = (TPacketDGChangeEmpirePriv*) c_pData;

	// ADD_EMPIRE_PRIV_TIME
	CPrivManager::instance().GiveEmpirePriv(p->empire, p->type, p->value, p->bLog, p->end_time_sec);
	// END_OF_ADD_EMPIRE_PRIV_TIME
}

void CInputDB::ChangeGuildPriv(const char* c_pData)
{
	TPacketDGChangeGuildPriv* p = (TPacketDGChangeGuildPriv*) c_pData;

	// ADD_GUILD_PRIV_TIME
	CPrivManager::instance().GiveGuildPriv(p->guild_id, p->type, p->value, p->bLog, p->end_time_sec);
	// END_OF_ADD_GUILD_PRIV_TIME
}

void CInputDB::ChangeCharacterPriv(const char* c_pData)
{
	TPacketDGChangeCharacterPriv* p = (TPacketDGChangeCharacterPriv*) c_pData;
	CPrivManager::instance().GiveCharacterPriv(p->pid, p->type, p->value, p->bLog);
}

void CInputDB::MoneyLog(const char* c_pData)
{
	TPacketMoneyLog * p = (TPacketMoneyLog *) c_pData;

	if (p->type == 4) // QUEST_MONEY_LOG_SKIP
		return;

	if (g_bAuthServer ==true )
		return;

	LogManager::instance().MoneyLog(p->type, p->vnum, p->gold);
}

void CInputDB::GuildMoneyChange(const char* c_pData)
{
	TPacketDGGuildMoneyChange* p = (TPacketDGGuildMoneyChange*) c_pData;

	CGuild* g = CGuildManager::instance().TouchGuild(p->dwGuild);
	if (g)
	{
		g->RecvMoneyChange(p->iTotalGold);
	}
}

void CInputDB::GuildWithdrawMoney(const char* c_pData)
{
	TPacketDGGuildMoneyWithdraw* p = (TPacketDGGuildMoneyWithdraw*) c_pData;

	CGuild* g = CGuildManager::instance().TouchGuild(p->dwGuild);
	if (g)
	{
		g->RecvWithdrawMoneyGive(p->iChangeGold);
	}
}

void CInputDB::SetEventFlag(const char* c_pData)
{
	TPacketSetEventFlag* p = (TPacketSetEventFlag*) c_pData;
	quest::CQuestManager::instance().SetEventFlag(p->szFlagName, p->lValue);
}

void CInputDB::CreateObject(const char * c_pData)
{
	using namespace building;
	CManager::instance().LoadObject((TObject *) c_pData);
}

void CInputDB::DeleteObject(const char * c_pData)
{
	using namespace building;
	CManager::instance().DeleteObject(*(DWORD *) c_pData);
}

void CInputDB::UpdateLand(const char * c_pData)
{
	using namespace building;
	CManager::instance().UpdateLand((TLand *) c_pData);
}

void CInputDB::Notice(const char * c_pData)
{
	char szBuf[256+1];
	strlcpy(szBuf, c_pData, sizeof(szBuf));

	sys_log(0, "InputDB:: Notice: %s", szBuf);

	//SendNotice(LC_TEXT(szBuf));
	SendNotice(szBuf);
}

void CInputDB::GuildWarNotice(TPacketDGGuildWarNotice* p)
{
	char buf[256];
	snprintf(buf, sizeof(buf), LC_TEXT("The war between %s and %s will start after %d minutes!"), p->guild1, p->guild2, p->duration);
	SendNotice(buf);
}

void CInputDB::LocaleNotice(const char* c_pData)
{
	char szBuf[256 + 1];
	strlcpy(szBuf, c_pData, sizeof(szBuf));

	sys_log(0, "InputDB:: Notice: %s", szBuf);

	SendNotice(LC_TEXT(szBuf));
}

void CInputDB::GuildWarReserveAdd(TGuildWarReserve * p)
{
	CGuildManager::instance().ReserveWarAdd(p);
}

void CInputDB::GuildWarReserveDelete(DWORD dwID)
{
	CGuildManager::instance().ReserveWarDelete(dwID);
}

void CInputDB::GuildWarBet(TPacketGDGuildWarBet * p)
{
	CGuildManager::instance().ReserveWarBet(p);
}

void CInputDB::MarriageAdd(TPacketMarriageAdd * p)
{
	sys_log(0, "MarriageAdd %u %u %u %s %s", p->dwPID1, p->dwPID2, (DWORD)p->tMarryTime, p->szName1, p->szName2);
	marriage::CManager::instance().Add(p->dwPID1, p->dwPID2, p->tMarryTime, p->szName1, p->szName2);
}

void CInputDB::MarriageUpdate(TPacketMarriageUpdate * p)
{
	sys_log(0, "MarriageUpdate %u %u %d %d", p->dwPID1, p->dwPID2, p->iLovePoint, p->byMarried);
	marriage::CManager::instance().Update(p->dwPID1, p->dwPID2, p->iLovePoint, p->byMarried);
}

void CInputDB::MarriageRemove(TPacketMarriageRemove * p)
{
	sys_log(0, "MarriageRemove %u %u", p->dwPID1, p->dwPID2);
	marriage::CManager::instance().Remove(p->dwPID1, p->dwPID2);
}

void CInputDB::WeddingRequest(TPacketWeddingRequest* p)
{
	marriage::WeddingManager::instance().Request(p->dwPID1, p->dwPID2);
}

void CInputDB::WeddingReady(TPacketWeddingReady* p)
{
	sys_log(0, "WeddingReady %u %u %u", p->dwPID1, p->dwPID2, p->dwMapIndex);
	marriage::CManager::instance().WeddingReady(p->dwPID1, p->dwPID2, p->dwMapIndex);
}

void CInputDB::WeddingStart(TPacketWeddingStart* p)
{
	sys_log(0, "WeddingStart %u %u", p->dwPID1, p->dwPID2);
	marriage::CManager::instance().WeddingStart(p->dwPID1, p->dwPID2);
}

void CInputDB::WeddingEnd(TPacketWeddingEnd* p)
{
	sys_log(0, "WeddingEnd %u %u", p->dwPID1, p->dwPID2);
	marriage::CManager::instance().WeddingEnd(p->dwPID1, p->dwPID2);
}

// MYSHOP_PRICE_LIST
void CInputDB::MyshopPricelistRes(LPDESC d, const TPacketMyshopPricelistHeader* p )
{
	LPCHARACTER ch;

	if (!d || !(ch = d->GetCharacter()) )
		return;

	sys_log(0, "RecvMyshopPricelistRes name[%s]", ch->GetName());
	ch->UseSilkBotaryReal(p );
}
// END_OF_MYSHOP_PRICE_LIST

//RELOAD_ADMIN
void CInputDB::ReloadAdmin(const char * c_pData )
{
	gm_new_clear();
	int ChunkSize = decode_2bytes(c_pData );
	c_pData += 2;
	int HostSize = decode_2bytes(c_pData );
	c_pData += 2;

	for (int n = 0; n < HostSize; ++n )
	{
		gm_new_host_inert(c_pData );
		c_pData += ChunkSize;
	}

	c_pData += 2;
	int size = 	decode_2bytes(c_pData );
	c_pData += 2;

	for (int n = 0; n < size; ++n )
	{
		tAdminInfo& rAdminInfo = *(tAdminInfo*)c_pData;

		gm_new_insert(rAdminInfo );

		c_pData += sizeof (tAdminInfo );

		LPCHARACTER pChar = CHARACTER_MANAGER::instance().FindPC(rAdminInfo.m_szName );
		if (pChar )
		{
			pChar->SetGMLevel();
		}
	}
}
#ifdef ENABLE_IKASHOP_RENEWAL
template <class T>
const T& Decode(const char*& data){
	auto obj = reinterpret_cast<const T*>(data);
	data += sizeof(T);
	return *obj;
}

void IkarusShopLoadTables(const char* data)
{
	// decoding subpacket
	const auto& subpack = Decode<ikashop::TSubPacketDGLoadTables>(data);	
	
	auto& manager = ikashop::GetManager();

	// decoding shop & items
	for (DWORD i = 0; i < subpack.shopcount; i++)
	{
		const auto& shopInfo = Decode<ikashop::TShopInfo>(data);
		auto shop = manager.PutsNewShop(shopInfo);

		for (DWORD j = 0; j < shopInfo.count; j++)
			shop->AddItem(Decode<ikashop::TShopItem>(data));
	}

	// decoding private offers
	for (DWORD i = 0; i < subpack.offercount; i++)
	{
		const auto& offerInfo = Decode<ikashop::TOfferInfo>(data);
		
		auto shop = manager.GetShopByOwnerID(offerInfo.ownerid);
		if (!shop)
		{
			sys_err("CANNOT FIND SHOP BY OWNERID (TOfferInfo) %d ", offerInfo.ownerid);
			continue;
		}

		auto offer = std::make_shared<ikashop::TOfferInfo>(offerInfo);
		shop->AddOffer(offer);
		manager.PutsNewOffer(offer);
	}

	// decoding auctions
	for (DWORD i = 0; i < subpack.auctioncount; i++)
	{
		const auto& auction = Decode<ikashop::TAuctionInfo>(data);
		manager.PutsAuction(auction);
		
	}

	// decoding auction offers
	for (DWORD i = 0; i < subpack.auctionoffercount; i++)
	{
		const auto& offer = Decode<ikashop::TAuctionOfferInfo>(data);
		manager.PutsAuctionOffer(offer);
		
	}

}

void IkarusShopBuyItemPacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGBuyItem>(data);
	ikashop::GetManager().RecvShopBuyDBPacket(subpack.buyerid, subpack.ownerid, subpack.itemid, subpack.requester);
}

void IkarusShopLockedBuyItemPacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGLockedBuyItem>(data);
	ikashop::GetManager().RecvShopLockedBuyItemDBPacket(subpack.buyerid, subpack.ownerid, subpack.itemid);
}

void IkarusShopEditItemPacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGEditItem>(data);
	ikashop::GetManager().RecvShopEditItemDBPacket(subpack.ownerid , subpack.itemid, subpack.price);
}

void IkarusShopRemoveItemPacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGRemoveItem>(data);
	ikashop::GetManager().RecvShopRemoveItemDBPacket(subpack.ownerid , subpack.itemid, subpack.requester);
}

void IkarusShopAddItemPacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGAddItem>(data);
	ikashop::GetManager().RecvShopAddItemDBPacket(subpack.ownerid, subpack.item);
}

void IkarusShopForceClosePacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGShopForceClose>(data);
	ikashop::GetManager().RecvShopForceCloseDBPacket(subpack.ownerid);
}

void IkarusShopShopCreateNewPacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGShopCreateNew>(data);
	ikashop::GetManager().RecvShopCreateNewDBPacket(subpack.shop, subpack.items);
}

void IkarusShopShopChangeNamePacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGShopChangeName>(data);
	ikashop::GetManager().RecvShopChangeNameDBPacket(subpack.ownerid , subpack.name);
}

void IkarusShopOfferCreatePacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGOfferCreate>(data);
	ikashop::GetManager().RecvShopOfferNewDBPacket(subpack.offer);
}

void IkarusShopOfferNotifiedPacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGOfferNotified>(data);
	ikashop::GetManager().RecvShopOfferNotifiedDBPacket(subpack.offerid , subpack.ownerid);
}

void IkarusShopOfferAcceptPacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGOfferAccept>(data);
	ikashop::GetManager().RecvShopOfferAcceptDBPacket(subpack.offerid , subpack.ownerid);
}

void IkarusShopOfferCancelPacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGOfferCancel>(data);
	ikashop::GetManager().RecvShopOfferCancelDBPacket(subpack.offerid , subpack.ownerid, subpack.removing);
}

void IkarusShopSafeboxAddItemPacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGSafeboxAddItem>(data);
	ikashop::GetManager().RecvShopSafeboxAddItemDBPacket(subpack.ownerid , subpack.itemid , subpack.item);
}

void IkarusShopSafeboxAddValutesPacket(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGSafeboxAddValutes>(data);
	ikashop::GetManager().RecvShopSafeboxAddValutesDBPacket(subpack.ownerid , subpack.valute);
}

void IkarusShopSafeboxLoad(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGSafeboxLoad>(data);
	std::vector<ikashop::TShopPlayerItem> items;
	items.reserve(subpack.itemcount);

	for (DWORD i = 0; i < subpack.itemcount; i++)
		items.emplace_back(Decode<ikashop::TShopPlayerItem>(data));

	ikashop::GetManager().RecvShopSafeboxLoadDBPacket(subpack.ownerid , subpack.valute, items);
}

void IkarusShopSafeboxExpiredItem(const char* data) {
	const auto& subpack = Decode<ikashop::TSubPacketDGSafeboxExpiredItem>(data);
	ikashop::GetManager().RecvShopSafeboxExpiredItemDBPacket(subpack.ownerid, subpack.itemid);
}

void IkarusShopSafeboxGetItemConfirm(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGSafeboxGetItemConfirm>(data);
	ikashop::GetManager().RecvShopSafeboxGetItemConfirm(subpack.ownerid, subpack.itemid);
}

void IkarusShopAuctionCreate(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGAuctionCreate>(data);
	ikashop::GetManager().RecvAuctionCreateDBPacket(subpack.auction);
}

void IkarusShopAuctionAddOffer(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGAuctionAddOffer>(data);
	ikashop::GetManager().RecvAuctionAddOfferDBPacket(subpack.offer);
}

void IkarusShopAuctionExpired(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGAuctionExpired>(data);
	ikashop::GetManager().RecvAuctionExpiredDBPacket(subpack.ownerid);
}

void IkarusShopAuctionOfferSeen(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGAuctionOfferSeen>(data);
	ikashop::GetManager().RecvAuctionOfferSeenDBPacket(subpack.ownerid, subpack.buyerid, subpack.price);
}

void IkarusShopShopExpired(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGShopExpired>(data);
	ikashop::GetManager().RecvShopExpiredDBPacket(subpack.ownerid);
}

#ifdef EXTEND_IKASHOP_PRO
void IkarusShopNotificationLoad(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGNotificationLoad>(data);
	// reserving notifications vector
	std::vector<ikashop::TNotificationInfo> notifications;
	notifications.reserve(subpack.count);
	// fetching vector
	for(DWORD i=0; i < subpack.count; i++)
		notifications.emplace_back(Decode<ikashop::TNotificationInfo>(data));
	// registering to manager
	ikashop::GetManager().RecvNotificationLoadDBPacket(subpack.ownerid, notifications);
}

void IkarusShopNotificationForward(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGNotificationForward>(data);
	ikashop::GetManager().RecvNotificationForwardDBPacket(subpack.ownerid, subpack);
}

void IkarusShopRestoreDuration(const char* data)
{
	const auto& owner = Decode<DWORD>(data);
	ikashop::GetManager().RecvShopRestoreDurationDBPacket(owner);
}
#ifdef ENABLE_IKASHOP_ENTITIES
void IkarusShopMoveShopEntity(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGMoveShopEntity>(data);
	ikashop::GetManager().RecvMoveShopEntityDBPacket(subpack.owner, subpack.spawn);
}
#endif
#endif

void IkarusShopOwnerOnline(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGOwnerOnline>(data);
	ikashop::GetManager().RecvShopOwnerOnlineDBPacket(subpack.owner, subpack.isOnline);
}

void IkarusShopLockedSlotState(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGLockedSlotState>(data);
	ikashop::GetManager().RecvShopLockedSlotStateDBPacket(subpack.owner, subpack.unlockCount, subpack.isPremium);
}

void IkarusShopLoadExpiredShop(const char* data)
{
	const auto& subpack = Decode<ikashop::TSubPacketDGLoadExpiredShop>(data);
	auto& manager = ikashop::GetManager();

	// decoding items
	std::vector<ikashop::TShopItem> items;
	items.reserve(subpack.itemCount);
	// fetching vector
	for (DWORD i = 0; i < subpack.itemCount; i++)
		items.emplace_back(Decode<ikashop::TShopItem>(data));

	// adding to shop
	auto shop = manager.GetShopByOwnerID(subpack.owner);
	if (shop)
	{
		for (const auto item : items)
			shop->AddItem(item);
	}
}


void IkarusShopPacket(const char* data)
{
	const auto& pack = Decode<TPacketDGNewIkarusShop>(data);

	switch (pack.bSubHeader)
	{
	case ikashop::SUBHEADER_DG_LOAD_TABLES:
		IkarusShopLoadTables(data);
		return;

	case ikashop::SUBHEADER_DG_BUY_ITEM:
		IkarusShopBuyItemPacket(data);
		return;

	case ikashop::SUBHEADER_DG_LOCKED_BUY_ITEM:
		IkarusShopLockedBuyItemPacket(data);
		return;

	case ikashop::SUBHEADER_DG_EDIT_ITEM:
		IkarusShopEditItemPacket(data);
		return;
	case ikashop::SUBHEADER_DG_REMOVE_ITEM:
		IkarusShopRemoveItemPacket(data);
		return;

	case ikashop::SUBHEADER_DG_ADD_ITEM:
		IkarusShopAddItemPacket(data);
		return;

	case ikashop::SUBHEADER_DG_SHOP_FORCE_CLOSE:
		IkarusShopForceClosePacket(data);
		return;

	case ikashop::SUBHEADER_DG_SHOP_CREATE_NEW:
		IkarusShopShopCreateNewPacket(data);
		return;

	case ikashop::SUBHEADER_DG_SHOP_CHANGE_NAME:
		IkarusShopShopChangeNamePacket(data);
		return;

	case ikashop::SUBHEADER_DG_SHOP_EXPIRED:
		IkarusShopShopExpired(data);
		break;

	case ikashop::SUBHEADER_DG_OFFER_CREATE:
		IkarusShopOfferCreatePacket(data);
		return;

	case ikashop::SUBHEADER_DG_OFFER_NOTIFIED:
		IkarusShopOfferNotifiedPacket(data);
		return;

	case ikashop::SUBHEADER_DG_OFFER_ACCEPT:
		IkarusShopOfferAcceptPacket(data);
		return;
	
	case ikashop::SUBHEADER_DG_OFFER_CANCEL:
		IkarusShopOfferCancelPacket(data);
		return;

	case ikashop::SUBHEADER_DG_SAFEBOX_ADD_ITEM:
		IkarusShopSafeboxAddItemPacket(data);
		return;

	case ikashop::SUBHEADER_DG_SAFEBOX_ADD_VALUTES:
		IkarusShopSafeboxAddValutesPacket(data);
		return;

	case ikashop::SUBHEADER_DG_SAFEBOX_LOAD:
		IkarusShopSafeboxLoad(data);
		return;

	case ikashop::SUBHEADER_DG_SAFEBOX_EXPIRED_ITEM:
		IkarusShopSafeboxExpiredItem(data);
		return;

	case ikashop::SUBHEADER_DG_SAFEBOX_GET_ITEM_CONFIRM:
		IkarusShopSafeboxGetItemConfirm(data);
		return;

	case ikashop::SUBHEADER_DG_AUCTION_CREATE:
		IkarusShopAuctionCreate(data);
		return;

	case ikashop::SUBHEADER_DG_AUCTION_ADD_OFFER:
		IkarusShopAuctionAddOffer(data);
		return;

	case ikashop::SUBHEADER_DG_AUCTION_EXPIRED:
		IkarusShopAuctionExpired(data);
		return;

	case ikashop::SUBHEADER_DG_AUCTION_OFFER_SEEN:
		IkarusShopAuctionOfferSeen(data);
		return;

#ifdef EXTEND_IKASHOP_PRO
	case ikashop::SUBHEADER_DG_NOTIFICATION_LOAD:
		IkarusShopNotificationLoad(data);
		return;

	case ikashop::SUBHEADER_DG_NOTIFICATION_FORWARD:
		IkarusShopNotificationForward(data);
		return;

	case ikashop::SUBHEADER_DG_SHOP_RESTORE_DURATION:
		IkarusShopRestoreDuration(data);
		return;
#ifdef ENABLE_IKASHOP_ENTITIES
	case ikashop::SUBHEADER_DG_MOVE_SHOP_ENTITY:
		IkarusShopMoveShopEntity(data);
		return;
#endif
#endif

	case ikashop::SUBHEADER_DG_SHOP_OWNER_ONLINE:
		IkarusShopOwnerOnline(data);
		return;

	case ikashop::SUBHEADER_DG_SHOP_LOCKED_SLOT_STATE:
		IkarusShopLockedSlotState(data);
		return;

	case ikashop::SUBHEADER_DG_LOAD_EXPIRED_SHOP:
		IkarusShopLoadExpiredShop(data);
		return;

	default:
		sys_err("UKNOWN SUB HEADER %d ", pack.bSubHeader);
		return;
	}
}
#endif
//END_RELOAD_ADMIN

////////////////////////////////////////////////////////////////////
// Analyze
////////////////////////////////////////////////////////////////////
int CInputDB::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	switch (bHeader)
	{
	case HEADER_DG_BOOT:
		Boot(c_pData);
		break;

	case HEADER_DG_LOGIN_SUCCESS:
		LoginSuccess(m_dwHandle, c_pData);
		break;

	case HEADER_DG_LOGIN_NOT_EXIST:
		LoginFailure(DESC_MANAGER::instance().FindByHandle(m_dwHandle), "NOID");
		break;

	case HEADER_DG_LOGIN_WRONG_PASSWD:
		LoginFailure(DESC_MANAGER::instance().FindByHandle(m_dwHandle), "WRONGPWD");
		break;

	case HEADER_DG_LOGIN_ALREADY:
		LoginAlready(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_PLAYER_LOAD_SUCCESS:
		PlayerLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_PLAYER_CREATE_SUCCESS:
		PlayerCreateSuccess(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_PLAYER_CREATE_FAILED:
		PlayerCreateFailure(DESC_MANAGER::instance().FindByHandle(m_dwHandle), 0);
		break;

	case HEADER_DG_PLAYER_CREATE_ALREADY:
		PlayerCreateFailure(DESC_MANAGER::instance().FindByHandle(m_dwHandle), 1);
		break;

	case HEADER_DG_PLAYER_DELETE_SUCCESS:
		PlayerDeleteSuccess(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_PLAYER_LOAD_FAILED:
		//sys_log(0, "PLAYER_LOAD_FAILED");
		if (DESC_MANAGER::instance().FindByHandle(m_dwHandle) &&
				DESC_MANAGER::instance().FindByHandle(m_dwHandle)->IsBot())
			CPlayerBotManager::instance().OnLoadFailed(m_dwHandle);
		break;

	case HEADER_DG_PLAYER_DELETE_FAILED:
		//sys_log(0, "PLAYER_DELETE_FAILED");
		PlayerDeleteFail(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_ITEM_LOAD:
		ItemLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_QUEST_LOAD:
		QuestLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_AFFECT_LOAD:
		AffectLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_SPECIAL_FLAG_LOAD:
		SpecialFlagLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_SAFEBOX_LOAD:
		SafeboxLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_RESPOND_MOVE_CHANNEL:
		MoveChannelRespond(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_SAFEBOX_CHANGE_SIZE:
		SafeboxChangeSize(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_SAFEBOX_WRONG_PASSWORD:
		SafeboxWrongPassword(DESC_MANAGER::instance().FindByHandle(m_dwHandle));
		break;

	case HEADER_DG_SAFEBOX_CHANGE_PASSWORD_ANSWER:
		SafeboxChangePasswordAnswer(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_MALL_LOAD:
		MallLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_EMPIRE_SELECT:
		EmpireSelect(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_MAP_LOCATIONS:
		MapLocations(c_pData);
		break;

	case HEADER_DG_P2P:
		P2P(c_pData);
		break;

	case HEADER_DG_GUILD_SKILL_UPDATE:
		GuildSkillUpdate(c_pData);
		break;

	case HEADER_DG_GUILD_LOAD:
		GuildLoad(c_pData);
		break;

	case HEADER_DG_GUILD_SKILL_RECHARGE:
		GuildSkillRecharge();
		break;

	case HEADER_DG_GUILD_EXP_UPDATE:
		GuildExpUpdate(c_pData);
		break;

	case HEADER_DG_PARTY_CREATE:
		PartyCreate(c_pData);
		break;

	case HEADER_DG_PARTY_DELETE:
		PartyDelete(c_pData);
		break;

	case HEADER_DG_PARTY_ADD:
		PartyAdd(c_pData);
		break;

	case HEADER_DG_PARTY_REMOVE:
		PartyRemove(c_pData);
		break;

	case HEADER_DG_PARTY_STATE_CHANGE:
		PartyStateChange(c_pData);
		break;

	case HEADER_DG_PARTY_SET_MEMBER_LEVEL:
		PartySetMemberLevel(c_pData);
		break;

	case HEADER_DG_TIME:
		Time(c_pData);
		break;

	case HEADER_DG_GUILD_ADD_MEMBER:
		GuildAddMember(c_pData);
		break;

	case HEADER_DG_GUILD_REMOVE_MEMBER:
		GuildRemoveMember(c_pData);
		break;

	case HEADER_DG_GUILD_CHANGE_GRADE:
		GuildChangeGrade(c_pData);
		break;

	case HEADER_DG_GUILD_CHANGE_MEMBER_DATA:
		GuildChangeMemberData(c_pData);
		break;

	case HEADER_DG_GUILD_DISBAND:
		GuildDisband(c_pData);
		break;

	case HEADER_DG_RELOAD_PROTO:
		ReloadProto(c_pData);
		break;

	case HEADER_DG_RELOAD_ITEMSHOP:
		ReloadItemShop(c_pData);
		break;

	case HEADER_DG_GUILD_WAR:
		GuildWar(c_pData);
		break;

	case HEADER_DG_GUILD_WAR_SCORE:
		GuildWarScore(c_pData);
		break;

	case HEADER_DG_GUILD_LADDER:
		GuildLadder(c_pData);
		break;

	case HEADER_DG_GUILD_SKILL_USABLE_CHANGE:
		GuildSkillUsableChange(c_pData);
		break;

	case HEADER_DG_CHANGE_NAME:
		ChangeName(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_AUTH_LOGIN:
		AuthLogin(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_CHANGE_EMPIRE_PRIV:
		ChangeEmpirePriv(c_pData);
		break;

	case HEADER_DG_CHANGE_GUILD_PRIV:
		ChangeGuildPriv(c_pData);
		break;

	case HEADER_DG_CHANGE_CHARACTER_PRIV:
		ChangeCharacterPriv(c_pData);
		break;

	case HEADER_DG_MONEY_LOG:
		MoneyLog(c_pData);
		break;

	case HEADER_DG_GUILD_WITHDRAW_MONEY_GIVE:
		GuildWithdrawMoney(c_pData);
		break;

	case HEADER_DG_GUILD_MONEY_CHANGE:
		GuildMoneyChange(c_pData);
		break;

	case HEADER_DG_SET_EVENT_FLAG:
		SetEventFlag(c_pData);
		break;

	case HEADER_DG_CREATE_OBJECT:
		CreateObject(c_pData);
		break;

	case HEADER_DG_DELETE_OBJECT:
		DeleteObject(c_pData);
		break;

	case HEADER_DG_UPDATE_LAND:
		UpdateLand(c_pData);
		break;

	case HEADER_DG_NOTICE:
		Notice(c_pData);
		break;

	case HEADER_DG_LOCALE_NOTICE:
		LocaleNotice(c_pData);
		break;

	case HEADER_DG_GUILD_WAR_NOTICE:
		GuildWarNotice((TPacketDGGuildWarNotice*)c_pData);
		break;

	case HEADER_DG_GUILD_WAR_RESERVE_ADD:
		GuildWarReserveAdd((TGuildWarReserve *) c_pData);
		break;

	case HEADER_DG_GUILD_WAR_RESERVE_DEL:
		GuildWarReserveDelete(*(DWORD *) c_pData);
		break;

	case HEADER_DG_GUILD_WAR_BET:
		GuildWarBet((TPacketGDGuildWarBet *) c_pData);
		break;

	case HEADER_DG_MARRIAGE_ADD:
		MarriageAdd((TPacketMarriageAdd*) c_pData);
		break;

	case HEADER_DG_MARRIAGE_UPDATE:
		MarriageUpdate((TPacketMarriageUpdate*) c_pData);
		break;

	case HEADER_DG_MARRIAGE_REMOVE:
		MarriageRemove((TPacketMarriageRemove*) c_pData);
		break;

	case HEADER_DG_WEDDING_REQUEST:
		WeddingRequest((TPacketWeddingRequest*) c_pData);
		break;

	case HEADER_DG_WEDDING_READY:
		WeddingReady((TPacketWeddingReady*) c_pData);
		break;

	case HEADER_DG_WEDDING_START:
		WeddingStart((TPacketWeddingStart*) c_pData);
		break;

	case HEADER_DG_WEDDING_END:
		WeddingEnd((TPacketWeddingEnd*) c_pData);
		break;

		// MYSHOP_PRICE_LIST
	case HEADER_DG_MYSHOP_PRICELIST_RES:
		MyshopPricelistRes(DESC_MANAGER::instance().FindByHandle(m_dwHandle), (TPacketMyshopPricelistHeader*) c_pData );
		break;
		// END_OF_MYSHOP_PRICE_LIST

	// RELOAD_ADMIN
	case HEADER_DG_RELOAD_ADMIN:
		ReloadAdmin(c_pData );
		break;
	//END_RELOAD_ADMIN

	case HEADER_DG_ADD_MONARCH_MONEY:
		AddMonarchMoney(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData );
		break;

	case HEADER_DG_DEC_MONARCH_MONEY:
		DecMonarchMoney(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData );
		break;

	case HEADER_DG_TAKE_MONARCH_MONEY:
		TakeMonarchMoney(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData );
		break;

	case HEADER_DG_CHANGE_MONARCH_LORD_ACK :
		ChangeMonarchLord((TPacketChangeMonarchLordACK*)c_pData);
		break;

	case HEADER_DG_UPDATE_MONARCH_INFO :
		UpdateMonarchInfo((TMonarchInfo*)c_pData);
		break;

	case HEADER_DG_ACK_CHANGE_GUILD_MASTER :
		this->GuildChangeMaster((TPacketChangeGuildMaster*) c_pData);
		break;
	case HEADER_DG_ACK_SPARE_ITEM_ID_RANGE :
		ITEM_MANAGER::instance().SetMaxSpareItemID(*((TItemIDRangeTable*)c_pData) );
		break;

	case HEADER_DG_UPDATE_HORSE_NAME :
	case HEADER_DG_ACK_HORSE_NAME :
		CHorseNameManager::instance().UpdateHorseName(
				((TPacketUpdateHorseName*)c_pData)->dwPlayerID,
				((TPacketUpdateHorseName*)c_pData)->szHorseName);
		break;

	case HEADER_DG_NEED_LOGIN_LOG:
		DetailLog( (TPacketNeedLoginLogInfo*) c_pData );
		break;

	case HEADER_DG_ITEMAWARD_INFORMER:
		ItemAwardInformer((TPacketItemAwardInfromer*) c_pData);
		break;
	case HEADER_DG_RESPOND_CHANNELSTATUS:
		RespondChannelStatus(DESC_MANAGER::instance().FindByHandle(m_dwHandle), c_pData);
		break;
#ifdef ENABLE_IKASHOP_RENEWAL
	case HEADER_DG_NEW_OFFLINESHOP:
		IkarusShopPacket(c_pData);
		break;
#endif

	case HEADER_DG_SPECIAL_SHOP_SEED:
	{
		RecvSpecialShopSeedPacket((TPacketDGSpecialShopSeed*)c_pData);
		break;
	}

	case HEADER_DG_USAGE:
		RecvUsage((TPacketDGUsage*)c_pData);
		break;

	case HEADER_DG_USE_CODE_VOUCHER:
		RecvUseCodeVoucher((TRequestUseCodeVoucherResponse*)c_pData);
		break;
	default:
		return (-1);
	}

	return 0;
}

bool CInputDB::Process(LPDESC d, const void * orig, int bytes, int & r_iBytesProceed)
{
	const char *	c_pData = (const char *) orig;
	BYTE		bHeader, bLastHeader = 0;
	int			iSize;
	int			iLastPacketLen = 0;

	for (m_iBufferLeft = bytes; m_iBufferLeft > 0;)
	{
		if (m_iBufferLeft < 9)
			return true;

		bHeader		= *((BYTE *) (c_pData));	// 1
		m_dwHandle	= *((DWORD *) (c_pData + 1));	// 4
		iSize		= *((DWORD *) (c_pData + 5));	// 4

		sys_log(1, "DBCLIENT: header %d handle %d size %d bytes %d", bHeader, m_dwHandle, iSize, bytes);

		if (m_iBufferLeft - 9 < iSize)
			return true;

		const char * pRealData = (c_pData + 9);

		if (Analyze(d, bHeader, pRealData) < 0)
		{
			sys_err("in InputDB: UNKNOWN HEADER: %d, LAST HEADER: %d(%d), REMAIN BYTES: %d, DESC: %d",
					bHeader, bLastHeader, iLastPacketLen, m_iBufferLeft, d->GetSocket());

			//printdata((BYTE*) orig, bytes);
			//d->SetPhase(PHASE_CLOSE);
		}

		c_pData		+= 9 + iSize;
		m_iBufferLeft	-= 9 + iSize;
		r_iBytesProceed	+= 9 + iSize;

		iLastPacketLen	= 9 + iSize;
		bLastHeader	= bHeader;
	}

	return true;
}

void CInputDB::AddMonarchMoney(LPDESC d, const char * data )
{
	int Empire = *(int *) data;
	data += sizeof(int);

	int Money = *(int *) data;
	data += sizeof(int);

	CMonarch::instance().AddMoney(Money, Empire);

	DWORD pid = CMonarch::instance().GetMonarchPID(Empire);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pid);

	if (ch)
	{
		if (number(1, 100) > 95)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("현재 %s 국고에는 %u 의 돈이 있습니다"), EMPIRE_NAME(Empire), CMonarch::instance().GetMoney(Empire));
	}
}

void CInputDB::DecMonarchMoney(LPDESC d, const char * data)
{
	int Empire = *(int *) data;
	data += sizeof(int);

	int Money = *(int *) data;
	data += sizeof(int);

	CMonarch::instance().DecMoney(Money, Empire);

	DWORD pid = CMonarch::instance().GetMonarchPID(Empire);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pid);

	if (ch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s still has %d Yang available."), EMPIRE_NAME(Empire), CMonarch::instance().GetMoney(Empire));
	}
}

void CInputDB::TakeMonarchMoney(LPDESC d, const char * data)
{
	int Empire = *(int *) data;
	data += sizeof(int);

	int Money = *(int *) data;
	data += sizeof(int);

	if (!CMonarch::instance().DecMoney(Money, Empire))
	{
		if (!d)
			return;

		if (!d->GetCharacter())
			return;

		LPCHARACTER ch = d->GetCharacter();
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have enough Yang."));
	}
}

void CInputDB::ChangeMonarchLord(TPacketChangeMonarchLordACK* info)
{
	char notice[256];
	snprintf(notice, sizeof(notice), LC_TEXT("The emperor of %s has changed to %s."), EMPIRE_NAME(info->bEmpire), info->szName);
	SendNotice(notice);
}

void CInputDB::UpdateMonarchInfo(TMonarchInfo* info)
{
	CMonarch::instance().SetMonarchInfo(info);
	sys_log(0, "MONARCH INFO UPDATED");
}

void CInputDB::GuildChangeMaster(TPacketChangeGuildMaster* p)
{
	CGuildManager::instance().ChangeMaster(p->dwGuildID);
}

void CInputDB::DetailLog(const TPacketNeedLoginLogInfo* info)
{
	LPCHARACTER pChar = CHARACTER_MANAGER::instance().FindByPID( info->dwPlayerID );

	if (NULL != pChar)
	{
		LogManager::instance().DetailLoginLog(true, pChar);
	}
}

void CInputDB::ItemAwardInformer(TPacketItemAwardInfromer *data)
{
	LPDESC d = DESC_MANAGER::instance().FindByLoginName(data->login);

	if(d == NULL)
		return;
	else
	{
		if (d->GetCharacter())
		{
			LPCHARACTER ch = d->GetCharacter();
			ch->SetItemAward_vnum(data->vnum);
			ch->SetItemAward_cmd(data->command);

			if(d->IsPhase(PHASE_GAME))
			{
				quest::CQuestManager::instance().ItemInformer(ch->GetPlayerID(),ch->GetItemAward_vnum());
			}
		}
	}
}

#ifdef ENABLE_CHANNEL_STATUS_CACHE
std::vector<char> cachedChannelStatus;
void WriteBytesToVector(std::vector<char>& buffer, const void* data, size_t size)
{
	const char* charData = reinterpret_cast<const char*>(data);
	buffer.insert(buffer.end(), charData, charData + size);
}
#endif
void CInputDB::RespondChannelStatus(LPDESC desc, const char* pcData)
{
	if (!desc) {
		return;
	}
	const int nSize = decode_4bytes(pcData);
	pcData += sizeof(nSize);

	BYTE bHeader = HEADER_GC_RESPOND_CHANNELSTATUS;
	const BYTE bSuccess = 1;
#ifdef ENABLE_CHANNEL_STATUS_CACHE
	cachedChannelStatus.clear();
	WriteBytesToVector(cachedChannelStatus, &bHeader, sizeof(BYTE));
	WriteBytesToVector(cachedChannelStatus, &nSize, sizeof(nSize));

	if (0 < nSize)
		WriteBytesToVector(cachedChannelStatus, pcData, sizeof(TChannelStatus) * nSize);
	WriteBytesToVector(cachedChannelStatus, &bSuccess, sizeof(bSuccess));
	desc->Packet(cachedChannelStatus.data(), cachedChannelStatus.size());
#else
	desc->BufferedPacket(&bHeader, sizeof(BYTE));
	desc->BufferedPacket(&nSize, sizeof(nSize));
	if (0 < nSize)
		desc->BufferedPacket(pcData, sizeof(TChannelStatus) * nSize);
	desc->Packet(&bSuccess, sizeof(bSuccess));
#endif

	desc->SetChannelStatusRequested(false);
}

void CInputDB::RecvSpecialShopSeedPacket(TPacketDGSpecialShopSeed* data)
{
	CSpecialShopManager::instance().SetShopMeta(data->shopVnum, data->meta);
}
void CInputDB::RecvUsage(TPacketDGUsage* pcData)
{
	if (LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pcData->invokerPid))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "### SERVER USAGE ###");
		ch->ChatPacket(CHAT_TYPE_INFO, "Current: %d", pcData->usageCur);
		ch->ChatPacket(CHAT_TYPE_INFO, "Average (last hour): %d", pcData->usageAvg);
		ch->ChatPacket(CHAT_TYPE_INFO, "Max (last hour): %d", pcData->usageMax);
	}
}
void CInputDB::RecvUseCodeVoucher(TRequestUseCodeVoucherResponse* pcData)
{
	if (LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pcData->dwPID))
	{
		CItemShopManager::instance().RecvUseCodeVoucher(ch, pcData->bResponse, pcData->szCode);
	}
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
