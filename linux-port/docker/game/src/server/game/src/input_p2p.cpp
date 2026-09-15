#include "stdafx.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "party.h"
#include "messenger_manager.h"
#include "empire_text_convert.h"
#include "unique_item.h"
#include "xmas_event.h"
#include "affect.h"
#include "castle.h"
#include "locale_service.h"
#include "questmanager.h"
#include "skill.h"
#include "threeway_war.h"
#include "dungeon.h"
#include "itemshop_manager.h"
#include "ikarus_shop_manager.h"
#include "maintenance.h"
#include "BanManager.h"

template <class T>
const T& Decode(const char*& data) {
	auto obj = reinterpret_cast<const T*>(data);
	data += sizeof(T);
	return *obj;
}

////////////////////////////////////////////////////////////////////////////////
// Input Processor
CInputP2P::CInputP2P()
{
	BindPacketInfo(&m_packetInfoGG);
}

void CInputP2P::Login(LPDESC d, const char * c_pData)
{
	P2P_MANAGER::instance().Login(d, (TPacketGGLogin *) c_pData);
}

void CInputP2P::Logout(LPDESC d, const char * c_pData)
{
	TPacketGGLogout * p = (TPacketGGLogout *) c_pData;
	P2P_MANAGER::instance().Logout(p->szName);
}

int CInputP2P::Relay(LPDESC d, const char * c_pData, size_t uiBytes)
{
	TPacketGGRelay * p = (TPacketGGRelay *) c_pData;

	if (uiBytes < sizeof(TPacketGGRelay) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	sys_log(0, "InputP2P::Relay : %s size %d", p->szName, p->lSize);

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(p->szName);

	const BYTE* c_pbData = (const BYTE *) (c_pData + sizeof(TPacketGGRelay));

	if (!pkChr)
		return p->lSize;

	if (*c_pbData == HEADER_GC_WHISPER)
	{
		if (pkChr->IsBlockMode(BLOCK_WHISPER))
		{
			return p->lSize;
		}

		char buf[1024];
		memcpy(buf, c_pbData, MIN(p->lSize, sizeof(buf)));

		TPacketGCWhisper* p2 = (TPacketGCWhisper*) buf;

		BYTE bToEmpire = (p2->bType >> 4);
		p2->bType = p2->bType & 0x0F;
		if(p2->bType == 0x0F) {
			p2->bType = WHISPER_TYPE_SYSTEM;
		} else {
			if (!pkChr->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
				if (bToEmpire >= 1 && bToEmpire <= 3 && pkChr->GetEmpire() != bToEmpire)
				{
					ConvertEmpireText(bToEmpire,
							buf + sizeof(TPacketGCWhisper),
							p2->wSize - sizeof(TPacketGCWhisper),
							10+2*pkChr->GetSkillPower(SKILL_LANGUAGE1 + bToEmpire - 1));
				}
		}

		pkChr->GetDesc()->Packet(buf, p->lSize);
	}
	else
		pkChr->GetDesc()->Packet(c_pbData, p->lSize);

	return (p->lSize);
}

#ifdef ENABLE_FULL_NOTICE
int CInputP2P::Notice(LPDESC d, const char * c_pData, size_t uiBytes, bool bBigFont)
#else
int CInputP2P::Notice(LPDESC d, const char * c_pData, size_t uiBytes)
#endif
{
	TPacketGGNotice * p = (TPacketGGNotice *) c_pData;

	if (uiBytes < sizeof(TPacketGGNotice) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[256+1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGNotice), MIN(p->lSize + 1, sizeof(szBuf)));
#ifdef ENABLE_FULL_NOTICE
	SendNotice(szBuf, p->bChatType);
#else
	SendNotice(szBuf);
#endif
	return (p->lSize);
}

int CInputP2P::MonarchNotice(LPDESC d, const char * c_pData, size_t uiBytes)
{
	TPacketGGMonarchNotice * p = (TPacketGGMonarchNotice *) c_pData;

	if (uiBytes < p->lSize + sizeof(TPacketGGMonarchNotice))
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[256+1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGMonarchNotice), MIN(p->lSize + 1, sizeof(szBuf)));
	SendMonarchNotice(p->bEmpire, szBuf);
	return (p->lSize);
}

int CInputP2P::MonarchTransfer(LPDESC d, const char* c_pData)
{
	TPacketMonarchGGTransfer* p = (TPacketMonarchGGTransfer*) c_pData;
	LPCHARACTER pTargetChar = CHARACTER_MANAGER::instance().FindByPID(p->dwTargetPID);

	if (pTargetChar != NULL)
	{
		unsigned int qIndex = quest::CQuestManager::instance().GetQuestIndexByName("monarch_transfer");

		if (qIndex != 0)
		{
			pTargetChar->SetQuestFlag("monarch_transfer.x", p->x);
			pTargetChar->SetQuestFlag("monarch_transfer.y", p->y);
			quest::CQuestManager::instance().Letter(pTargetChar->GetPlayerID(), qIndex, 0);
		}
	}

	return 0;
}

int CInputP2P::Guild(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGGuild * p = (TPacketGGGuild *) c_pData;
	uiBytes -= sizeof(TPacketGGGuild);
	c_pData += sizeof(TPacketGGGuild);

	CGuild * g = CGuildManager::instance().FindGuild(p->dwGuild);

	switch (p->bSubHeader)
	{
		case GUILD_SUBHEADER_GG_CHAT:
			{
				if (uiBytes < sizeof(TPacketGGGuildChat))
					return -1;

				TPacketGGGuildChat * p = (TPacketGGGuildChat *) c_pData;

				if (g)
					g->P2PChat(p->szText);

				return sizeof(TPacketGGGuildChat);
			}

		case GUILD_SUBHEADER_GG_SET_MEMBER_COUNT_BONUS:
			{
				if (uiBytes < sizeof(int))
					return -1;

				int iBonus = *((int *) c_pData);
				CGuild* pGuild = CGuildManager::instance().FindGuild(p->dwGuild);
				if (pGuild)
				{
					pGuild->SetMemberCountBonus(iBonus);
				}
				return sizeof(int);
			}
		default:
			sys_err ("UNKNOWN GUILD SUB PACKET");
			break;
	}
	return 0;
}

struct FuncShout
{
	const char * m_str;
	BYTE m_bEmpire;
	BYTE m_bChatType;

	FuncShout(const char* str, BYTE bEmpire, BYTE bChatType) : m_str(str), m_bEmpire(bEmpire), m_bChatType(bChatType)
	{
	}

	void operator () (LPDESC d)
	{
		if (!d)
			return;

		if (!d->GetCharacter() || (m_bChatType == CHAT_TYPE_SHOUT && d->GetCharacter()->GetGMLevel() == GM_PLAYER && d->GetEmpire() != m_bEmpire))
			return;

		d->GetCharacter()->ChatPacket(m_bChatType, "%s", m_str);
	}
};

void SendTrade(const char* szText)
{
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), FuncShout(szText, 0, CHAT_TYPE_TRADE));
}

void SendShout(const char * szText, BYTE bEmpire)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), FuncShout(szText, bEmpire, CHAT_TYPE_SHOUT));
}

void CInputP2P::Shout(const char * c_pData)
{
	TPacketGGShout * p = (TPacketGGShout *) c_pData;
	if (p->bChatType == CHAT_TYPE_SHOUT)
		SendShout(p->szText, p->bEmpire);
	else
		SendTrade(p->szText);
}

void CInputP2P::Disconnect(const char * c_pData)
{
	TPacketGGDisconnect * p = (TPacketGGDisconnect *) c_pData;

	LPDESC d = DESC_MANAGER::instance().FindByLoginName(p->szLogin);

	if (!d)
		return;

	if (!d->GetCharacter())
	{
		d->SetPhase(PHASE_CLOSE);
	}
	else
		d->DisconnectOfSameLogin();
}

void CInputP2P::Setup(LPDESC d, const char * c_pData)
{
	TPacketGGSetup * p = (TPacketGGSetup *) c_pData;
	sys_log(0, "P2P: Setup %s:%d", d->GetHostName(), p->wPort);
	d->SetP2P(d->GetHostName(), p->wPort, p->bChannel);
}

void CInputP2P::MessengerAdd(const char * c_pData)
{
	TPacketGGMessenger * p = (TPacketGGMessenger *) c_pData;
	sys_log(0, "P2P: Messenger Add %s %s", p->szAccount, p->szCompanion);
	MessengerManager::instance().__AddToList(p->szAccount, p->szCompanion);
}

void CInputP2P::MessengerRemove(const char * c_pData)
{
	TPacketGGMessenger * p = (TPacketGGMessenger *) c_pData;
	sys_log(0, "P2P: Messenger Remove %s %s", p->szAccount, p->szCompanion);
	MessengerManager::instance().__RemoveFromList(p->szAccount, p->szCompanion);
}

void CInputP2P::FindPosition(LPDESC d, const char* c_pData)
{
	TPacketGGFindPosition* p = (TPacketGGFindPosition*) c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->dwTargetPID);

	if (ch)
	{
		TPacketGGWarpCharacter pw;
		pw.header = HEADER_GG_WARP_CHARACTER;
		pw.pid = p->dwFromPID;
		pw.x = ch->GetX();
		pw.y = ch->GetY();
		pw.iTargetChannel = g_bChannel;
#ifdef ENABLE_CMD_WARP_IN_DUNGEON
		pw.mapIndex = (ch->GetMapIndex() >= 10000 && !p->canWarpDungeon) ? 0 : ch->GetMapIndex();
#endif

		if (!p->canWarpDungeon) // marriage teleport item for non gm
		{
			if (ch->GetMapIndex() == 66) // summon at DT entrance
			{
				pw.mapIndex = 65;
				pw.x = 590800;
				pw.y = 111200;
			}
		}

		d->Packet(&pw, sizeof(pw));
	}
}

void CInputP2P::WarpCharacter(const char* c_pData)
{
	TPacketGGWarpCharacter* p = (TPacketGGWarpCharacter*) c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->pid);
	if (ch && ch->GetDesc())
	{
		if (p->mapIndex == 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot warp to this person."));
			return;
		}

		if (p->iTargetChannel != 99 && p->iTargetChannel != g_bChannel)
		{
			TMoveChannel t{ ch->GetDesc()->GetAccountTable().id, (BYTE)p->iTargetChannel, p->x, p->y, p->mapIndex };
			db_clientdesc->DBPacket(HEADER_GD_MOVE_CHANNEL, ch->GetDesc()->GetHandle(), &t, sizeof(t));
		}
		else
		{
#ifdef ENABLE_CMD_WARP_IN_DUNGEON
			ch->WarpSet(p->x, p->y, p->mapIndex);
#else
			ch->WarpSet(p->x, p->y);
#endif
		}
	}
}

void CInputP2P::GuildWarZoneMapIndex(const char* c_pData)
{
	TPacketGGGuildWarMapIndex * p = (TPacketGGGuildWarMapIndex*) c_pData;
	CGuildManager & gm = CGuildManager::instance();

	sys_log(0, "P2P: GuildWarZoneMapIndex g1(%u) vs g2(%u), mapIndex(%d)", p->dwGuildID1, p->dwGuildID2, p->lMapIndex);

	CGuild * g1 = gm.FindGuild(p->dwGuildID1);
	CGuild * g2 = gm.FindGuild(p->dwGuildID2);

	if (g1 && g2)
	{
		g1->SetGuildWarMapIndex(p->dwGuildID2, p->lMapIndex);
		g2->SetGuildWarMapIndex(p->dwGuildID1, p->lMapIndex);
	}
}

void CInputP2P::Transfer(const char * c_pData)
{
	TPacketGGTransfer * p = (TPacketGGTransfer *) c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szName);

	if (ch)
		ch->WarpSet(p->lX, p->lY);
}

void CInputP2P::XmasWarpSanta(const char * c_pData)
{
	TPacketGGXmasWarpSanta * p =(TPacketGGXmasWarpSanta *) c_pData;

	if (p->bChannel == g_bChannel && map_allow_find(p->lMapIndex))
	{
		int	iNextSpawnDelay = 50 * 60;

		xmas::SpawnSanta(p->lMapIndex, iNextSpawnDelay);

		TPacketGGXmasWarpSantaReply pack_reply;
		pack_reply.bHeader = HEADER_GG_XMAS_WARP_SANTA_REPLY;
		pack_reply.bChannel = g_bChannel;
		P2P_MANAGER::instance().Send(&pack_reply, sizeof(pack_reply));
	}
}

void CInputP2P::XmasWarpSantaReply(const char* c_pData)
{
	TPacketGGXmasWarpSantaReply* p = (TPacketGGXmasWarpSantaReply*) c_pData;

	if (p->bChannel == g_bChannel)
	{
		CharacterVectorInteractor i;

		if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(xmas::MOB_SANTA_VNUM, i))
		{
			CharacterVectorInteractor::iterator it = i.begin();

			while (it != i.end()) {
				M2_DESTROY_CHARACTER(*it++);
			}
		}
	}
}

void CInputP2P::LoginPing(LPDESC d, const char * c_pData)
{
	TPacketGGLoginPing * p = (TPacketGGLoginPing *) c_pData;

	if (!g_pkAuthMasterDesc) // If I am master, I have to broadcast
		P2P_MANAGER::instance().Send(p, sizeof(TPacketGGLoginPing), d);
}

void CInputP2P::DisconnectPlayer(const char* c_pData)
{
	TPacketGGDisconnectPlayer* p = (TPacketGGDisconnectPlayer*)c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szName);

	if (ch && ch->GetDesc())
	{
		LPDESC d = ch->GetDesc();
		sys_log(0, "DISCONNECT PLAYER success name %s", p->szName);
		DESC_MANAGER::instance().DestroyLoginKey(d);
		d->SetPhase(PHASE_CLOSE);
	}
	else
	{
		sys_log(0, "DISCONNECT PLAYER fail name %s", p->szName);
	}
}

void CInputP2P::DisconnectPlayerId(const char* c_pData)
{
	TPacketGGDisconnectPlayerId* p = (TPacketGGDisconnectPlayerId*)c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->pid);

	if (ch && ch->GetDesc())
	{
		LPDESC d = ch->GetDesc();
		sys_log(0, "DISCONNECT PLAYER success PID %d ", p->pid);
		DESC_MANAGER::instance().DestroyLoginKey(d);
		d->SetPhase(PHASE_CLOSE);
	}
	else
	{
		sys_log(0, "DISCONNECT PLAYER fail PID %d ", p->pid);
	}
}

void CInputP2P::DisconnectHwid(const char* c_pData)
{
	TPacketGGDisconnectHwid* p = (TPacketGGDisconnectHwid*)c_pData;
	BanManager::instance().DisconnectHwid(p->hwid);
}

// BLOCK_CHAT
void CInputP2P::BlockChat(const char * c_pData)
{
	TPacketGGBlockChat * p = (TPacketGGBlockChat *) c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szName);

	if (ch)
	{
		sys_log(0, "BLOCK CHAT apply name %s dur %d", p->szName, p->lBlockDuration);
		ch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, p->lBlockDuration, 0, true);
	}
	else
	{
		sys_log(0, "BLOCK CHAT fail name %s dur %d", p->szName, p->lBlockDuration);
	}
}
// END_OF_BLOCK_CHAT
//

void CInputP2P::ItemShopAuctionUpdate(const char* c_pData)
{
	TPacketGGItemShopAuctionUpdate* p = (TPacketGGItemShopAuctionUpdate*)c_pData;
	CItemShopManager::instance().UpdateAuctionBuyAmount(p->dwItemIndex, p->dwBuyAmount);
}

void CInputP2P::DungeonFind(const char* c_pData)
{
	TPacketGGDungeonFind* p = (TPacketGGDungeonFind*)c_pData;
	if (p->iChannel != g_bChannel)
		return;

	LPDUNGEON pDungeon = CDungeonManager::instance().FindByMapIndex(p->lMapIndex);
	if (pDungeon && !pDungeon->IsToBeDestroyed())
	{
		TPacketGGWarpCharacter warpPacket;
		warpPacket.header = HEADER_GG_WARP_CHARACTER;
		warpPacket.pid = p->dwPID;
		warpPacket.x = p->lPosX;
		warpPacket.y = p->lPosY;
		warpPacket.mapIndex = p->lMapIndex;
		warpPacket.iTargetChannel = p->iChannel;
		P2P_MANAGER::instance().Send(&warpPacket, sizeof(TPacketGGWarpCharacter));
	}
}

void CInputP2P::Maintenance(const char* c_pData)
{
	TPacketGGMaintenance* p = (TPacketGGMaintenance*)c_pData;
	if (p->bState)
	{
		g_bIsMaintenance = true;
	}
	else
	{
		g_bIsMaintenance = false;
	}
}

void CInputP2P::DelayedShutdown(const char* c_pData)
{
	TPacketGGDelayedShutdown* p = (TPacketGGDelayedShutdown*)c_pData;
	if (p->dwTimeUntil > 0)
	{
		CMaintenance::instance().StartMaintenance(p->dwTimeUntil, p->dwDuration, false);
	}
	else
	{
		CMaintenance::instance().StopMaintenance();
	}
}

int CInputP2P::BlockMessenger(const char* c_pData)
{
	const auto& p = Decode<TPacketGGBlockMessenger>(c_pData);

	switch (p.subheader)
	{
	case SUBHEADER_BLOCK_MESSENGER_BLOCK:
		{
			const auto& pack = Decode<TSubPacketGGBlockMessengerBlock>(c_pData);
			MessengerManager::instance().__BlockP2P(pack.pid, pack.name);
			return sizeof(TSubPacketGGBlockMessengerBlock);
		}
	case SUBHEADER_BLOCK_MESSENGER_UNBLOCK:
		{
			const auto& pack = Decode<TSubPacketGGBlockMessengerBlock>(c_pData);
			MessengerManager::instance().__UnblockP2P(pack.pid, pack.name);
			return sizeof(TSubPacketGGBlockMessengerBlock);
		}
	case SUBHEADER_BLOCK_MESSENGER_LOAD:
		{
			const auto& pack = Decode<TSubPacketGGBlockMessengerLoad>(c_pData);

			for (int i = 0; i < pack.itemCount; i++)
			{
				const auto& item = Decode<TSubPacketGGBlockMessengerLoadItem>(c_pData);
				MessengerManager::instance().__InsertBlockListP2P(pack.pid, item.name);
			}
			return sizeof(TSubPacketGGBlockMessengerLoad) + sizeof(TSubPacketGGBlockMessengerLoadItem) * pack.itemCount;
		}
	default:
		break;
	}
	return 0;
}

void CInputP2P::IamAwake(LPDESC d, const char * c_pData)
{
	std::string hostNames;
	P2P_MANAGER::instance().GetP2PHostNames(hostNames);
	sys_log(0, "P2P Awakeness check from %s. My P2P connection number is %d. and details...\n%s", d->GetHostName(), P2P_MANAGER::instance().GetDescCount(), hostNames.c_str());
}

int CInputP2P::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	if (test_server)
		sys_log(0, "CInputP2P::Analyze[Header %d]", bHeader);

	int iExtraLen = 0;

	switch (bHeader)
	{
		case HEADER_GG_SETUP:
			Setup(d, c_pData);
			break;

		case HEADER_GG_LOGIN:
			Login(d, c_pData);
			break;

		case HEADER_GG_LOGOUT:
			Logout(d, c_pData);
			break;

		case HEADER_GG_RELAY:
			if ((iExtraLen = Relay(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
#ifdef ENABLE_FULL_NOTICE
		case HEADER_GG_BIG_NOTICE:
			if ((iExtraLen = Notice(d, c_pData, m_iBufferLeft, true)) < 0)
				return -1;
			break;
#endif
		case HEADER_GG_NOTICE:
			if ((iExtraLen = Notice(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_GG_SHUTDOWN:
			sys_err("Accept shutdown p2p command from %s.", d->GetHostName());
			Shutdown(10);
			break;

		case HEADER_GG_GUILD:
			if ((iExtraLen = Guild(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_GG_SHOUT:
			Shout(c_pData);
			break;

		case HEADER_GG_DISCONNECT:
			Disconnect(c_pData);
			break;

		case HEADER_GG_MESSENGER_ADD:
			MessengerAdd(c_pData);
			break;

		case HEADER_GG_MESSENGER_REMOVE:
			MessengerRemove(c_pData);
			break;

		case HEADER_GG_FIND_POSITION:
			FindPosition(d, c_pData);
			break;

		case HEADER_GG_WARP_CHARACTER:
			WarpCharacter(c_pData);
			break;

		case HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX:
			GuildWarZoneMapIndex(c_pData);
			break;

		case HEADER_GG_TRANSFER:
			Transfer(c_pData);
			break;

		case HEADER_GG_XMAS_WARP_SANTA:
			XmasWarpSanta(c_pData);
			break;

		case HEADER_GG_XMAS_WARP_SANTA_REPLY:
			XmasWarpSantaReply(c_pData);
			break;

		case HEADER_GG_RELOAD_CRC_LIST:
			LoadValidCRCList();
			break;

		case HEADER_GG_LOGIN_PING:
			LoginPing(d, c_pData);
			break;

		case HEADER_GG_BLOCK_CHAT:
			BlockChat(c_pData);
			break;

		case HEADER_GG_DISCONNECT_PLAYER:
			DisconnectPlayer(c_pData);
			break;

		case HEADER_GG_DISCONNECT_PLAYER_ID:
			DisconnectPlayerId(c_pData);
			break;

		case HEADER_GG_DISCONNECT_HWID:
			DisconnectHwid(c_pData);
			break;

		case HEADER_GG_SIEGE:
			{
				TPacketGGSiege* pSiege = (TPacketGGSiege*)c_pData;
				castle_siege(pSiege->bEmpire, pSiege->bTowerCount);
			}
			break;

		case HEADER_GG_MONARCH_NOTICE:
			if ((iExtraLen = MonarchNotice(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_GG_MONARCH_TRANSFER :
			MonarchTransfer(d, c_pData);
			break;

		case HEADER_GG_CHECK_AWAKENESS:
			IamAwake(d, c_pData);
			break;

		case HEADER_GG_ITEMSHOP_AUCTION_UPDATE:
			ItemShopAuctionUpdate(c_pData);
			break;

		case HEADER_GG_FIND_DUNGEON:
			DungeonFind(c_pData);
			break;

		case HEADER_GG_MAINTENANCE:
			Maintenance(c_pData);
			break;

		case HEADER_GG_DELAYED_SHUTDOWN:
			DelayedShutdown(c_pData);
			break;

		case HEADER_GG_BLOCK_MESSENGER:
			if ((iExtraLen = BlockMessenger(c_pData)) < 0)
				return -1;
			break;

		case HEADER_GG_HANDSHAKE_VALIDATION:
			DESC_MANAGER::instance().AddToHandshakeWhiteList((const TPacketGGHandshakeValidate*)c_pData);
			break;
	}

	return (iExtraLen);
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
