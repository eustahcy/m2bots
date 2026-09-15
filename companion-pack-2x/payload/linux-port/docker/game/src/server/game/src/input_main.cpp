#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "utils.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "protocol.h"
#include "char.h"
#include "char_manager.h"
#include "playerbot_manager.h"
#include "item.h"
#include "item_manager.h"
#include "cmd.h"
#include "shop.h"
#include "shop_manager.h"
#include "safebox.h"
#include "regen.h"
#include "battle.h"
#include "exchange.h"
#include "questmanager.h"
#include "profiler.h"
#include "messenger_manager.h"
#include "party.h"
#include "p2p.h"
#include "affect.h"
#include "guild.h"
#include "guild_manager.h"
#include "log.h"
#include "banword.h"
#include "empire_text_convert.h"
#include "unique_item.h"
#include "building.h"
#include "locale_service.h"
#include "gm.h"
#include "spam.h"
#include "ani.h"
#include "motion.h"
#include "OXEvent.h"
#include "locale_service.h"
#include "DragonSoul.h"
#include "belt_inventory_helper.h" // @fixme119
#include "../../common/CommonDefines.h"
#include "../../common/PulseManager.h"

#include "input.h"

#define ENABLE_CHAT_COLOR_SYSTEM
#define ENABLE_CHAT_LOGGING
#define ENABLE_CHAT_SPAMLIMIT
#define ENABLE_WHISPER_CHAT_SPAMLIMIT
#define ENABLE_CHECK_GHOSTMODE

#ifdef ENABLE_CHAT_LOGGING
static char	__escape_string[1024];
static char	__escape_string2[1024];
#endif

static int __deposit_limit()
{
	return (1000*10000);
}

void SendBlockChatInfo(LPCHARACTER ch, int sec)
{
	if (sec <= 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your chat is blocked."));
		return;
	}

	long hour = sec / 3600;
	sec -= hour * 3600;

	long min = (sec / 60);
	sec -= min * 60;

	char buf[128+1];

	if (hour > 0 && min > 0)
		snprintf(buf, sizeof(buf), LC_TEXT("%d hours %d minutes %d seconds left on your chat block"), hour, min, sec);
	else if (hour > 0 && min == 0)
		snprintf(buf, sizeof(buf), LC_TEXT("%d hours %d seconds left on your chat block"), hour, sec);
	else if (hour == 0 && min > 0)
		snprintf(buf, sizeof(buf), LC_TEXT("%d minutes %d seconds left on your chat block"), min, sec);
	else
		snprintf(buf, sizeof(buf), LC_TEXT("%d seconds left on your chat block"), sec);

	ch->ChatPacket(CHAT_TYPE_INFO, buf);
}

EVENTINFO(spam_event_info)
{
	char host[MAX_HOST_LENGTH+1];

	spam_event_info()
	{
		::memset( host, 0, MAX_HOST_LENGTH+1 );
	}
};

typedef boost::unordered_map<std::string, std::pair<unsigned int, LPEVENT> > spam_score_of_ip_t;
spam_score_of_ip_t spam_score_of_ip;

EVENTFUNC(block_chat_by_ip_event)
{
	spam_event_info* info = dynamic_cast<spam_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "block_chat_by_ip_event> <Factor> Null pointer" );
		return 0;
	}

	const char * host = info->host;

	spam_score_of_ip_t::iterator it = spam_score_of_ip.find(host);

	if (it != spam_score_of_ip.end())
	{
		it->second.first = 0;
		it->second.second = NULL;
	}

	return 0;
}

bool SpamBlockCheck(LPCHARACTER ch, const char* const buf, const size_t buflen)
{
	if (ch->GetLevel() < g_iSpamBlockMaxLevel)
	{
		spam_score_of_ip_t::iterator it = spam_score_of_ip.find(ch->GetDesc()->GetHostName());

		if (it == spam_score_of_ip.end())
		{
			spam_score_of_ip.emplace(ch->GetDesc()->GetHostName(), std::make_pair(0, (LPEVENT) NULL));
			it = spam_score_of_ip.find(ch->GetDesc()->GetHostName());
		}

		if (it->second.second)
		{
			SendBlockChatInfo(ch, event_time(it->second.second) / passes_per_sec);
			return true;
		}

		unsigned int score;
		const char * word = SpamManager::instance().GetSpamScore(buf, buflen, score);

		it->second.first += score;

		if (word)
			sys_log(0, "SPAM_SCORE: %s text: %s score: %u total: %u word: %s", ch->GetName(), buf, score, it->second.first, word);

		if (it->second.first >= g_uiSpamBlockScore)
		{
			spam_event_info* info = AllocEventInfo<spam_event_info>();
			strlcpy(info->host, ch->GetDesc()->GetHostName(), sizeof(info->host));

			it->second.second = event_create(block_chat_by_ip_event, info, PASSES_PER_SEC(g_uiSpamBlockDuration));
			sys_log(0, "SPAM_IP: %s for %u seconds", info->host, g_uiSpamBlockDuration);

			LogManager::instance().CharLog(ch, 0, "SPAM", word);

			SendBlockChatInfo(ch, event_time(it->second.second) / passes_per_sec);

			return true;
		}
	}

	return false;
}

enum
{
	TEXT_TAG_PLAIN,
	TEXT_TAG_TAG, // ||
	TEXT_TAG_COLOR, // |cffffffff
	TEXT_TAG_HYPERLINK_START, // |H
	TEXT_TAG_HYPERLINK_END, // |h ex) |Hitem:1234:1:1:1|h
	TEXT_TAG_RESTORE_COLOR,
};

int GetTextTag(const char * src, int maxLen, int & tagLen, std::string & extraInfo)
{
	tagLen = 1;

	if (maxLen < 2 || *src != '|')
		return TEXT_TAG_PLAIN;

	const char * cur = ++src;

	if (*cur == '|')
	{
		tagLen = 2;
		return TEXT_TAG_TAG;
	}
	else if (*cur == 'c') // color |cffffffffblahblah|r
	{
		tagLen = 2;
		return TEXT_TAG_COLOR;
	}
	else if (*cur == 'H')
	{
		tagLen = 2;
		return TEXT_TAG_HYPERLINK_START;
	}
	else if (*cur == 'h') // end of hyperlink
	{
		tagLen = 2;
		return TEXT_TAG_HYPERLINK_END;
	}

	return TEXT_TAG_PLAIN;
}

void GetTextTagInfo(const char * src, int src_len, int & hyperlinks, bool & colored)
{
	colored = false;
	hyperlinks = 0;

	int len;
	std::string extraInfo;

	for (int i = 0; i < src_len;)
	{
		int tag = GetTextTag(&src[i], src_len - i, len, extraInfo);

		if (tag == TEXT_TAG_HYPERLINK_START)
			++hyperlinks;

		if (tag == TEXT_TAG_COLOR)
			colored = true;

		i += len;
	}
}

int ProcessTextTag(LPCHARACTER ch, const char * c_pszText, size_t len)
{
	int hyperlinks;
	bool colored;

	GetTextTagInfo(c_pszText, len, hyperlinks, colored);

	if (colored == true && hyperlinks == 0)
		return 4;

	if (len > 1)
	{
		// allowed colors
		constexpr std::string_view allow_color1 = "|cffffc700";
		constexpr std::string_view allow_color2 = "|cfff1e6c0";
		constexpr size_t target_length = 10;

		std::string_view buf_view(c_pszText, len);

		// Loop through the buffer to find occurrences of "|c"
		for (size_t i = 0; i <= len - 2; ++i) {
			if (buf_view.substr(i, 2) == "|c") {
				// If there's not enough characters left for a full target substring
				if (i + target_length > len) {
					break;
				}

				// Extract the substring starting at position i
				auto substr = buf_view.substr(i, target_length);

				// Check if the substring matches either of the targets
				if (substr == allow_color1 || substr == allow_color2) {
					i += target_length - 1; // Move index past the matched target
				}
				else {
					return 4;
				}
			}
		}
	}

#ifdef ENABLE_NEWSTUFF
	if (g_bDisablePrismNeed)
		return 0;
#endif
	bool hasPrismPlus = ch->IsActivePremiumSubscription() || ch->CountSpecifyItem(ITEM_PRISM_PLUS) > 0;
	if (hasPrismPlus)
		return 0;

	int nPrismCount = ch->CountSpecifyItem(ITEM_PRISM);

	if (nPrismCount < hyperlinks)
		return 1;

	if (!ch->GetMyShop())
	{
		ch->RemoveSpecifyItem(ITEM_PRISM, hyperlinks);
		return 0;
	} else
	{
		int sellingNumber = ch->GetMyShop()->GetNumberByVnum(ITEM_PRISM);
		if(nPrismCount - sellingNumber < hyperlinks)
		{
			return 2;
		} else
		{
			ch->RemoveSpecifyItem(ITEM_PRISM, hyperlinks);
			return 0;
		}
	}

	return 4;
}

void SendWhisperError(LPDESC d, BYTE whisperErrorType, const char* szNameTo)
{
	if (d)
	{
		TPacketGCWhisper pack;
		pack.bHeader = HEADER_GC_WHISPER;
		pack.bType = whisperErrorType;
		pack.wSize = sizeof(TPacketGCWhisper);
		strlcpy(pack.szNameFrom, szNameTo, sizeof(pack.szNameFrom));
		d->Packet(&pack, sizeof(pack));
	}
}

int CInputMain::Whisper(LPCHARACTER ch, const char * data, size_t uiBytes)
{
	const TPacketCGWhisper* pinfo = reinterpret_cast<const TPacketCGWhisper*>(data);

	if (uiBytes < pinfo->wSize)
		return -1;

	int iExtraLen = pinfo->wSize - sizeof(TPacketCGWhisper);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->wSize, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}

#ifdef ENABLE_WHISPER_CHAT_SPAMLIMIT
	if (ch->IncreaseChatCounter() >= 10)
	{
		ch->GetDesc()->DelayedDisconnect(0);
		return (iExtraLen);
	}
#endif

	if (ch->FindAffect(AFFECT_BLOCK_CHAT))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your chat is blocked."));
		return (iExtraLen);
	}

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(pinfo->szNameTo);

	if (pkChr == ch)
		return (iExtraLen);

	// A whisper to a playerbot is answered by the bot rather than delivered
	// to a descriptor with no client behind it.
	if (pkChr && pkChr->GetDesc() && pkChr->GetDesc()->IsBot() &&
			ch->GetDesc() && !ch->GetDesc()->IsBot())
	{
		char szBotText[CHAT_MAX_LEN + 1];
		strlcpy(szBotText, data + sizeof(TPacketCGWhisper), MIN(iExtraLen + 1, (int) sizeof(szBotText)));
		CPlayerBotManager::instance().OnPlayerWhisper(ch, pkChr, szBotText);
		return (iExtraLen);
	}

	LPDESC pkDesc = NULL;
	BYTE bOpponentEmpire = 0;
	DWORD bOpponentPID = 0;

	if (test_server)
	{
		if (!pkChr)
			sys_log(0, "Whisper to %s(%s) from %s", "Null", pinfo->szNameTo, ch->GetName());
		else
			sys_log(0, "Whisper to %s(%s) from %s", pkChr->GetName(), pinfo->szNameTo, ch->GetName());
	}

	if (ch->IsBlockMode(BLOCK_WHISPER))
	{
		SendWhisperError(ch->GetDesc(), WHISPER_TYPE_SENDER_BLOCKED, pinfo->szNameTo);
		return iExtraLen;
	}

	if (ch->GetLevel() < WHISPER_MINIMUM_LEVEL && pinfo->szNameTo && pinfo->szNameTo[0] != '[')
	{
		SendWhisperError(ch->GetDesc(), WHISPER_TYPE_LOW_LEVEL, pinfo->szNameTo);
		return iExtraLen;
	}

	if (!pkChr)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(pinfo->szNameTo);

		if (pkCCI)
		{
			pkDesc = pkCCI->pkDesc;
			pkDesc->SetRelay(pinfo->szNameTo);
			bOpponentEmpire = pkCCI->bEmpire;
			bOpponentPID = pkCCI->dwPID;

			if (test_server)
				sys_log(0, "Whisper to %s from %s (Channel %d Mapindex %d)", "Null", ch->GetName(), pkCCI->bChannel, pkCCI->lMapIndex);
		}
	}
	else
	{
		pkDesc = pkChr->GetDesc();
		bOpponentEmpire = pkChr->GetEmpire();
		bOpponentPID = pkChr->GetPlayerID();
	}

	if (!pkDesc)
	{
		if (ch->GetDesc())
		{
			SendWhisperError(ch->GetDesc(), WHISPER_TYPE_NOT_EXIST, pinfo->szNameTo);
			sys_log(0, "WHISPER: no player");
		}
	}
	else
	{
		if (ch->IsBlockMode(BLOCK_WHISPER))
		{
			SendWhisperError(ch->GetDesc(), WHISPER_TYPE_SENDER_BLOCKED, pinfo->szNameTo);
		}
		else if (pkChr && pkChr->IsBlockMode(BLOCK_WHISPER))
		{
			SendWhisperError(ch->GetDesc(), WHISPER_TYPE_TARGET_BLOCKED, pinfo->szNameTo);
		}
		else if (MessengerManager::instance().IsBlock(ch->GetPlayerID(), pinfo->szNameTo))
		{
			SendWhisperError(ch->GetDesc(), WHISPER_TYPE_MESSENGER_BLOCKED_TARGET, pinfo->szNameTo);
		}
		else if (MessengerManager::instance().IsBlock(bOpponentPID, ch->GetName()) &&
				(test_server || !ch->IsGM()))
		{
			SendWhisperError(ch->GetDesc(), WHISPER_TYPE_MESSENGER_BLOCKED_ME, pinfo->szNameTo);
		}
		else
		{

			BYTE bType = WHISPER_TYPE_NORMAL;

			char buf[CHAT_MAX_LEN + 1];
			strlcpy(buf, data + sizeof(TPacketCGWhisper), MIN(iExtraLen + 1, sizeof(buf)));
			const size_t buflen = strlen(buf);

			if (true == SpamBlockCheck(ch, buf, buflen))
			{
				if (!pkChr)
				{
					CCI * pkCCI = P2P_MANAGER::instance().Find(pinfo->szNameTo);

					if (pkCCI)
					{
						pkDesc->SetRelay("");
					}
				}
				return iExtraLen;
			}

			//CBanwordManager::instance().ConvertString(buf, buflen);

			if (g_bEmpireWhisper)
				if (!ch->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
					if (!(pkChr && pkChr->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE)))
						if (bOpponentEmpire != ch->GetEmpire() && ch->GetEmpire() && bOpponentEmpire
								&& ch->GetGMLevel() == GM_PLAYER && gm_get_level(pinfo->szNameTo) == GM_PLAYER)

						{
							if (!pkChr)
							{
								bType = ch->GetEmpire() << 4;
							}
							else
							{
								ConvertEmpireText(ch->GetEmpire(), buf, buflen, 10 + 2 * pkChr->GetSkillPower(SKILL_LANGUAGE1 + ch->GetEmpire() - 1));
							}
						}

			int processReturn = ProcessTextTag(ch, buf, buflen);
			if (processReturn == 4) // color chat hack
			{
				pkDesc->SetRelay("");
				return iExtraLen;
			}
#ifndef DISABLE_WHISPER_PRISM_REQUIREMENT
			if (0!=processReturn)
			{
				if (ch->GetDesc())
				{
					TItemTable * pTable = ITEM_MANAGER::instance().GetTable(ITEM_PRISM);

					if (pTable)
					{
						char buf[128];
						int len;
						if (3==processReturn)
							len = snprintf(buf, sizeof(buf), LC_TEXT("This function is not available right now."), pTable->szLocaleName);
						else
							len = snprintf(buf, sizeof(buf), LC_TEXT("ITEM_PRISM_NEED"));

						if (len < 0 || len >= (int) sizeof(buf))
							len = sizeof(buf) - 1;

						++len;

						TPacketGCWhisper pack;

						pack.bHeader = HEADER_GC_WHISPER;
						pack.bType = WHISPER_TYPE_ERROR;
						pack.wSize = sizeof(TPacketGCWhisper) + len;
						strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));

						ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
						ch->GetDesc()->Packet(buf, len);

						sys_log(0, "WHISPER: not enough %s: char: %s", pTable->szLocaleName, ch->GetName());
					}
				}

				pkDesc->SetRelay("");
				return (iExtraLen);
			}
#endif

			if (ch->IsGM())
				bType = (bType & 0xF0) | WHISPER_TYPE_GM;
			else if (ch->IsTutor())
				bType = (bType & 0xF0) | WHISPER_TYPE_TUTOR;

			if (buflen > 0)
			{
				TPacketGCWhisper pack;

				pack.bHeader = HEADER_GC_WHISPER;
				pack.wSize = sizeof(TPacketGCWhisper) + buflen;
				pack.bType = bType;
				strlcpy(pack.szNameFrom, ch->GetName(), sizeof(pack.szNameFrom));

				TEMP_BUFFER tmpbuf;

				tmpbuf.write(&pack, sizeof(pack));
				tmpbuf.write(buf, buflen);

				pkDesc->Packet(tmpbuf.read_peek(), tmpbuf.size());

				// @warme006
				// sys_log(0, "WHISPER: %s -> %s : %s", ch->GetName(), pinfo->szNameTo, buf);
#ifdef ENABLE_CHAT_LOGGING

				auto szNameToLen = strlen(pinfo->szNameTo);
				LogManager::instance().EscapeString(__escape_string, sizeof(__escape_string), buf, buflen);
				LogManager::instance().EscapeString(__escape_string2, sizeof(__escape_string2), pinfo->szNameTo, szNameToLen);

				LogManager::instance().ChatLog(ch->GetMapIndex(), ch->GetPlayerID(), ch->GetName(), 0, __escape_string2, "WHISPER", __escape_string, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "");

#endif
			}
		}
	}
	if(pkDesc)
		pkDesc->SetRelay("");

	return (iExtraLen);
}

struct RawPacketToCharacterFunc
{
	const void * m_buf;
	int	m_buf_len;

	RawPacketToCharacterFunc(const void * buf, int buf_len) : m_buf(buf), m_buf_len(buf_len)
	{
	}

	void operator () (LPCHARACTER c)
	{
		if (!c->GetDesc())
			return;

		c->GetDesc()->Packet(m_buf, m_buf_len);
	}
};

struct FEmpireChatPacket
{
	packet_chat& p;
	const char* orig_msg;
	int orig_len;
	char converted_msg[CHAT_MAX_LEN+1];

	BYTE bEmpire;
	int iMapIndex;
	int namelen;

	FEmpireChatPacket(packet_chat& p, const char* chat_msg, int len, BYTE bEmpire, int iMapIndex, int iNameLen)
		: p(p), orig_msg(chat_msg), orig_len(len), bEmpire(bEmpire), iMapIndex(iMapIndex), namelen(iNameLen)
	{
		memset( converted_msg, 0, sizeof(converted_msg) );
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (d->GetCharacter()->GetMapIndex() != iMapIndex)
			return;

		d->BufferedPacket(&p, sizeof(packet_chat));

		if (d->GetEmpire() == bEmpire ||
			bEmpire == 0 ||
			d->GetCharacter()->GetGMLevel() > GM_PLAYER ||
			d->GetCharacter()->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
		{
			d->Packet(orig_msg, orig_len);
		}
		else
		{
			size_t len = strlcpy(converted_msg, orig_msg, sizeof(converted_msg));

			if (len >= sizeof(converted_msg))
				len = sizeof(converted_msg) - 1;

			ConvertEmpireText(bEmpire, converted_msg + namelen, len - namelen, 10 + 2 * d->GetCharacter()->GetSkillPower(SKILL_LANGUAGE1 + bEmpire - 1));
			d->Packet(converted_msg, orig_len);
		}
	}
};

struct FYmirChatPacket
{
	packet_chat& packet;
	const char* m_szChat;
	size_t m_lenChat;
	const char* m_szName;

	int m_iMapIndex;
	BYTE m_bEmpire;
	bool m_ring;

	char m_orig_msg[CHAT_MAX_LEN+1];
	int m_len_orig_msg;
	char m_conv_msg[CHAT_MAX_LEN+1];
	int m_len_conv_msg;

	FYmirChatPacket(packet_chat& p, const char* chat, size_t len_chat, const char* name, size_t len_name, int iMapIndex, BYTE empire, bool ring)
		: packet(p),
		m_szChat(chat), m_lenChat(len_chat),
		m_szName(name),
		m_iMapIndex(iMapIndex), m_bEmpire(empire),
		m_ring(ring)
	{
		m_len_orig_msg = snprintf(m_orig_msg, sizeof(m_orig_msg), "%s : %s", m_szName, m_szChat) + 1;

		if (m_len_orig_msg < 0 || m_len_orig_msg >= (int) sizeof(m_orig_msg))
			m_len_orig_msg = sizeof(m_orig_msg) - 1;

		m_len_conv_msg = snprintf(m_conv_msg, sizeof(m_conv_msg), "??? : %s", m_szChat) + 1;

		if (m_len_conv_msg < 0 || m_len_conv_msg >= (int) sizeof(m_conv_msg))
			m_len_conv_msg = sizeof(m_conv_msg) - 1;

		ConvertEmpireText(m_bEmpire, m_conv_msg + 6, m_len_conv_msg - 6, 10);
	}

	void operator() (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (d->GetCharacter()->GetMapIndex() != m_iMapIndex)
			return;

		if (m_ring ||
			d->GetEmpire() == m_bEmpire ||
			d->GetCharacter()->GetGMLevel() > GM_PLAYER ||
			d->GetCharacter()->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
		{
			packet.size = m_len_orig_msg + sizeof(TPacketGCChat);

			d->BufferedPacket(&packet, sizeof(packet_chat));
			d->Packet(m_orig_msg, m_len_orig_msg);
		}
		else
		{
			packet.size = m_len_conv_msg + sizeof(TPacketGCChat);

			d->BufferedPacket(&packet, sizeof(packet_chat));
			d->Packet(m_conv_msg, m_len_conv_msg);
		}
	}
};

int CInputMain::Chat(LPCHARACTER ch, const char * data, size_t uiBytes)
{
	const TPacketCGChat* pinfo = reinterpret_cast<const TPacketCGChat*>(data);

	if (uiBytes < pinfo->size)
		return -1;

	const int iExtraLen = pinfo->size - sizeof(TPacketCGChat);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->size, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}

	if (pinfo->type == CHAT_TYPE_SPECIAL_INTERNAL && ch->HasPlayerData())
	{
		ch->playerData->MarkHacker("BOT_HLBOT_NEW", ch);
		return iExtraLen;
	}

	char buf[CHAT_MAX_LEN - (CHARACTER_NAME_MAX_LEN * 2 + 31) + 1];
	strlcpy(buf, data + sizeof(TPacketCGChat), MIN(iExtraLen + 1, sizeof(buf)));
	const size_t buflen = strlen(buf);

	if (buflen > 1 && *buf == '/')
	{
		interpret_command(ch, buf + 1, buflen - 1);
		return iExtraLen;
	}
#ifdef ENABLE_CHAT_SPAMLIMIT
	if (ch->IncreaseChatCounter() >= 4)
	{
		if (ch->GetChatCounter() == 10)
			ch->GetDesc()->DelayedDisconnect(0);
		return iExtraLen;
	}
#else
	if (ch->IncreaseChatCounter() >= 10)
	{
		if (ch->GetChatCounter() == 10)
		{
			sys_log(0, "CHAT_HACK: %s", ch->GetName());
			ch->GetDesc()->DelayedDisconnect(5);
		}

		return iExtraLen;
	}
#endif

	if (ch->GetLevel() < 5)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need a minimum level of %d to be able to talk."), 5);
		return (iExtraLen);
	}

	const CAffect* pAffect = ch->FindAffect(AFFECT_BLOCK_CHAT);

	if (pAffect != NULL)
	{
		SendBlockChatInfo(ch, pAffect->lDuration);
		return iExtraLen;
	}

	if (true == SpamBlockCheck(ch, buf, buflen))
	{
		return iExtraLen;
	}

	// @fixme133 begin
	//CBanwordManager::instance().ConvertString(buf, buflen);

	int processReturn = ProcessTextTag(ch, buf, buflen);
	if (0!=processReturn)
	{
		const TItemTable* pTable = ITEM_MANAGER::instance().GetTable(ITEM_PRISM);

		if (NULL != pTable)
		{
			if (3==processReturn)
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This function is not available right now."), pTable->szLocaleName);
			else
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ITEM_PRISM_NEED"));

		}

		return iExtraLen;
	}
	// @fixme133 end

	char chatbuf[CHAT_MAX_LEN + 1];
	int len = 0;

	if (ch->GetGMLevel() < GM_WIZARD)
	{
		if (CHAT_TYPE_SHOUT == pinfo->type || CHAT_TYPE_TRADE == pinfo->type)
		{
			const char* color_by_empire[EMPIRE_MAX_NUM] = { "", "ff5959", "ffdb3b", "4590ff" };

			std::string message_format = "[|Hmsg:%s,%d|h%s|h|r]: %s";
			if (pinfo->type == CHAT_TYPE_SHOUT)
			{
				std::string message_format = "[|Hmsg:%s,%d|h%s|h|r]: %s";
				len = snprintf(chatbuf, sizeof(chatbuf), message_format.c_str(), ch->GetName(), ch->GetEmpire(), ch->GetName(), buf);
			}
			else if (CHAT_TYPE_TRADE == pinfo->type)
			{
				std::string message_format = "[|cff%s|Hmsg:%s,%d|h%s|h|r]: %s";
				//message_format = "*|r " + message_format;
				len = snprintf(chatbuf, sizeof(chatbuf), message_format.c_str(), color_by_empire[ch->GetEmpire()], ch->GetName(), ch->GetEmpire(), ch->GetName(), buf);
			}


		}
		else
			len = snprintf(chatbuf, sizeof(chatbuf), "%s : %s", ch->GetName(), buf);
	}
	else
	{
		// GM chat
		len = snprintf(chatbuf, sizeof(chatbuf), "|cffffb514[Mt2009]|r %s : %s", ch->GetName(), buf);
	}
	

	if (len < 0 || len >= (int) sizeof(chatbuf))
		len = sizeof(chatbuf) - 1;

	if (pinfo->type == CHAT_TYPE_SHOUT || pinfo->type == CHAT_TYPE_TRADE)
	{

		int level_limit = g_iShoutLimitLevel;
		if (pinfo->type == CHAT_TYPE_TRADE)
			level_limit = 20;

		if (ch->GetLevel() < level_limit)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need a minimum level of %d to be able to call."), level_limit);
			return (iExtraLen);
		}

		int second_limit = 15;
		if (pinfo->type == CHAT_TYPE_TRADE)
			second_limit = 30;

		int last_chat_time = pinfo->type == CHAT_TYPE_SHOUT ? ch->GetLastShoutPulse() : ch->GetLastTradeShoutPulse();
		if (thecore_heart->pulse - last_chat_time < passes_per_sec * second_limit)
			return (iExtraLen);

		if (pinfo->type == CHAT_TYPE_SHOUT)
			ch->SetLastShoutPulse(thecore_heart->pulse);
		else
			ch->SetLastTradeShoutPulse(thecore_heart->pulse);

		TPacketGGShout p{};

		p.bHeader = HEADER_GG_SHOUT;
		p.bEmpire = pinfo->type == CHAT_TYPE_SHOUT ? ch->GetEmpire() : 0;
		p.bChatType = pinfo->type;
		strlcpy(p.szText, chatbuf, sizeof(p.szText));

		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShout));

		if (pinfo->type == CHAT_TYPE_SHOUT)
		{
			SendShout(chatbuf, ch->GetEmpire());

			// The playerbots hear it too: one with the thing on its counter, or
			// one short of it, whispers back.
			if (ch->GetDesc() && !ch->GetDesc()->IsBot())
				CPlayerBotManager::instance().OnPlayerShout(ch, buf);
		}
		else
		{
			SendTrade(chatbuf);
		}

#ifdef ENABLE_CHAT_LOGGING
		LogManager::instance().EscapeString(__escape_string, sizeof(__escape_string), chatbuf, len);
		LogManager::instance().ChatLog(ch->GetMapIndex(), ch->GetPlayerID(), ch->GetName(), 0, "", CHAT_TYPE_SHOUT == pinfo->type ? "SHOUT" : "TRADE", __escape_string, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "");
#endif

		return (iExtraLen);
	}

	TPacketGCChat pack_chat;

	pack_chat.header = HEADER_GC_CHAT;
	pack_chat.size = sizeof(TPacketGCChat) + len;
	pack_chat.type = pinfo->type;
	pack_chat.id = ch->GetVID();

	switch (pinfo->type)
	{
		case CHAT_TYPE_TALKING:
			{
				const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();

				if (false)
				{
					std::for_each(c_ref_set.begin(), c_ref_set.end(),
							FYmirChatPacket(pack_chat,
								buf,
								strlen(buf),
								ch->GetName(),
								strlen(ch->GetName()),
								ch->GetMapIndex(),
								ch->GetEmpire(),
								ch->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE)));
				}
				else
				{
					std::for_each(c_ref_set.begin(), c_ref_set.end(),
							FEmpireChatPacket(pack_chat,
								chatbuf,
								len,
								(ch->GetGMLevel() > GM_PLAYER ||
								 ch->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE)) ? 0 : ch->GetEmpire(),
								ch->GetMapIndex(), strlen(ch->GetName())));
#ifdef ENABLE_CHAT_LOGGING
					LogManager::instance().EscapeString(__escape_string, sizeof(__escape_string), chatbuf, len);
					LogManager::instance().ChatLog(ch->GetMapIndex(), ch->GetPlayerID(), ch->GetName(), 0, "", "NORMAL", __escape_string, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "");
#endif
				}
			}
			break;

		case CHAT_TYPE_PARTY:
			{
				if (!ch->GetParty())
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are not in this Group."));
				else
				{
					TEMP_BUFFER tbuf;

					tbuf.write(&pack_chat, sizeof(pack_chat));
					tbuf.write(chatbuf, len);

					RawPacketToCharacterFunc f(tbuf.read_peek(), tbuf.size());
					ch->GetParty()->ForEachOnlineMember(f);
#ifdef ENABLE_CHAT_LOGGING
					LogManager::instance().EscapeString(__escape_string, sizeof(__escape_string), chatbuf, len);
					LogManager::instance().ChatLog(ch->GetMapIndex(), ch->GetPlayerID(), ch->GetName(), ch->GetParty()->GetLeaderPID(), "", "PARTY", __escape_string, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "");
#endif
				}
			}
			break;

		case CHAT_TYPE_GUILD:
			{
				if (!ch->GetGuild())
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You did not join this Guild."));
				else
				{
					ch->GetGuild()->Chat(chatbuf);
#ifdef ENABLE_CHAT_LOGGING
					LogManager::instance().EscapeString(__escape_string, sizeof(__escape_string), chatbuf, len);
					LogManager::instance().ChatLog(ch->GetMapIndex(), ch->GetPlayerID(), ch->GetName(), ch->GetGuild()->GetID(), ch->GetGuild()->GetName(), "GUILD", __escape_string, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "");
#endif
				}
			}
			break;

		default:
			sys_err("Unknown chat type %d", pinfo->type);
			break;
	}

	return (iExtraLen);
}

void CInputMain::ItemUse(LPCHARACTER ch, const char * data)
{
	ch->UseItem(((struct command_item_use *) data)->Cell);
}

void CInputMain::ItemToItem(LPCHARACTER ch, const char * pcData)
{
	if (ch && g_bChannel > 90)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot do that on special channel."));
		return;
	}

	TPacketCGItemUseToItem * p = (TPacketCGItemUseToItem *) pcData;
	if (ch)
		ch->UseItem(p->Cell, p->TargetCell);
}

void CInputMain::ItemDrop(LPCHARACTER ch, const char * data)
{
	struct command_item_drop * pinfo = (struct command_item_drop *) data;
	if (!ch)
		return;

#if defined(ENABLE_CHEQUE_SYSTEM) && !defined(DISABLE_CHEQUE_DROP)
	if (pinfo->cheque > 0)
	{
		ch->DropCheque(pinfo->cheque);
		return;
	}
#endif

#ifndef DISABLE_GOLD_PLAYER_DROP
	if (pinfo->gold > 0)
		ch->DropGold(pinfo->gold);
	else
#endif
		ch->DropItem(pinfo->Cell);
}

void CInputMain::ItemDrop2(LPCHARACTER ch, const char * data)
{
	TPacketCGItemDrop2 * pinfo = (TPacketCGItemDrop2 *) data;
	if (!ch)
		return;

#if defined(ENABLE_CHEQUE_SYSTEM) && !defined(DISABLE_CHEQUE_DROP)
	if (pinfo->cheque > 0)
	{
		ch->DropCheque(pinfo->cheque);
		return;
	}
#endif

#ifndef DISABLE_GOLD_PLAYER_DROP
	if (pinfo->gold > 0)
		ch->DropGold(pinfo->gold);
	else
#endif
		ch->DropItem(pinfo->Cell, pinfo->count);
}

void CInputMain::ItemMove(LPCHARACTER ch, const char * data)
{
	struct command_item_move * pinfo = (struct command_item_move *) data;

	if (ch)
		ch->MoveItem(pinfo->Cell, pinfo->CellTo, pinfo->count);
}

void CInputMain::ItemPickup(LPCHARACTER ch, const char * data)
{
	struct command_item_pickup * pinfo = (struct command_item_pickup*) data;
	if (ch)
		ch->PickupItem(pinfo->vid);
}

void CInputMain::QuickslotAdd(LPCHARACTER ch, const char * data)
{
	struct command_quickslot_add * pinfo = (struct command_quickslot_add *) data;
	ch->SetQuickslot(pinfo->pos, pinfo->slot);
}

void CInputMain::QuickslotDelete(LPCHARACTER ch, const char * data)
{
	struct command_quickslot_del * pinfo = (struct command_quickslot_del *) data;
	ch->DelQuickslot(pinfo->pos);
}

void CInputMain::QuickslotSwap(LPCHARACTER ch, const char * data)
{
	struct command_quickslot_swap * pinfo = (struct command_quickslot_swap *) data;
	ch->SwapQuickslot(pinfo->pos, pinfo->change_pos);
}

int CInputMain::Messenger(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGMessenger* p = (TPacketCGMessenger*) c_pData;

	if (uiBytes < sizeof(TPacketCGMessenger))
		return -1;

	c_pData += sizeof(TPacketCGMessenger);
	uiBytes -= sizeof(TPacketCGMessenger);

	switch (p->subheader)
	{
		case MESSENGER_SUBHEADER_CG_ADD_BY_VID:
			{
				if (uiBytes < sizeof(TPacketCGMessengerAddByVID))
					return -1;

				if (!PulseManager::Instance().IncreaseCount(ch->GetPlayerID(), ePulse::Messenger, std::chrono::milliseconds(2000), 5))
					return sizeof(TPacketCGMessengerAddByVID);

				TPacketCGMessengerAddByVID * p2 = (TPacketCGMessengerAddByVID *) c_pData;
				LPCHARACTER ch_companion = CHARACTER_MANAGER::instance().Find(p2->vid);

				if (!ch_companion)
					return sizeof(TPacketCGMessengerAddByVID);

				if (ch->IsObserverMode())
					return sizeof(TPacketCGMessengerAddByVID);

				if (ch_companion->IsBlockMode(BLOCK_MESSENGER_INVITE))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The player has rejected your request to add him to your friend list."));
					return sizeof(TPacketCGMessengerAddByVID);
				}

				LPDESC d = ch_companion->GetDesc();

				if (!d)
					return sizeof(TPacketCGMessengerAddByVID);

				if (ch->GetGMLevel() == GM_PLAYER && ch_companion->GetGMLevel() != GM_PLAYER)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Friends] You cannot add a GM to your list."));
					return sizeof(TPacketCGMessengerAddByVID);
				}

				if (ch->GetDesc() == d)
					return sizeof(TPacketCGMessengerAddByVID);

				MessengerManager::instance().RequestToAdd(ch, ch_companion);
				//MessengerManager::instance().AddToList(ch->GetName(), ch_companion->GetName());
			}
			return sizeof(TPacketCGMessengerAddByVID);

		case MESSENGER_SUBHEADER_CG_ADD_BY_NAME:
			{
				if (uiBytes < CHARACTER_NAME_MAX_LEN)
					return -1;

				if (!PulseManager::Instance().IncreaseCount(ch->GetPlayerID(), ePulse::Messenger, std::chrono::milliseconds(2000), 5))
					return CHARACTER_NAME_MAX_LEN;

				char name[CHARACTER_NAME_MAX_LEN + 1];
				strlcpy(name, c_pData, sizeof(name));

				if (ch->GetGMLevel() == GM_PLAYER && gm_get_level(name) != GM_PLAYER)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Friends] You cannot add a GM to your list."));
					return CHARACTER_NAME_MAX_LEN;
				}

				LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

				if (!tch)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s is not online."), name);
				else
				{
					if (tch == ch)
						return CHARACTER_NAME_MAX_LEN;

					if (tch->IsBlockMode(BLOCK_MESSENGER_INVITE) == true)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The player has rejected your request to add him to your friend list."));
					}
					else
					{
						MessengerManager::instance().RequestToAdd(ch, tch);
						//MessengerManager::instance().AddToList(ch->GetName(), tch->GetName());
					}
				}
			}
			return CHARACTER_NAME_MAX_LEN;

		case MESSENGER_SUBHEADER_CG_REMOVE:
			{
				if (uiBytes < CHARACTER_NAME_MAX_LEN)
					return -1;

				if (!PulseManager::Instance().IncreaseCount(ch->GetPlayerID(), ePulse::Messenger, std::chrono::milliseconds(2000), 5))
					return CHARACTER_NAME_MAX_LEN;

				char char_name[CHARACTER_NAME_MAX_LEN + 1];
				strlcpy(char_name, c_pData, sizeof(char_name));
				MessengerManager::instance().RemoveFromList(ch->GetName(), char_name);
				MessengerManager::instance().RemoveFromList(char_name, ch->GetName()); // @fixme183
			}
			return CHARACTER_NAME_MAX_LEN;

		case MESSENGER_SUBHEADER_CG_ADD_BLOCK:
		{
			if (uiBytes < CHARACTER_NAME_MAX_LEN)
				return -1;

			if (!PulseManager::Instance().IncreaseCount(ch->GetPlayerID(), ePulse::Messenger, std::chrono::milliseconds(2000), 5))
				return CHARACTER_NAME_MAX_LEN;

			char char_name[CHARACTER_NAME_MAX_LEN + 1];
			strlcpy(char_name, c_pData, sizeof(char_name));
			MessengerManager::instance().Block(ch, char_name);
		}
		return CHARACTER_NAME_MAX_LEN;

		case MESSENGER_SUBHEADER_CG_REMOVE_BLOCK:
		{
			if (uiBytes < CHARACTER_NAME_MAX_LEN)
				return -1;

			if (!PulseManager::Instance().IncreaseCount(ch->GetPlayerID(), ePulse::Messenger, std::chrono::milliseconds(2000), 5))
				return CHARACTER_NAME_MAX_LEN;

			char char_name[CHARACTER_NAME_MAX_LEN + 1];
			strlcpy(char_name, c_pData, sizeof(char_name));
			MessengerManager::instance().Unblock(ch, char_name);
		}
		return CHARACTER_NAME_MAX_LEN;

		default:
			sys_err("CInputMain::Messenger : Unknown subheader %d : %s", p->subheader, ch->GetName());
			break;
	}

	return 0;
}

int CInputMain::Shop(LPCHARACTER ch, const char * data, size_t uiBytes)
{
	TPacketCGShop * p = (TPacketCGShop *) data;

	if (uiBytes < sizeof(TPacketCGShop))
		return -1;

	if (test_server)
		sys_log(0, "CInputMain::Shop() ==> SubHeader %d", p->subheader);

	const char * c_pData = data + sizeof(TPacketCGShop);
	uiBytes -= sizeof(TPacketCGShop);

	switch (p->subheader)
	{
		case SHOP_SUBHEADER_CG_END:
			sys_log(1, "INPUT: %s SHOP: END", ch->GetName());
			CShopManager::instance().StopShopping(ch);
			return 0;

		case SHOP_SUBHEADER_CG_BUY:
			{
				if (uiBytes < sizeof(BYTE) + sizeof(BYTE))
					return -1;

				BYTE bPos = *(c_pData + 1);
				sys_log(1, "INPUT: %s SHOP: BUY %d", ch->GetName(), bPos);
				CShopManager::instance().Buy(ch, bPos);
				return (sizeof(BYTE) + sizeof(BYTE));
			}

		case SHOP_SUBHEADER_CG_SELL:
			{
				if (uiBytes < sizeof(BYTE))
					return -1;

				BYTE pos = *c_pData;

				sys_log(0, "INPUT: %s SHOP: SELL", ch->GetName());
				CShopManager::instance().Sell(ch, pos);
				return sizeof(BYTE);
			}

		case SHOP_SUBHEADER_CG_SELL2:
			{
				if (uiBytes < sizeof(BYTE) + sizeof(ITEM_COUNT))
					return -1;

				BYTE pos = *(c_pData++);
				ITEM_COUNT count = *(c_pData);

				sys_log(0, "INPUT: %s SHOP: SELL2", ch->GetName());
				CShopManager::instance().Sell(ch, pos, count);
				return sizeof(BYTE) + sizeof(ITEM_COUNT);
			}

		default:
			sys_err("CInputMain::Shop : Unknown subheader %d : %s", p->subheader, ch->GetName());
			break;
	}

	return 0;
}

void CInputMain::OnClick(LPCHARACTER ch, const char * data)
{
	struct command_on_click *	pinfo = (struct command_on_click *) data;
	LPCHARACTER			victim;

	if ((victim = CHARACTER_MANAGER::instance().Find(pinfo->vid)))
		victim->OnClick(ch);
	else if (test_server)
	{
		sys_err("CInputMain::OnClick %s.Click.NOT_EXIST_VID[%d]", ch->GetName(), pinfo->vid);
	}
}

void CInputMain::Exchange(LPCHARACTER ch, const char * data)
{
	struct command_exchange * pinfo = (struct command_exchange *) data;
	LPCHARACTER	to_ch = NULL;

	if (!PulseManager::Instance().IncreaseCount(ch->GetPlayerID(), ePulse::Exchange, std::chrono::milliseconds(500), 5))
	{
		return;
	}

	if (!ch->CanHandleItem(false, false, BUSY_CAN_HANDLE_ITEM_EXCLUDE | BUSY_EXCHANGE))
		return;

	int iPulse = thecore_pulse();

	if ((to_ch = CHARACTER_MANAGER::instance().Find(pinfo->arg1)))
	{
		if (iPulse - to_ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			to_ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("After a trade, you have to wait %d seconds before you can open a shop."), g_nPortalLimitTime);
			return;
		}

		if( true == to_ch->IsDead() )
		{
			return;
		}
	}

	sys_log(0, "CInputMain()::Exchange()  SubHeader %d ", pinfo->sub_header);

	if (iPulse - ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("After a trade, you have to wait %d seconds before you can open a shop."), g_nPortalLimitTime);
		return;
	}

	switch (pinfo->sub_header)
	{
		case EXCHANGE_SUBHEADER_CG_START:	// arg1 == vid of target character
			if (!ch->GetExchange())
			{
				if (!PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::ExchangeStart, std::chrono::milliseconds(1000)))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("NEXT_ACTION_WAIT_NO_FORMAT"));
					return;
				}

				if ((to_ch = CHARACTER_MANAGER::instance().Find(pinfo->arg1)))
				{
					//MONARCH_LIMIT

					if (iPulse - ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can trade again in %d seconds."), g_nPortalLimitTime);

						if (test_server)
							ch->ChatPacket(CHAT_TYPE_INFO, "[TestOnly][Safebox]Pulse %d LoadTime %d PASS %d", iPulse, ch->GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
						return;
					}

					if (iPulse - to_ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
					{
						to_ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can trade again in %d seconds."), g_nPortalLimitTime);

						if (test_server)
							to_ch->ChatPacket(CHAT_TYPE_INFO, "[TestOnly][Safebox]Pulse %d LoadTime %d PASS %d", iPulse, to_ch->GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
						return;
					}

					if (ch->GetGold() >= GOLD_MAX)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have more than 2 Billion Yang. You cannot trade."));

						sys_err("[OVERFLOG_GOLD] START (%lld) id %u name %s ", ch->GetGold(), ch->GetPlayerID(), ch->GetName());
						return;
					}

					if (to_ch->IsPC())
					{
						if (quest::CQuestManager::instance().GiveItemToPC(ch->GetPlayerID(), to_ch))
						{
							sys_log(0, "Exchange canceled by quest %s %s", ch->GetName(), to_ch->GetName());
							return;
						}
					}

					if (ch->IsBusy(BUSY_EXCHANGE))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot open a private shop while another window is open."));
						return;
					}

					ch->ExchangeStart(to_ch);
				}
			}
			break;

		case EXCHANGE_SUBHEADER_CG_ITEM_ADD:	// arg1 == position of item, arg2 == position in exchange window
			if (ch->GetExchange())
			{
				if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
					ch->GetExchange()->AddItem(pinfo->Pos, pinfo->arg2);
			}
			break;

#ifndef DISABLE_MT2009_EXCHANGE_ITEM_DEL
		case EXCHANGE_SUBHEADER_CG_ITEM_DEL:	// arg1 == position of item
			if (ch->GetExchange())
			{
				if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
					ch->GetExchange()->RemoveItem(pinfo->arg1);
			}
			break;
#endif

		case EXCHANGE_SUBHEADER_CG_ELK_ADD:	// arg1 == amount of gold
			if (ch->GetExchange())
			{
				const YANG nTotalGold = static_cast<YANG>(ch->GetExchange()->GetCompany()->GetOwner()->GetGold()) + static_cast<YANG>(pinfo->arg1);

				if (GOLD_MAX <= nTotalGold)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The player has more than 2 Billion Yang. You cannot trade with him."));

					sys_err("[OVERFLOW_GOLD] ELK_ADD (%lld) id %u name %s ",
							ch->GetExchange()->GetCompany()->GetOwner()->GetGold(),
							ch->GetExchange()->GetCompany()->GetOwner()->GetPlayerID(),
						   	ch->GetExchange()->GetCompany()->GetOwner()->GetName());

					return;
				}

				if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
					ch->GetExchange()->AddGold(pinfo->arg1);
			}
			break;

#ifdef ENABLE_CHEQUE_SYSTEM
		case EXCHANGE_SUBHEADER_CG_CHEQUE_ADD:	// arg1 == amount of cheque
			if (ch->GetExchange())
			{
				const int64_t nTotalCheque = static_cast<int64_t>(ch->GetExchange()->GetCompany()->GetOwner()->GetCheque()) + static_cast<int64_t>(pinfo->arg1);

				if (CHEQUE_MAX <= nTotalCheque)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't trade because the other character exceeds 999 won."));

					sys_err("[OVERFLOW_CHEQUE] CHEQUE_ADD (%u) id %u name %s ",
						ch->GetExchange()->GetCompany()->GetOwner()->GetCheque(),
						ch->GetExchange()->GetCompany()->GetOwner()->GetPlayerID(),
						ch->GetExchange()->GetCompany()->GetOwner()->GetName());

					return;
				}

				if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
					ch->GetExchange()->AddCheque(pinfo->arg1);
			}
			break;
#endif

		case EXCHANGE_SUBHEADER_CG_ACCEPT:	// arg1 == not used
			if (ch->GetExchange())
			{
				sys_log(0, "CInputMain()::Exchange() ==> ACCEPT ");
				ch->GetExchange()->Accept(true);
			}

			break;

		case EXCHANGE_SUBHEADER_CG_CANCEL:	// arg1 == not used
			if (ch->GetExchange())
				ch->GetExchange()->Cancel();
			break;
	}
}

void CInputMain::Position(LPCHARACTER ch, const char * data)
{
	struct command_position * pinfo = (struct command_position *) data;

	switch (pinfo->position)
	{
		case POSITION_GENERAL:
			ch->Standup();
			break;

		case POSITION_SITTING_CHAIR:
			ch->Sitdown(0);
			break;

		case POSITION_SITTING_GROUND:
			ch->Sitdown(1);
			break;
	}
}

static const int ComboSequenceBySkillLevel[3][8] =
{
	// 0   1   2   3   4   5   6   7
	{ 14, 15, 16, 17,  0,  0,  0,  0 },
	{ 14, 15, 16, 18, 20,  0,  0,  0 },
	{ 14, 15, 16, 18, 19, 17,  0,  0 },
};

#define COMBO_HACK_ALLOWABLE_MS	100

bool CheckComboHack(LPCHARACTER ch, BYTE bArg, DWORD dwTime, bool CheckSpeedHack)
{
	if(!gHackCheckEnable) return false;

	//	if (IsStun() || IsDead()) return false;

	if (ch->IsStun() || ch->IsDead())
		return false;
	int ComboInterval = dwTime - ch->GetLastComboTime();
	int HackScalar = 0;
#if 0
	sys_log(0, "COMBO: %s arg:%u seq:%u delta:%d checkspeedhack:%d",
			ch->GetName(), bArg, ch->GetComboSequence(), ComboInterval - ch->GetValidComboInterval(), CheckSpeedHack);
#endif

	if (bArg == 14)
	{
		if (CheckSpeedHack && ComboInterval > 0 && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
		{
			//HackScalar = 1 + (ch->GetValidComboInterval() - ComboInterval) / 300;

			//sys_log(0, "COMBO_HACK: 2 %s arg:%u interval:%d valid:%u atkspd:%u riding:%s",
			//		ch->GetName(),
			//		bArg,
			//		ComboInterval,
			//		ch->GetValidComboInterval(),
			//		ch->GetPoint(POINT_ATT_SPEED),
			//	    ch->IsRiding() ? "yes" : "no");
		}

		ch->SetComboSequence(1);
		ch->SetValidComboInterval((int) (ani_combo_speed(ch, 1) / (ch->GetPoint(POINT_ATT_SPEED) / 100.f)));
		ch->SetLastComboTime(dwTime);
	}
	else if (bArg > 14 && bArg < 22)
	{
		int idx = MIN(2, ch->GetComboIndex());

		if (ch->GetComboSequence() > 5)
		{
			HackScalar = 1;
			ch->SetValidComboInterval(300);
			sys_log(0, "COMBO_HACK: 5 %s combo_seq:%d", ch->GetName(), ch->GetComboSequence());
		}

		else if (bArg == 21 &&
				 idx == 2 &&
				 ch->GetComboSequence() == 5 &&
				 ch->GetJob() == JOB_ASSASSIN &&
				 ch->GetWear(WEAR_WEAPON) &&
				 ch->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER)
			ch->SetValidComboInterval(300);
#ifdef ENABLE_WOLFMAN_CHARACTER
		else if (bArg == 21 && idx == 2 && ch->GetComboSequence() == 5 && ch->GetJob() == JOB_WOLFMAN && ch->GetWear(WEAR_WEAPON) && ch->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_CLAW)
			ch->SetValidComboInterval(300);
#endif
		else if (ComboSequenceBySkillLevel[idx][ch->GetComboSequence()] != bArg)
		{
			HackScalar = 1;
			ch->SetValidComboInterval(300);

			sys_log(0, "COMBO_HACK: 3 %s arg:%u valid:%u combo_idx:%d combo_seq:%d",
					ch->GetName(),
					bArg,
					ComboSequenceBySkillLevel[idx][ch->GetComboSequence()],
					idx,
					ch->GetComboSequence());
		}
		else
		{
			if (CheckSpeedHack && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
			{
				HackScalar = 1 + (ch->GetValidComboInterval() - ComboInterval) / 100;

				sys_log(0, "COMBO_HACK: 2 %s arg:%u interval:%d valid:%u atkspd:%u riding:%s",
						ch->GetName(),
						bArg,
						ComboInterval,
						ch->GetValidComboInterval(),
						ch->GetPoint(POINT_ATT_SPEED),
						ch->IsRiding() ? "yes" : "no");
			}

			//if (ch->IsHorseRiding())
			if (ch->IsRiding())
				ch->SetComboSequence(ch->GetComboSequence() == 1 ? 2 : 1);
			else
				ch->SetComboSequence(ch->GetComboSequence() + 1);

			ch->SetValidComboInterval((int) (ani_combo_speed(ch, bArg - 13) / (ch->GetPoint(POINT_ATT_SPEED) / 100.f)));
			ch->SetLastComboTime(dwTime);
		}
	}
	else if (bArg == 13)
	{
		if (ch->GetRaceNum() >= MAIN_RACE_MAX_NUM)
		{
			// POLYMORPH_BUG_FIX

			// DELETEME

			float normalAttackDuration = CMotionManager::instance().GetNormalAttackDuration(ch->GetRaceNum());
			int k = (int) (normalAttackDuration / ((float) ch->GetPoint(POINT_ATT_SPEED) / 100.f) * 900.f);
			ch->SetValidComboInterval(k);
			ch->SetLastComboTime(dwTime);
			// END_OF_POLYMORPH_BUG_FIX
		}
	}
	else
	{
		if (ch->GetDesc()->DelayedDisconnect(number(2, 9)))
		{
			LogManager::instance().HackLog("Hacker", ch);
			sys_log(0, "HACKER: %s arg %u", ch->GetName(), bArg);
		}

		HackScalar = 10;
		ch->SetValidComboInterval(300);
	}

	if (HackScalar)
	{
		if (get_dword_time() - ch->GetLastMountTime() > 1500)
			ch->IncreaseComboHackCount(1 + HackScalar);

		ch->SkipComboAttackByTime(ch->GetValidComboInterval());
	}

	return HackScalar;
}

void CInputMain::Move(LPCHARACTER ch, const char * data)
{
	if (!ch->CanMove())
		return;

	struct command_move * pinfo = (struct command_move *) data;

	if (pinfo->bFunc >= FUNC_MAX_NUM && !(pinfo->bFunc & 0x80))
	{
		sys_err("invalid move type: %s", ch->GetName());
		return;
	}

	if (pinfo->bFunc == FUNC_MOVE && ch->IsAffectFlag(AFF_ROOT))
		return;

	BYTE correctArg = pinfo->bArg;
	if (ch->HasPlayerData())
	{
		//ch->ChatDebug("MovePacket: bFunc %u bArg %u ", pinfo->bFunc, pinfo->bArg);

		if (pinfo->bFunc == 1) correctArg = 0;

		if (pinfo->bFunc == 1) { // CInstanceBase::FUNC_MOVE

			if (ch->playerData->IsHacker() && pinfo->bArg < 8)
				ch->playerData->MarkHackerReleased(ch);
			else if (pinfo->bArg == 9)
				ch->playerData->MarkHacker("BOT_VAP", ch);
			else if (pinfo->bArg == 10)
				ch->playerData->MarkHacker("BOT_HLBOT", ch);
			else if (pinfo->bArg == 11)
				ch->playerData->MarkHacker("BOT_FSR", ch);
			else if (pinfo->bArg == 12)
				ch->playerData->MarkHacker("BOT_CUSTOM", ch);
			else if (pinfo->bArg == 13)
				ch->playerData->MarkHacker("BOT_CLOUD", ch);
			else if (pinfo->bArg == 14)
				ch->playerData->MarkHacker("BOT_MACRO", ch);
			else if (pinfo->bArg == 15)
				ch->playerData->MarkHacker("BOT_REXT", ch);
		}
	}

	const float fDist = DISTANCE_SQRT((ch->GetX() - pinfo->lX) / 100, (ch->GetY() - pinfo->lY) / 100);
	if (pinfo->bFunc == 0 && ch->GetLastMoveFunc() == 0 && fDist > 3.2f) {

		ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
		ch->Stop();
		return;
	}

	ch->SetLastMoveFunc(pinfo->bFunc);

	//ch->ChatDebug("command_move: bFunc %u, bArg %u, bRot %u, lX %ld, lY %ld, Time %d ", pinfo->bFunc, pinfo->bArg, pinfo->bRot, pinfo->lX, pinfo->lY, pinfo->dwTime);
	
	{
		// mt2009 cheatwarden
		const float fDist = DISTANCE_SQRT((ch->GetX() - pinfo->lX) / 100, (ch->GetY() - pinfo->lY) / 100);

		if (((false == ch->IsRiding() && fDist > 25) || fDist > 40) && OXEVENT_MAP_INDEX != ch->GetMapIndex())
		{
			sys_log(0, "MOVE: %s trying to move too far (dist: %.1fm) Riding(%d)", ch->GetName(), fDist, ch->IsRiding());

			ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
			ch->Stop();
			return;
		}

		const int distance = DISTANCE_APPROX((ch->GetX() - pinfo->lX), (ch->GetY() - pinfo->lY));
		if (distance < 0 || distance > REAL_MOVE_MAX_DISTANCE) {
			ch->ChatDebug("SYNC_HACK: Too great move distance (%d)", distance);
			ch->IncrementSyncHackCount();
			ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
			ch->Stop();
			return;
		}
		ch->position_before_update = ch->GetXYZ();

		PIXEL_POSITION pos;
		if (ch->IsPC() && !SECTREE_MANAGER::instance().GetMovablePosition(ch->GetMapIndex(), pinfo->lX, pinfo->lY, pos))
		{
			ch->ChatDebug("Walking on not movable position..");

			if (ch->HasPlayerData()) {
				ch->playerData->wallHackCount++;
				if (ch->playerData->wallHackCount >= 10)
					ch->GoTown();
			}
			return;
		}
		// end mt2009 cheatwarden

		// #define ENABLE_TP_SPEED_CHECK
		#ifdef ENABLE_TP_SPEED_CHECK
		const float fDist = DISTANCE_SQRT((ch->GetX() - pinfo->lX) / 100, (ch->GetY() - pinfo->lY) / 100);
		if (((false == ch->IsRiding() && fDist > 25) || fDist > 60) && OXEVENT_MAP_INDEX != ch->GetMapIndex()) // @fixme106 (changed 40 to 60)
		{
			#ifdef ENABLE_HACK_TELEPORT_LOG // @warme006
			{
				const PIXEL_POSITION & warpPos = ch->GetWarpPosition();

				if (warpPos.x == 0 && warpPos.y == 0)
					LogManager::instance().HackLog("Teleport", ch);
			}
			#endif
			sys_log(0, "MOVE: %s trying to move too far (dist: %.1fm) Riding(%d)", ch->GetName(), fDist, ch->IsRiding());

			ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
			ch->Stop();
			return;
		}
		#endif
		#ifdef ENABLE_CHECK_GHOSTMODE
		if (ch->IsPC() && ch->IsDead())
		{
			sys_log(0, "MOVE: %s trying to move as dead", ch->GetName());

			ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
			ch->Stop();
			return;
		}
		#endif

		DWORD dwCurTime = get_dword_time();

		bool CheckSpeedHack = (false == ch->GetDesc()->IsHandshaking() && dwCurTime - ch->GetDesc()->GetClientTime() > 7000);

		if (CheckSpeedHack)
		{
			int iDelta = (int) (pinfo->dwTime - ch->GetDesc()->GetClientTime());
			int iServerDelta = (int) (dwCurTime - ch->GetDesc()->GetClientTime());

			iDelta = (int) (dwCurTime - pinfo->dwTime);

			if (iDelta >= 30000)
			{
				sys_log(0, "SPEEDHACK: slow timer name %s delta %d", ch->GetName(), iDelta);
				ch->GetDesc()->DelayedDisconnect(3);
			}

			else if (iDelta < -(iServerDelta / 50))
			{
				sys_log(0, "SPEEDHACK: DETECTED! %s (delta %d %d)", ch->GetName(), iDelta, iServerDelta);
				ch->GetDesc()->DelayedDisconnect(3);
			}
		}

		if (pinfo->bFunc == FUNC_COMBO && g_bCheckMultiHack)
		{
			CheckComboHack(ch, correctArg, pinfo->dwTime, CheckSpeedHack);
		}

		if (ch->HasPlayerData())
		{
			int checkTimeBonus = ch->playerData->m_iMoveCheckTimeBonus;
			if (dwCurTime - ch->playerData->m_moveCheckerTime > 1500 + checkTimeBonus)
			{
				PIXEL_POSITION pos = ch->playerData->last_checked_position;
				int moveBonusPoint = ch->playerData->m_iMoveBonusPoint;
				int moveBonusPointThisRound = ch->playerData->m_iMoveBonusPointNextRound;
				const int checkDistance = DISTANCE_APPROX((ch->GetX() - pos.x), (ch->GetY() - pos.y));
				ch->playerData->last_checked_position = ch->GetXYZ();
				ch->playerData->m_moveCheckerTime = dwCurTime;
				ch->playerData->m_iMoveBonusPointNextRound = 0;
				ch->playerData->m_iMoveCheckTimeBonus = 0;

				DWORD averageDistance = MAX(ch->playerData->GetAllowedMovementDistance(ch, ch->IsRiding(), 1500 + checkTimeBonus) + moveBonusPointThisRound, moveBonusPoint);
				ch->playerData->m_iMaxMovementSpeedDuringCheck = 0;

				ch->playerData->m_iMoveBonusPoint = MAX(0, moveBonusPoint - checkDistance);
				int penalty = ch->playerData->m_iMoveSpeedHackPenalty;

				ch->ChatDebug("MOVE: check (%d) (avg %d) (penalty %d)", checkDistance, averageDistance, penalty);
				if (checkDistance > averageDistance)
				{
					penalty += checkDistance - averageDistance;

					if (penalty >= averageDistance)
					{
						ch->playerData->m_iMoveSpeedHackPunishmentCount++;
						ch->ChatDebug("MOVE SPEED_HACK: Moving too fast (%d) (avg %d (bonus %d) (penalty %d, punishment %d)", checkDistance, averageDistance, moveBonusPoint, penalty, ch->playerData->m_iMoveSpeedHackPunishmentCount);
						if (ch->playerData->m_iMoveSpeedHackPunishmentCount > 3)
						{
							if (ch->GetDesc())
								ch->GetDesc()->DelayedDisconnect(3);
						}
						else
						{
							ch->Show(ch->GetMapIndex(),
								pos.x,
								pos.y,
								pos.z);

							ch->Stop();
							ch->playerData->m_iMoveSpeedHackPenalty = penalty * 80 / 100; // reduce for potential sync errors
						}
						return;
					}
				}
				else
				{
					penalty = penalty * 70 / 100; // reduce penalty by 70% if right move

					if (penalty < 50)
						ch->playerData->m_iMoveSpeedHackPunishmentCount = MAX(ch->playerData->m_iMoveSpeedHackPunishmentCount--, 0);
				}

				ch->playerData->m_iMoveSpeedHackPenalty = penalty;
			}
		}
	}

	if (pinfo->bFunc != FUNC_WAIT && ch->IsBusyAction())
	{
		ch->playerData->SetBusyAction(ch, 0); // interrupt when move
	}

	if (pinfo->bFunc == FUNC_MOVE)
	{
		if (ch->GetLimitPoint(POINT_MOV_SPEED) == 0)
			return;

		ch->SetRotation(pinfo->bRot * 5);
		ch->ResetStopTime();				// ""

		ch->Goto(pinfo->lX, pinfo->lY);
	}
	else
	{
		if (pinfo->bFunc == FUNC_ATTACK || pinfo->bFunc == FUNC_COMBO)
			ch->OnMove(true);
		else if (pinfo->bFunc & FUNC_SKILL)
		{
			const int MASK_SKILL_MOTION = 0x7F;
			unsigned int motion = pinfo->bFunc & MASK_SKILL_MOTION;

			if (!ch->IsUsableSkillMotion(motion))
			{
				const char* name = ch->GetName();
				unsigned int job = ch->GetJob();
				unsigned int group = ch->GetSkillGroup();

				char szBuf[256];
				snprintf(szBuf, sizeof(szBuf), "SKILL_HACK: name=%s, job=%d, group=%d, motion=%d", name, job, group, motion);
				LogManager::instance().HackLog(szBuf, ch->GetDesc()->GetAccountTable().login, ch->GetName(), ch->GetDesc()->GetHostName());
				sys_log(0, "%s", szBuf);

				if (test_server)
				{
					ch->GetDesc()->DelayedDisconnect(number(2, 8));
					ch->ChatPacket(CHAT_TYPE_INFO, szBuf);
				}
				else
				{
					ch->GetDesc()->DelayedDisconnect(number(150, 500));
				}
			}

			ch->OnMove();
		}

		ch->SetRotation(pinfo->bRot * 5);
		ch->ResetStopTime();				// ""

		ch->Move(pinfo->lX, pinfo->lY);
		ch->Stop();
		ch->StopStaminaConsume();
	}

	TPacketGCMove pack;

	pack.bHeader      = HEADER_GC_MOVE;
	pack.bFunc        = pinfo->bFunc;
	pack.bArg         = correctArg;
	pack.bRot         = pinfo->bRot;
	pack.dwVID        = ch->GetVID();
	pack.lX           = pinfo->lX;
	pack.lY           = pinfo->lY;
	pack.dwTime       = pinfo->dwTime;
	pack.dwDuration   = (pinfo->bFunc == FUNC_MOVE) ? ch->GetCurrentMoveDuration() : 0;

	ch->PacketAround(&pack, sizeof(TPacketGCMove), ch);

	// sys_log(0,
	// 		"MOVE: %s Func:%u Arg:%u Pos:%dx%d Time:%u Dist:%.1f",
	// 		ch->GetName(),
	// 		pinfo->bFunc,
	// 		pinfo->bArg,
	// 		pinfo->lX / 100,
	// 		pinfo->lY / 100,
	// 		pinfo->dwTime,
	// 		fDist);
}

void CInputMain::Attack(LPCHARACTER ch, const BYTE header, const char* data)
{
	if (NULL == ch)
		return;

	struct type_identifier
	{
		BYTE header;
		BYTE type;
	};

	const struct type_identifier* const type = reinterpret_cast<const struct type_identifier*>(data);

	if (type->type > 0)
	{
		if (false == ch->CanUseSkill(type->type))
		{
			return;
		}

		
		if (IsShootSkill(type->type) && HEADER_CG_SHOOT != type->header)
		{
			if (test_server)
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Attack :name[%s] Vnum[%d] can't use skill by attack(warning)"), type->type);
			return;
		}
	}

	switch (header)
	{
		case HEADER_CG_ATTACK:
			{
				if (NULL == ch->GetDesc())
					return;

				const TPacketCGAttack* const packMelee = reinterpret_cast<const TPacketCGAttack*>(data);

				ch->GetDesc()->AssembleCRCMagicCube(packMelee->bCRCMagicCubeProcPiece, packMelee->bCRCMagicCubeFilePiece);

				LPCHARACTER	victim = CHARACTER_MANAGER::instance().Find(packMelee->dwVID);

				if (NULL == victim || ch == victim)
					return;

				switch (victim->GetCharType())
				{
					case CHAR_TYPE_NPC:
					case CHAR_TYPE_WARP:
					case CHAR_TYPE_GOTO:
						return;
				}

				CPlayerBotManager::instance().OnOwnerAttack(ch, victim);
                        ch->Attack(victim, packMelee->bType, type->type);
			}
			break;

		case HEADER_CG_SHOOT:
			{
				const TPacketCGShoot* const packShoot = reinterpret_cast<const TPacketCGShoot*>(data);

				ch->Shoot(packShoot->bType);
			}
			break;
	}
}

int CInputMain::SyncPosition(LPCHARACTER ch, const char * c_pcData, size_t uiBytes)
{
	const TPacketCGSyncPosition* pinfo = reinterpret_cast<const TPacketCGSyncPosition*>( c_pcData );

	if (uiBytes < pinfo->wSize)
		return -1;

	int iExtraLen = pinfo->wSize - sizeof(TPacketCGSyncPosition);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->wSize, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}

	if (0 != (iExtraLen % sizeof(TPacketCGSyncPositionElement)))
	{
		sys_err("invalid packet length %d (name: %s)", pinfo->wSize, ch->GetName());
		return iExtraLen;
	}

	int iCount = iExtraLen / sizeof(TPacketCGSyncPositionElement);

	if (iCount <= 0)
		return iExtraLen;

	static const int nCountLimit = 16;

	if( iCount > nCountLimit )
	{
		sys_err( "Too many SyncPosition Count(%d) from Name(%s)", iCount, ch->GetName() );
		iCount = nCountLimit;
	}

	TEMP_BUFFER tbuf;
	LPBUFFER lpBuf = tbuf.getptr();

	TPacketGCSyncPosition * pHeader = (TPacketGCSyncPosition *) buffer_write_peek(lpBuf);
	buffer_write_proceed(lpBuf, sizeof(TPacketGCSyncPosition));

	const TPacketCGSyncPositionElement* e =
		reinterpret_cast<const TPacketCGSyncPositionElement*>(c_pcData + sizeof(TPacketCGSyncPosition));

	timeval tvCurTime;
	gettimeofday(&tvCurTime, NULL);

	for (int i = 0; i < iCount; ++i, ++e)
	{
		LPCHARACTER victim = CHARACTER_MANAGER::instance().Find(e->dwVID);

		if (!victim)
			continue;

		switch (victim->GetCharType())
		{
			case CHAR_TYPE_NPC:
			case CHAR_TYPE_WARP:
			case CHAR_TYPE_GOTO:
				continue;
		}

		if (!victim->SetSyncOwner(ch))
			continue;

		const float fDistWithSyncOwner = DISTANCE_SQRT( (victim->GetX() - ch->GetX()) / 100, (victim->GetY() - ch->GetY()) / 100 );
		static const float fLimitDistWithSyncOwner = 2500.f + 1000.f;

		if (fDistWithSyncOwner > fLimitDistWithSyncOwner)
		{
			if (ch->GetSyncHackCount() < g_iSyncHackLimitCount)
			{
				ch->SetSyncHackCount(ch->GetSyncHackCount() + 1);
				continue;
			}
			else
			{
				LogManager::instance().HackLog( "SYNC_POSITION_HACK", ch );

				sys_err( "Too far SyncPosition DistanceWithSyncOwner(%f)(%s) from Name(%s) CH(%d,%d) VICTIM(%d,%d) SYNC(%d,%d)",
					fDistWithSyncOwner, victim->GetName(), ch->GetName(), ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY(),
					e->lX, e->lY );

				ch->GetDesc()->SetPhase(PHASE_CLOSE);

				return -1;
			}
		}

		const float fDist = DISTANCE_SQRT( (victim->GetX() - e->lX) / 100, (victim->GetY() - e->lY) / 100 );
		static const long g_lValidSyncInterval = 100 * 1000; // 100ms
		const timeval &tvLastSyncTime = victim->GetLastSyncTime();
		timeval *tvDiff = timediff(&tvCurTime, &tvLastSyncTime);

		if (tvDiff->tv_sec == 0 && tvDiff->tv_usec < g_lValidSyncInterval)
		{
			if (ch->GetSyncHackCount() < g_iSyncHackLimitCount)
			{
				ch->SetSyncHackCount(ch->GetSyncHackCount() + 1);
				continue;
			}
			else
			{
				LogManager::instance().HackLog( "SYNC_POSITION_HACK", ch );

				sys_err( "Too often SyncPosition Interval(%ldms)(%s) from Name(%s) VICTIM(%d,%d) SYNC(%d,%d)",
					tvDiff->tv_sec * 1000 + tvDiff->tv_usec / 1000, victim->GetName(), ch->GetName(), victim->GetX(), victim->GetY(),
					e->lX, e->lY );

				ch->GetDesc()->SetPhase(PHASE_CLOSE);

				return -1;
			}
		}
		else if( fDist > 25.0f )
		{
			LogManager::instance().HackLog( "SYNC_POSITION_HACK", ch );

			sys_err( "Too far SyncPosition Distance(%f)(%s) from Name(%s) CH(%d,%d) VICTIM(%d,%d) SYNC(%d,%d)",
				   	fDist, victim->GetName(), ch->GetName(), ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY(),
				  e->lX, e->lY );

			ch->GetDesc()->SetPhase(PHASE_CLOSE);

			return -1;
		}
		else
		{
			victim->SetLastSyncTime(tvCurTime);
			victim->Sync(e->lX, e->lY);
			buffer_write(lpBuf, e, sizeof(TPacketCGSyncPositionElement));
		}
	}

	if (buffer_size(lpBuf) != sizeof(TPacketGCSyncPosition))
	{
		pHeader->bHeader = HEADER_GC_SYNC_POSITION;
		pHeader->wSize = buffer_size(lpBuf);

		ch->PacketAround(buffer_read_peek(lpBuf), buffer_size(lpBuf), ch);
	}

	return iExtraLen;
}

void CInputMain::FlyTarget(LPCHARACTER ch, const char * pcData, BYTE bHeader)
{
	TPacketCGFlyTargeting * p = (TPacketCGFlyTargeting *) pcData;
	ch->MainFlyTarget(p->dwTargetVID, p->x, p->y);
}

void CInputMain::UseSkill(LPCHARACTER ch, const char * pcData)
{
	TPacketCGUseSkill * p = (TPacketCGUseSkill *) pcData;
	ch->UseSkill(p->dwVnum, CHARACTER_MANAGER::instance().Find(p->dwVID));
}

void CInputMain::ScriptButton(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptButton * p = (TPacketCGScriptButton *) c_pData;
	sys_log(0, "QUEST ScriptButton pid %d idx %u", ch->GetPlayerID(), p->idx);

	quest::PC* pc = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	if (pc && pc->IsConfirmWait())
	{
		quest::CQuestManager::instance().Confirm(ch->GetPlayerID(), quest::CONFIRM_TIMEOUT);
	}
	else if (p->idx & 0x80000000)
	{
		quest::CQuestManager::Instance().QuestInfo(ch->GetPlayerID(), p->idx & 0x7fffffff);
	}
	else
	{
		quest::CQuestManager::Instance().QuestButton(ch->GetPlayerID(), p->idx);
	}
}

void CInputMain::ScriptAnswer(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptAnswer * p = (TPacketCGScriptAnswer *) c_pData;
	sys_log(0, "QUEST ScriptAnswer pid %d answer %d", ch->GetPlayerID(), p->answer);

	if (p->answer > 250)
	{
		quest::CQuestManager::Instance().Resume(ch->GetPlayerID());
	}
	else
	{
		quest::CQuestManager::Instance().Select(ch->GetPlayerID(),  p->answer);
	}
}

// SCRIPT_SELECT_ITEM
void CInputMain::ScriptSelectItem(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptSelectItem* p = (TPacketCGScriptSelectItem*) c_pData;
	sys_log(0, "QUEST ScriptSelectItem pid %d answer %d", ch->GetPlayerID(), p->selection);
	quest::CQuestManager::Instance().SelectItem(ch->GetPlayerID(), p->selection);
}
// END_OF_SCRIPT_SELECT_ITEM

void CInputMain::QuestInputString(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGQuestInputString * p = (TPacketCGQuestInputString*) c_pData;

	char msg[65];
	strlcpy(msg, p->msg, sizeof(msg));
	sys_log(0, "QUEST InputString pid %u msg %s", ch->GetPlayerID(), msg);

	quest::CQuestManager::Instance().Input(ch->GetPlayerID(), msg);
}

void CInputMain::QuestConfirm(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGQuestConfirm* p = (TPacketCGQuestConfirm*) c_pData;
	LPCHARACTER ch_wait = CHARACTER_MANAGER::instance().FindByPID(p->requestPID);
	if (p->answer)
		p->answer = quest::CONFIRM_YES;
	sys_log(0, "QuestConfirm from %s pid %u name %s answer %d", ch->GetName(), p->requestPID, (ch_wait)?ch_wait->GetName():"", p->answer);
	if (ch_wait)
	{
		quest::CQuestManager::Instance().Confirm(ch_wait->GetPlayerID(), (quest::EQuestConfirmType) p->answer, ch->GetPlayerID());
	}
}

void CInputMain::Target(LPCHARACTER ch, const char * pcData)
{
	TPacketCGTarget * p = (TPacketCGTarget *) pcData;

	building::LPOBJECT pkObj = building::CManager::instance().FindObjectByVID(p->dwVID);

	if (pkObj)
	{
		TPacketGCTarget pckTarget{};
		pckTarget.header = HEADER_GC_TARGET;
		pckTarget.subheader = SUBHEADER_GC_TARGET_ONLY;
		pckTarget.size = sizeof(TPacketGCTarget);
		pckTarget.dwVID = p->dwVID;
		ch->GetDesc()->Packet(&pckTarget, sizeof(TPacketGCTarget));
	}
	else
		ch->SetTarget(CHARACTER_MANAGER::instance().Find(p->dwVID));
}

void CInputMain::Warp(LPCHARACTER ch, const char * pcData)
{
	ch->WarpEnd();
}

void CInputMain::SafeboxCheckin(LPCHARACTER ch, const char * c_pData)
{
	if (ch && !PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::SafeboxCheckInOut, std::chrono::milliseconds(250)))
		return;

	if (quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->IsRunning() == true)
		return;

	TPacketCGSafeboxCheckin * p = (TPacketCGSafeboxCheckin *) c_pData;

	if (!ch->CanHandleItem(false, false, BUSY_CAN_HANDLE_ITEM_EXCLUDE | BUSY_SAFEBOX))
		return;

	if (ch->IsBusy(BUSY_SAFEBOX))
		return;

	CSafebox * pkSafebox = ch->GetSafebox();
	LPITEM pkItem = ch->GetItem(p->ItemPos);

	if (!pkSafebox || !pkItem)
		return;

	if (pkItem->GetCell() >= INVENTORY_DEFAULT_MAX_NUM && IS_SET(pkItem->GetFlag(), ITEM_FLAG_IRREMOVABLE))
	{
	    ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Storage> Cannot move items in safebox."));
	    return;
	}

	if (pkItem->GetCell() >= ch->GetInventoryMaxCount())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Summon your horse to get access to horse inventory."));
		return;
	}

	if (!pkSafebox->IsEmpty(p->bSafePos, pkItem->GetSize()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Storeroom] No movement possible."));
		return;
	}

	if (pkItem->GetVnum() == UNIQUE_ITEM_SAFEBOX_EXPAND)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Storeroom] The item cannot be stored."));
		return;
	}

	if( IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_SAFEBOX) )
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Storeroom] The item cannot be stored."));
		return;
	}

	if (true == pkItem->isLocked())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Storeroom] The item cannot be stored."));
		return;
	}

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	if (pkItem->IsEquipped())
	{
		int iWearCell = pkItem->FindEquipCell(ch);
		if (iWearCell == WEAR_WEAPON)
		{
			LPITEM costumeWeapon = ch->GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && !ch->UnequipItem(costumeWeapon))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot unequip the costume weapon. Not enough space."));
				return;
			}
		}
	}
#endif

	// @fixme140 BEGIN
	if (ITEM_BELT == pkItem->GetType() && CBeltInventoryHelper::IsExistItemInBeltInventory(ch))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can only discard the belt when there are no longer any items in its inventory."));
		return;
	}
	// @fixme140 END

	pkItem->RemoveFromCharacter();
	if (!pkItem->IsDragonSoul())
		ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, p->ItemPos.cell, 255);
	pkSafebox->Add(p->bSafePos, pkItem);

	char szHint[128];
	snprintf(szHint, sizeof(szHint), "%s %u", pkItem->GetName(), pkItem->GetCount());
	LogManager::instance().ItemLog(ch, pkItem, "SAFEBOX PUT", szHint);
}

void CInputMain::SafeboxCheckout(LPCHARACTER ch, const char * c_pData, bool bMall)
{
	if (ch && !PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::SafeboxCheckInOut, std::chrono::milliseconds(250)))
		return;

	TPacketCGSafeboxCheckout * p = (TPacketCGSafeboxCheckout *) c_pData;

	if (ch && !ch->CanHandleItem(false, false, BUSY_CAN_HANDLE_ITEM_EXCLUDE | BUSY_SAFEBOX))
		return;

	if (quest::CQuestManager::instance().GetEventFlag("block_safebox") > 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This feature is temporarily unavailable."));
		return;
	}

	if (ch && g_bChannel > 90)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot do that on special channel."));
		return;
	}

	CSafebox * pkSafebox;

	if (bMall)
		pkSafebox = ch->GetMall();
	else
		pkSafebox = ch->GetSafebox();

	if (!pkSafebox)
		return;

	LPITEM pkItem = pkSafebox->Get(p->bSafePos);

	if (!pkItem)
		return;

	if (!ch->IsEmptyItemGrid(p->ItemPos, pkItem->GetSize()))
		return;

	if (pkItem->IsDragonSoul())
	{
		if (bMall)
		{
			DSManager::instance().DragonSoulItemInitialize(pkItem);
		}

		if (DRAGON_SOUL_INVENTORY != p->ItemPos.window_type)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Storeroom] No movement possible."));
			return;
		}

		TItemPos DestPos = p->ItemPos;
		if (!DSManager::instance().IsValidCellForThisItem(pkItem, DestPos))
		{
			int iCell = ch->GetEmptyDragonSoulInventory(pkItem);
			if (iCell < 0)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Storeroom] No movement possible."));
				return ;
			}
			DestPos = TItemPos (DRAGON_SOUL_INVENTORY, iCell);
		}

		pkSafebox->Remove(p->bSafePos);
		pkItem->AddToCharacter(ch, DestPos);
		ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
	}
	else
	{
		if (DRAGON_SOUL_INVENTORY == p->ItemPos.window_type)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Storeroom] No movement possible."));
			return;
		}
		// @fixme119
		if (p->ItemPos.IsBeltInventoryPosition() && false == CBeltInventoryHelper::CanMoveIntoBeltInventory(pkItem))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot equip this item in your belt inventory."));
			return;
		}

		pkSafebox->Remove(p->bSafePos);
		pkItem->AddToCharacter(ch, p->ItemPos);
		ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
	}

	DWORD dwID = pkItem->GetID();
	db_clientdesc->DBPacketHeader(HEADER_GD_ITEM_FLUSH, 0, sizeof(DWORD));
	db_clientdesc->Packet(&dwID, sizeof(DWORD));

	char szHint[128];
	snprintf(szHint, sizeof(szHint), "%s %u", pkItem->GetName(), pkItem->GetCount());
	if (bMall)
		LogManager::instance().ItemLog(ch, pkItem, "MALL GET", szHint);
	else
		LogManager::instance().ItemLog(ch, pkItem, "SAFEBOX GET", szHint);
}

void CInputMain::SafeboxItemMove(LPCHARACTER ch, const char * data)
{
	struct command_item_move * pinfo = (struct command_item_move *) data;

	if (!ch->CanHandleItem(false, false, BUSY_CAN_HANDLE_ITEM_EXCLUDE | BUSY_SAFEBOX))
		return;

	if (!ch->GetSafebox())
		return;

	if (!PulseManager::Instance().IncreaseCount(ch->GetPlayerID(), ePulse::SafeboxMove, std::chrono::milliseconds(500), 5))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("NEXT_ACTION_WAIT_NO_FORMAT"));
		return;
	}

	ch->GetSafebox()->MoveItem(pinfo->Cell.cell, pinfo->CellTo.cell);
}

// PARTY_JOIN_BUG_FIX
void CInputMain::PartyInvite(LPCHARACTER ch, const char * c_pData)
{
	if (ch && !PulseManager::Instance().IncreaseClock(ch->GetPlayerID(), ePulse::PartyInvite, std::chrono::milliseconds(1000))) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("NEXT_ACTION_WAIT_NO_FORMAT"));
		return;
	}

	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this in the duel arena."));
		return;
	}

	TPacketCGPartyInvite * p = (TPacketCGPartyInvite*) c_pData;

	LPCHARACTER pInvitee = CHARACTER_MANAGER::instance().Find(p->vid);

	if (!pInvitee || !ch->GetDesc() || !pInvitee->GetDesc())
	{
		sys_err("PARTY Cannot find invited character");
		return;
	}

	ch->PartyInvite(pInvitee);
}

void CInputMain::PartyInviteAnswer(LPCHARACTER ch, const char * c_pData)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this in the duel arena."));
		return;
	}

	TPacketCGPartyInviteAnswer * p = (TPacketCGPartyInviteAnswer*) c_pData;

	LPCHARACTER pInviter = CHARACTER_MANAGER::instance().Find(p->leader_vid);

	if (!pInviter)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] The player who invited you is not online."));
	// @fixme301 BEGIN
	else if (!pInviter->GetParty() && pInviter->GetMapIndex() >= 10000)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Party> The party leader is inside a dungeon."));
	else if (pInviter->GetParty() && pInviter->GetParty()->IsPartyInAnyDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Party> The party can't invite people if members are inside a dungeon."));
		pInviter->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Party> The party can't invite people if members are inside a dungeon."));
	}
	// @fixme301 END
	else if (!p->accept)
		pInviter->PartyInviteDeny(ch->GetPlayerID());
	else
		pInviter->PartyInviteAccept(ch);
}
// END_OF_PARTY_JOIN_BUG_FIX

void CInputMain::PartySetState(LPCHARACTER ch, const char* c_pData)
{
	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] The server cannot execute this group request."));
		return;
	}

	TPacketCGPartySetState* p = (TPacketCGPartySetState*) c_pData;

	if (!ch->GetParty())
		return;

	if (ch->GetParty()->GetLeaderPID() != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] Only the group leader can change this."));
		return;
	}

	if (!ch->GetParty()->IsMember(p->pid))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] The target is not a member of your group."));
		return;
	}

	DWORD pid = p->pid;
	sys_log(0, "PARTY SetRole pid %d to role %d state %s", pid, p->byRole, p->flag ? "on" : "off");

	switch (p->byRole)
	{
		case PARTY_ROLE_NORMAL:
			break;

		case PARTY_ROLE_ATTACKER:
		case PARTY_ROLE_TANKER:
		case PARTY_ROLE_BUFFER:
		case PARTY_ROLE_SKILL_MASTER:
		case PARTY_ROLE_HASTE:
		case PARTY_ROLE_DEFENDER:
			if (ch->GetParty()->SetRole(pid, p->byRole, p->flag))
			{
				TPacketPartyStateChange pack;
				pack.dwLeaderPID = ch->GetPlayerID();
				pack.dwPID = p->pid;
				pack.bRole = p->byRole;
				pack.bFlag = p->flag;
				db_clientdesc->DBPacket(HEADER_GD_PARTY_STATE_CHANGE, 0, &pack, sizeof(pack));
			}

			break;

		default:
			sys_err("wrong byRole in PartySetState Packet name %s state %d", ch->GetName(), p->byRole);
			break;
	}
}

void CInputMain::PartyRemove(LPCHARACTER ch, const char* c_pData)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this in the duel arena."));
		return;
	}

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] The server cannot execute this group request."));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] You cannot kick out a player while you are in a dungeon."));
		return;
	}

	TPacketCGPartyRemove* p = (TPacketCGPartyRemove*) c_pData;

	if (!ch->GetParty())
		return;

	LPPARTY pParty = ch->GetParty();
	if (pParty->GetLeaderPID() == ch->GetPlayerID())
	{
		if (ch->GetDungeon())
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] You cannot kick out a player while you are in a dungeon."));
		else
		{
			if (pParty->IsPartyInAnyDungeon()) // @fixme301
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Party> The party cannot be disbanded because there are party members inside the dungeon."));
				return;
			}

			// leader can remove any member
			if (p->pid == ch->GetPlayerID() || pParty->GetMemberCount() == 2)
			{
				// party disband
				CPartyManager::instance().DeleteParty(pParty);
			}
			else
			{
				LPCHARACTER B = CHARACTER_MANAGER::instance().FindByPID(p->pid);
				if (B)
				{
					//pParty->SendPartyRemoveOneToAll(B);
					B->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] You have been out kicked of the group."));
					//pParty->Unlink(B);
					//CPartyManager::instance().SetPartyMember(B->GetPlayerID(), NULL);
				}
				pParty->Quit(p->pid);
			}
		}
	}
	else
	{
		// otherwise, only remove itself
		if (p->pid == ch->GetPlayerID())
		{
			if (ch->GetDungeon())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] You cannot leave a group while you are in a dungeon."));
			}
			else
			{
				if (pParty->GetMemberCount() == 2)
				{
					// party disband
					CPartyManager::instance().DeleteParty(pParty);
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] You have left the group."));
					//pParty->SendPartyRemoveOneToAll(ch);
					pParty->Quit(ch->GetPlayerID());
					//pParty->SendPartyRemoveAllToOne(ch);
					//CPartyManager::instance().SetPartyMember(ch->GetPlayerID(), NULL);
				}
			}
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] You cannot kick out group members."));
		}
	}
}

void CInputMain::AnswerMakeGuild(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGAnswerMakeGuild* p = (TPacketCGAnswerMakeGuild*) c_pData;

	YANG GuildCreateFee = 200000;

	if (ch->GetGold() < GuildCreateFee)
		return;

	if (get_global_time() - ch->GetQuestFlag("guild_manage.new_disband_time") <
			CGuildManager::instance().GetDisbandDelay())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] After disbanding a guild, you cannot create a new one for %d days."),
				quest::CQuestManager::instance().GetEventFlag("guild_disband_delay"));
		return;
	}

	if (get_global_time() - ch->GetQuestFlag("guild_manage.new_withdraw_time") <
			CGuildManager::instance().GetWithdrawDelay())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] After leaving a guild, you cannot create a new one for %d days."),
				quest::CQuestManager::instance().GetEventFlag("guild_withdraw_delay"));
		return;
	}

	if (ch->GetGuild())
		return;

	auto npcVid = ch->GetSpecialFlag("make_guild_vid");
	if (npcVid == 0)
	{
		ch->ChatDebug("Zagadaj do NPC...");
		return;
	}

	LPCHARACTER npc = CHARACTER_MANAGER::instance().Find(npcVid);
	if (!npc)
	{
		ch->ChatDebug("NPC nie istnieje.");
		return;
	}

	if (ch->DistanceTo(npc) > 2000)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are too far away."));
		return;
	}

	CGuildManager& gm = CGuildManager::instance();

	TGuildCreateParameter cp{};
	cp.master = ch;
	strlcpy(cp.name, p->guild_name, sizeof(cp.name));

	if (cp.name[0] == 0 || !check_name(cp.name))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This guild name is invalid."));
		return;
	}

	DWORD dwGuildID = gm.CreateGuild(cp);
	if (dwGuildID)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] [%s] guild has been created."), cp.name);

		ch->ChangeGold(-GuildCreateFee);
		DBManager::instance().SendMoneyLog(MONEY_LOG_GUILD, ch->GetPlayerID(), -GuildCreateFee);

		char Log[128];
		snprintf(Log, sizeof(Log), "GUILD_NAME %s MASTER %s", cp.name, ch->GetName());
		LogManager::instance().CharLog(ch, 0, "MAKE_GUILD", Log);

		ch->RemoveSpecifyItem(GUILD_CREATE_ITEM_VNUM, 1);
		#ifdef ENABLE_GUILD_TOKEN_AUTH
		CGuildManager::instance().GuildRelink(dwGuildID, ch);
		ch->SendGuildToken();
		#endif
	}
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] Creation of the guild has failed."));
}

void CInputMain::PartyUseSkill(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGPartyUseSkill* p = (TPacketCGPartyUseSkill*) c_pData;
	if (!ch->GetParty())
		return;

	if (ch->GetPlayerID() != ch->GetParty()->GetLeaderPID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] Only the group leader can use group skills."));
		return;
	}

	switch (p->bySkillIndex)
	{
		case PARTY_SKILL_HEAL:
			ch->GetParty()->HealParty();
			break;
		case PARTY_SKILL_WARP:
			{
				LPCHARACTER pch = CHARACTER_MANAGER::instance().Find(p->vid);
				if (pch)
					ch->GetParty()->SummonToLeader(pch->GetPlayerID());
				else
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] The target has not been found."));
			}
			break;
	}
}

void CInputMain::PartyParameter(LPCHARACTER ch, const char * c_pData)
{
	TPacketCGPartyParameter * p = (TPacketCGPartyParameter *) c_pData;

	if (ch->GetParty())
		ch->GetParty()->SetParameter(p->bDistributeMode);
}

size_t GetSubPacketSize(const GUILD_SUBHEADER_CG& header)
{
	switch (header)
	{
		case GUILD_SUBHEADER_CG_DEPOSIT_MONEY:				return sizeof(int);
		case GUILD_SUBHEADER_CG_WITHDRAW_MONEY:				return sizeof(YANG);
		case GUILD_SUBHEADER_CG_ADD_MEMBER:					return sizeof(DWORD);
		case GUILD_SUBHEADER_CG_REMOVE_MEMBER:				return sizeof(DWORD);
		case GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME:			return 10;
		case GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY:		return sizeof(BYTE) + sizeof(BYTE);
		case GUILD_SUBHEADER_CG_OFFER:						return sizeof(DWORD);
		case GUILD_SUBHEADER_CG_CHARGE_GSP:					return sizeof(int);
		case GUILD_SUBHEADER_CG_POST_COMMENT:				return 1;
		case GUILD_SUBHEADER_CG_DELETE_COMMENT:				return sizeof(DWORD);
		case GUILD_SUBHEADER_CG_REFRESH_COMMENT:			return 0;
		case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE:		return sizeof(DWORD) + sizeof(BYTE);
		case GUILD_SUBHEADER_CG_USE_SKILL:					return sizeof(TPacketCGGuildUseSkill);
		case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL:		return sizeof(DWORD) + sizeof(BYTE);
		case GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER:		return sizeof(DWORD) + sizeof(BYTE);
	}

	return 0;
}

int CInputMain::Guild(LPCHARACTER ch, const char * data, size_t uiBytes)
{
	if (uiBytes < sizeof(TPacketCGGuild))
		return -1;

	const TPacketCGGuild* p = reinterpret_cast<const TPacketCGGuild*>(data);
	const char* c_pData = data + sizeof(TPacketCGGuild);

	uiBytes -= sizeof(TPacketCGGuild);

	const GUILD_SUBHEADER_CG SubHeader = static_cast<GUILD_SUBHEADER_CG>(p->subheader);
	const size_t SubPacketLen = GetSubPacketSize(SubHeader);

	if (uiBytes < SubPacketLen)
	{
		return -1;
	}

	CGuild* pGuild = ch->GetGuild();

	if (NULL == pGuild)
	{
		if (SubHeader != GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] It does not belong to the guild."));
			return SubPacketLen;
		}
	}

	if (ch && !PulseManager::Instance().IncreaseCount(ch->GetPlayerID(), ePulse::Guild, std::chrono::milliseconds(500), 5)) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("NEXT_ACTION_WAIT_NO_FORMAT"));
		return SubPacketLen;
	}

	switch (SubHeader)
	{
		case GUILD_SUBHEADER_CG_DEPOSIT_MONEY:
			{
				return SubPacketLen; // return on start so its unavailable

				const int gold = MIN(*reinterpret_cast<const int*>(c_pData), __deposit_limit());

				if (gold < 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] That is not the correct amount of Yang."));
					return SubPacketLen;
				}

				if (ch->GetGold() < gold)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] You do not have enough Yang."));
					return SubPacketLen;
				}

				pGuild->RequestDepositMoney(ch, gold);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_WITHDRAW_MONEY:
			{
				const YANG gold = MAX(*reinterpret_cast<const YANG*>(c_pData), 500000);

				if (gold < 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] That is not the correct amount of Yang."));
					return SubPacketLen;
				}

				pGuild->RequestWithdrawMoney(ch, gold);
				return SubPacketLen;
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_ADD_MEMBER:
			{
				const DWORD vid = *reinterpret_cast<const DWORD*>(c_pData);
				LPCHARACTER newmember = CHARACTER_MANAGER::instance().Find(vid);

				if (!newmember)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] The person you were searching for cannot be found."));
					return SubPacketLen;
				}

				// @fixme145 BEGIN (+newmember ispc check)
				if (!ch->IsPC() || !newmember->IsPC())
					return SubPacketLen;
				// @fixme145 END

				pGuild->Invite(ch, newmember);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_REMOVE_MEMBER:
			{
				if (pGuild->UnderAnyWar() != 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Guild> During a guild war, you cannot kick out other guild members."));
					return SubPacketLen;
				}

				const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				LPCHARACTER member = CHARACTER_MANAGER::instance().FindByPID(pid);

				if (member)
				{
					if (member->GetGuild() != pGuild)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] This person is not in the same guild."));
						return SubPacketLen;
					}

					if (!pGuild->HasGradeAuth(m->grade, GUILD_AUTH_REMOVE_MEMBER))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] You do not have the authority to kick out guild members."));
						return SubPacketLen;
					}

					member->SetQuestFlag("guild_manage.new_withdraw_time", get_global_time());
					pGuild->RequestRemoveMember(member->GetPlayerID());

					if (g_bGuildInviteLimit)
					{
						DBManager::instance().Query("REPLACE INTO guild_invite_limit VALUES(%d, %d)", pGuild->GetID(), get_global_time());
					}
				}
				else
				{
					if (!pGuild->HasGradeAuth(m->grade, GUILD_AUTH_REMOVE_MEMBER))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] You do not have the authority to kick out guild members."));
						return SubPacketLen;
					}

					if (pGuild->RequestRemoveMember(pid))
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] You have kicked a guild member out."));
					else
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] The person you were searching for cannot be found."));
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME:
			{
				char gradename[GUILD_GRADE_NAME_MAX_LEN + 1];
				strlcpy(gradename, c_pData + 1, sizeof(gradename));

				const TGuildMember * m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				if (m->grade != GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] You do not have the authority to change your rank name."));
				}
				else if (*c_pData == GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] The guild leader's rights cannot be changed."));
				}
				else if (!check_name(gradename))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] This rank name is invalid."));
				}
				else
				{
					pGuild->ChangeGradeName(*c_pData, gradename);
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY:
			{
				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				if (m->grade != GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] You do not have the authority to change your position."));
				}
				else if (*c_pData == GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] The rights of the guild leader cannot be changed."));
				}
				else
				{
					pGuild->ChangeGradeAuth(*c_pData, *(c_pData + 1));
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_OFFER:
			{
				DWORD offer = *reinterpret_cast<const DWORD*>(c_pData);

				if (pGuild->GetLevel() >= GUILD_MAX_LEVEL)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<Guild> The Guild has already reached maximum level."));
				}
				else
				{
					offer /= 100;
					offer *= 100;

					if (pGuild->OfferExp(ch, offer))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] %u experience points used."), offer);
					}
					else
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] Experience usage has failed."));
					}
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_CHARGE_GSP:
			{
				const int offer = *reinterpret_cast<const int*>(c_pData);
				const int gold = offer * 100;

				if (offer < 0 || gold < offer || gold < 0 || ch->GetGold() < gold)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] Insufficient Yang in the guild treasury."));
					return SubPacketLen;
				}

				if (!pGuild->ChargeSP(ch, offer))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] Dragon ghost was not restored."));
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_POST_COMMENT:
			{
				const size_t length = *c_pData;

				if (length > GUILD_COMMENT_MAX_LEN)
				{
					sys_err("POST_COMMENT: %s comment too long (length: %u)", ch->GetName(), length);
					ch->GetDesc()->SetPhase(PHASE_CLOSE);
					return -1;
				}

				if (uiBytes < 1 + length)
					return -1;

				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				if (length && !pGuild->HasGradeAuth(m->grade, GUILD_AUTH_NOTICE) && *(c_pData + 1) == '!')
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] You do not have the authority to make an announcement."));
				}
				else
				{
					std::string str(c_pData + 1, length);
					pGuild->AddComment(ch, str);
				}

				return (1 + length);
			}

		case GUILD_SUBHEADER_CG_DELETE_COMMENT:
			{
				const DWORD comment_id = *reinterpret_cast<const DWORD*>(c_pData);

				pGuild->DeleteComment(ch, comment_id);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_REFRESH_COMMENT:
			pGuild->RefreshComment(ch);
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE:
			{
				const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
				const BYTE grade = *(c_pData + sizeof(DWORD));
				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				if (m->grade != GUILD_LEADER_GRADE)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] You do not have the authority to change the position."));
				else if (ch->GetPlayerID() == pid)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] The guild leader's position cannot be changed."));
				else if (grade == 1)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] You cannot make yourself guild leader."));
				else
					pGuild->ChangeMemberGrade(pid, grade);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_USE_SKILL:
			{
				const TPacketCGGuildUseSkill* p = reinterpret_cast<const TPacketCGGuildUseSkill*>(c_pData);

				pGuild->UseSkill(p->dwVnum, ch, p->dwPID);
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL:
			{
				const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
				const BYTE is_general = *(c_pData + sizeof(DWORD));
				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return -1;

				if (m->grade != GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] You do not have the authority to choose the guild leader."));
				}
				else
				{
					if (!pGuild->ChangeMemberGeneral(pid, is_general))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] You cannot choose any more guild leaders."));
					}
				}
			}
			return SubPacketLen;

		case GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER:
			{
				const DWORD guild_id = *reinterpret_cast<const DWORD*>(c_pData);
				const BYTE accept = *(c_pData + sizeof(DWORD));

				CGuild * g = CGuildManager::instance().FindGuild(guild_id);

				if (g)
				{
					if (accept)
						g->InviteAccept(ch);
					else
						g->InviteDeny(ch->GetPlayerID());
				}
			}
			return SubPacketLen;

	}

	return 0;
}

void CInputMain::Fishing(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGFishing* p = (TPacketCGFishing*)c_pData;
	if (ch && ch->HasPlayerData() && p->focusHWND != p->pythonApplicationHWND)
		ch->playerData->MarkHacker("BOT_FISHBOT", ch);

	ch->SetRotation(p->dir * 5);
	ch->fishing();
	return;
}

void CInputMain::ItemGive(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGGiveItem* p = (TPacketCGGiveItem*) c_pData;
	LPCHARACTER to_ch = CHARACTER_MANAGER::instance().Find(p->dwTargetVID);

	if (to_ch)
		ch->GiveItem(to_ch, p->ItemPos);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade this item."));
}

void CInputMain::Hack(LPCHARACTER ch, const char * c_pData)
{
	TPacketCGHack * p = (TPacketCGHack *) c_pData;

	char buf[sizeof(p->szBuf)];
	strlcpy(buf, p->szBuf, sizeof(buf));

	sys_err("HACK_DETECT: %s %s", ch->GetName(), buf);

	ch->GetDesc()->SetPhase(PHASE_CLOSE);
}

void CInputMain::RequestInfoResponse(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGRequestInfoResponse* p = (TPacketCGRequestInfoResponse*) c_pData;
	if (ch->HasPlayerData() && ch->playerData->IsRequestInfoResponseWaiting(p->type))
	{
		auto key = "A54oaPzVj8VDOwwa";

		auto decrypted_arg1 = decrypt_aes_hash(p->arg1, key);
		auto decrypted_arg2 = decrypt_aes_hash(p->arg2, key);

		ch->ChatDebug("dostalem request info response %s i %s", decrypted_arg1.c_str(), decrypted_arg2.c_str());

		ch->playerData->SetRequestInfoResponseState(p->type, REQUEST_INFO_STATE_RECEIVED);

		if (p->type == REQUEST_INFO_HACK && decrypted_arg1 == "ALLGOOD")
			return;
			
		LogManager::instance().RequestInfoLog(ch->GetPlayerID(), p->type, decrypted_arg1.c_str(), decrypted_arg2.c_str());
	}
}

int CInputMain::MyShop(LPCHARACTER ch, const char * c_pData, size_t uiBytes)
{
	TPacketCGMyShop * p = (TPacketCGMyShop *) c_pData;
	int iExtraLen = p->bCount * sizeof(TShopItemTable);

	if (uiBytes < sizeof(TPacketCGMyShop) + iExtraLen)
		return -1;

	if (g_bChannel >= 99) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_OX_ERROR"));
		return (iExtraLen);
	}

	if (quest::CQuestManager::instance().GetEventFlag("shop_off") == 1)
		if (ch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_TEMP_OFF"));
			return (iExtraLen);
		}

	if (ch->GetGold() >= GOLD_MAX)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have more than 2 Billion Yang with you. You cannot trade."));
		sys_log(0, "MyShop ==> OverFlow Gold id %u name %s ", ch->GetPlayerID(), ch->GetName());
		return (iExtraLen);
	}

	if (ch->IsStun() || ch->IsDead())
		return (iExtraLen);

	if (ch->IsBusy())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot open a private shop while another window is open."));
		return (iExtraLen);
	}

	sys_log(0, "MyShop time %u count %d", p->bTimeIndex, p->bCount);
	ch->OpenMyShop(p->szSign, (TShopItemTable *) (c_pData + sizeof(TPacketCGMyShop)), p->bCount, p->bTimeIndex);
	return (iExtraLen);
}

void CInputMain::Refine(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGRefine* p = reinterpret_cast<const TPacketCGRefine*>(c_pData);

	if (ch->IsBusy())
	{
		ch->ChatPacket(CHAT_TYPE_INFO,  LC_TEXT("You cannot upgrade anything while another window is open."));
		ch->ClearRefineMode();
		return;
	}

	// fix bug: if fast clicked with autowindow open it could somehow upgrade with jumping above refine level like from +4 to +6 without taking money or upgrade items for +5
	int iPulse = thecore_pulse();
	if (iPulse - ch->GetRefineTime() < PASSES_PER_SEC(1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("WAIT_BEFORE_NEXT_REFINE"));
		ch->ClearRefineMode();
		return;
	}

	if (p->type == 255)
	{
		// DoRefine Cancel
		ch->ClearRefineMode();
		return;
	}

	if (p->pos >= INVENTORY_MAX_NUM)
	{
		ch->ClearRefineMode();
		return;
	}

	LPITEM item = ch->GetInventoryItem(p->pos);

	if (!item)
	{
		ch->ClearRefineMode();
		return;
	}

	if (item->GetCell() >= ch->GetInventoryMaxCount())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Summon your horse to get access to horse inventory."));
		ch->ClearRefineMode();
		return;
	}

	if (item->GetType() == ITEM_ROD && p->type != REFINE_TYPE_FISHER)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ITEM_ROD_REFINE_ONLY_FISHER"));
		ch->ClearRefineMode();
		return;
	}

	if (item->GetType() == ITEM_HERB_KNIFE && p->type != REFINE_TYPE_HERB_KNIFE)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ITEM_ROD_REFINE_ONLY_HERBALIST"));
		ch->ClearRefineMode();
		return;
	}

	if (item->GetType() == ITEM_PICK && p->type != REFINE_TYPE_PICKAXE)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ITEM_PICKAXE_REFINE_ONLY_DEOKBAE"));
		ch->ClearRefineMode();
		return;
	}

	ch->SetRefineTime();

	if (p->type == REFINE_TYPE_NORMAL ||
		p->type == REFINE_TYPE_FISHER ||
		p->type == REFINE_TYPE_HERB_KNIFE ||
		p->type == REFINE_TYPE_PICKAXE)
	{
		if (ch->GetRefineNPCVID() == 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("An error has occurred. code %s"), "refine_normal_no_vid");
			ch->ClearRefineMode();
			return;
		}

		LPCHARACTER refineCh = CHARACTER_MANAGER::instance().Find(ch->GetRefineNPCVID());
		if (!refineCh) {
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("An error has occurred. code %s"), "refine_normal_no_refine_ch");
			ch->ClearRefineMode();
			return;
		}
		int distance = DISTANCE_APPROX((ch->GetX() - refineCh->GetX()), (ch->GetY() - refineCh->GetY()));
		if (distance > 2000)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("TOO_FAR_FROM_REFINE_NPC"));
			ch->ClearRefineMode();
			return;
		}

		sys_log(0, "refine_type: %u", p->type);
		ch->DoRefine(item, false, p->type);
	}
	else if (p->type == REFINE_TYPE_SCROLL || p->type == REFINE_TYPE_NO_REDUCTION_WHEN_FAIL || p->type == REFINE_TYPE_UP_TO_3TH_LEVEL || p->type == REFINE_TYPE_BDRAGON)
	{
		sys_log(0, "refine_type_scroll, ...");
		ch->DoRefineWithScroll(item);
	}
	else if (p->type == REFINE_TYPE_MONEY_ONLY)
	{
		const LPITEM item = ch->GetInventoryItem(p->pos);

		if (NULL != item)
		{
			if (IsEliteRefine(item->GetRefineSet()))
			{
				LogManager::instance().HackLog("DEVIL_TOWER_REFINE_HACK", ch);
			}
			else
			{
				if (ch->GetQuestFlag("deviltower_zone.can_refine") > 0) // @fixme158 (allow multiple refine attempts)
				{
					if (ch->DoRefine(item, true))
						ch->SetQuestFlag("deviltower_zone.can_refine", ch->GetQuestFlag("deviltower_zone.can_refine") - 1);
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "You can only be rewarded once for the Demon Tower Quest.");
				}
			}
		}
	}

	ch->ClearRefineMode();
}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
void CInputMain::Acce(LPCHARACTER pkChar, const char* c_pData)
{
	quest::PC * pPC = quest::CQuestManager::instance().GetPCForce(pkChar->GetPlayerID());
	if (pPC->IsRunning())
		return;

	TPacketAcce * sPacket = (TPacketAcce*) c_pData;
	switch (sPacket->subheader)
	{
	case ACCE_SUBHEADER_CG_CLOSE:
	{
		pkChar->CloseAcce();
	}
	break;
	case ACCE_SUBHEADER_CG_ADD:
	{
		pkChar->AddAcceMaterial(sPacket->tPos, sPacket->bPos);
	}
	break;
	case ACCE_SUBHEADER_CG_REMOVE:
	{
		pkChar->RemoveAcceMaterial(sPacket->bPos);
	}
	break;
	case ACCE_SUBHEADER_CG_REFINE:
	{
		pkChar->RefineAcceMaterials();
	}
	break;
	default:
		break;
	}
}
#endif
#ifdef ENABLE_IKASHOP_RENEWAL
#include "ikarus_shop.h"
#include "ikarus_shop_manager.h"

struct DecodeException : public std::exception {
	const char* what() const noexcept override{
		return "Decode failed receiving ikashop packet";
	}
};

template <class T>
const T& Decode(const char*& data, int& left, int& len)
{
	if(left < sizeof(T))
		throw DecodeException();

	auto obj = reinterpret_cast<const T*>(data);
	data += sizeof(T);
	len  += sizeof(T);
	left -= sizeof(T);
	return *obj;
}

int IkarusShopPacketReopenShop(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGShopReopen>(data, left, extra);
	ikashop::GetManager().RecvShopReopenClientPacket(ch, pack.name, pack.bTimeIndex);
	return extra;
}

int IkarusShopPacketForceCloseShop(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	ikashop::GetManager().RecvShopForceCloseClientPacket(ch);
	return 0;
}

int IkarusShopPacketOpenShop(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGShopOpen>(data, left, extra);
	ikashop::GetManager().RecvShopOpenClientPacket(ch, pack.ownerid);
	return extra;
}

int IkarusShopPacketOpenShowOwner(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	ikashop::GetManager().RecvShopOpenMyShopClientPacket(ch);
	return 0;
}

int IkarusShopPacketBuyItem(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGShopBuyItem>(data, left, extra);
	ikashop::GetManager().RecvShopBuyItemClientPacket(ch, pack.ownerid, pack.itemid, pack.searching, pack.seenprice);
	return extra;
}

int IkarusShopPacketAddItem(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGAddItem>(data, left, extra);
	ikashop::GetManager().RecvShopAddItemClientPacket(ch, pack.pos, pack.price, pack.destpos);
	return extra;
}

int IkarusShopPacketRemoveItem(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGRemoveItem>(data, left, extra);
	ikashop::GetManager().RecvShopRemoveItemClientPacket(ch, pack.itemid);
	return extra;
}

int IkarusShopPacketRemoveAllItem(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGRemoveItem>(data, left, extra);
	ikashop::GetManager().RecvShopRemoveAllItemClientPacket(ch);
	return extra;
}

int IkarusShopPacketEditItem(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGEditItem>(data, left, extra);
	ikashop::GetManager().RecvShopEditItemClientPacket(ch, pack.itemid, pack.price);
	return extra;
}

int IkarusShopPacketChangeName(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGChangeName>(data, left, extra);
	ikashop::GetManager().RecvShopChangeNameClientPacket(ch, pack.name);
	return extra;
}

int IkarusShopPacketFilterRequest(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGFilterRequest>(data, left, extra);
	ikashop::GetManager().RecvShopFilterRequestClientPacket(ch, pack.filter);
	return extra;
}

int IkarusShopPacketSearchFillRequest(LPCHARACTER ch, const char* data, int left)
{
	ikashop::GetManager().RecvShopSearchFillRequestClientPacket(ch);
	return 0;
}

int IkarusShopPacketCreateOffer(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGOfferCreate>(data, left, extra);
	ikashop::GetManager().RecvShopCreateOfferClientPacket(ch, pack.offer);
	return extra;
}

int IkarusShopPacketAcceptOffer(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGOfferAccept>(data, left, extra);
	ikashop::GetManager().RecvShopAcceptOfferClientPacket(ch, pack.offerid);
	return extra;
}

int IkarusShopPacketOfferCancel(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGOfferCancel>(data, left, extra);
	ikashop::GetManager().RecvShopCancelOfferClientPacket(ch, pack.offerid, pack.ownerid);
	return extra;
}

int IkarusShopPacketOfferListRequest(LPCHARACTER ch)
{
	ikashop::GetManager().RecvOfferListRequestPacket(ch);
	return 0;
}

int IkarusShopPacketOpenSafebox(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	ikashop::GetManager().RecvShopSafeboxOpenClientPacket(ch);
	return 0;
}

int IkarusShopPacketCloseMyShopBoard(LPCHARACTER ch)
{
	ikashop::GetManager().RecvCloseMyShopBoardClientPacket(ch);
	return 0;
}

int IkarusShopPacketCloseOfferList(LPCHARACTER ch)
{
	ikashop::GetManager().RecvCloseOfferListClientPacket(ch);
	return 0;
}

int IkarusShopPacketCloseShopGuest(LPCHARACTER ch)
{
	ikashop::GetManager().RecvCloseShopGuestClientPacket(ch);
	return 0;
}

int IkarusShopPacketGetItemSafebox(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGShopSafeboxGetItem>(data, left, extra);
	ikashop::GetManager().RecvShopSafeboxGetItemClientPacket(ch, pack.itemid);
	return extra;

}

int IkarusShopPacketGetValutesSafebox(LPCHARACTER ch, const char* data, int left)
{
	ikashop::GetManager().RecvShopSafeboxGetValutesClientPacket(ch);
	return 0;
}

int IkarusShopPacketCloseSafebox(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	ikashop::CShopManager& rManager = ikashop::GetManager();
	rManager.RecvShopSafeboxCloseClientPacket(ch);
	return 0;
}

int IkarusShopPacketListRequest(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	ikashop::CShopManager& rManager = ikashop::GetManager();
	rManager.RecvAuctionListRequestClientPacket(ch);
	return 0;
}

int IkarusShopPacketOpenAuctionRequest(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGAuctionOpenRequest>(data, left, extra);
	ikashop::GetManager().RecvAuctionOpenRequestClientPacket(ch, pack.ownerid);
	return extra;
}

int IkarusShopPacketOpenMyAuctionRequest(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	ikashop::GetManager().RecvMyAuctionOpenRequestClientPacket(ch);
	return 0;
}

int IkarusShopPacketCloseMyAuction(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	ikashop::GetManager().RecvMyAuctionCloseClientPacket(ch);
	return 0;
}

int IkarusShopPacketCancelMyAuction(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	ikashop::GetManager().RecvMyAuctionCancelClientPacket(ch);
	return 0;
}

int IkarusShopPacketCreateAuction(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGAuctionCreate>(data, left, extra);
	ikashop::GetManager().RecvAuctionCreateClientPacket(ch, pack.startprice, pack.pos);
	return extra;
}

int IkarusShopPacketAddOffer(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGAuctionAddOffer>(data, left, extra);
	ikashop::GetManager().RecvAuctionAddOfferClientPacket(ch, pack.ownerid, pack.price);
	return extra;
}

int IkarusShopPacketExitFromAuction(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGAuctionExitFrom>(data, left, extra);
	ikashop::GetManager().RecvAuctionExitFromAuction(ch, pack.ownerid);
	return extra;
}

#ifdef ENABLE_IKASHOP_ENTITIES
int IkarusShopPacketClickEntity(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGShopClickEntity>(data, left, extra);

	ikashop::GetManager().RecvShopClickEntityClientPacket(ch, pack.vid);
	return extra;
}

int IkarusShopPacketFindMyShop(LPCHARACTER ch)
{
	ikashop::GetManager().RecvFindMyShopClientPacket(ch);
	return 0;
}

#ifdef EXTEND_IKASHOP_PRO
int IkarusShopPacketMoveEntity(LPCHARACTER ch)
{
	ikashop::GetManager().RecvMoveShopEntityClientPacket(ch);
	return 0;
}
#endif
#endif

#ifdef EXTEND_IKASHOP_PRO
int IkarusShopPacketNotificationListRequest(LPCHARACTER ch)
{
	ikashop::GetManager().SendNotificationListClientPacket(ch);
	ch->SetLookingNotificationList(true);
	return 0;
}

int IkarusShopPacketNotificationListClose(LPCHARACTER ch)
{
	ch->SetLookingNotificationList(false);
	return 0;
}
#endif

int IkarusShopPacketSearchItem(LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGShopSearchItem>(data, left, extra);

	ikashop::GetManager().RecvShopSearchItemClientPacket(ch, pack.itemVnum, pack.socket0);
	return extra;
}

int IkarusShopPacketRequestEdit(BYTE subheader, LPCHARACTER ch, const char* data, int left)
{
	int extra = 0;
	const auto& pack = Decode<ikashop::TSubPacketCGShopRequestEdit>(data, left, extra);

	ikashop::GetManager().RecvShopRequestEditClientPacket(ch, pack.bIsEdit);
	return extra;
}

int IkarusShopPacket(const char* data , LPCHARACTER ch, int iBufferLeft)
{
	BYTE subheader = 255;

	if (ch)
	{
		if (g_bChannel > 90)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot do that on special channel."));
			return 0;
		}

		if (quest::CQuestManager::instance().GetEventFlag("block_offline_shop") > 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This feature is temporarily unavailable."));
			return 0;
		}
	}

	try
	{
		int extra = 0;
		const auto& pack = Decode<TPacketCGNewIkarusShop>(data, iBufferLeft, extra);
		subheader = pack.subheader;

		switch (pack.subheader)
		{

		case ikashop::SUBHEADER_CG_SHOP_REOPEN:
			return IkarusShopPacketReopenShop(ch, data, iBufferLeft);

		case ikashop::SUBHEADER_CG_SHOP_FORCE_CLOSE:
			return IkarusShopPacketForceCloseShop(ch, data, iBufferLeft);

		case ikashop::SUBHEADER_CG_SHOP_OPEN:
			return IkarusShopPacketOpenShop(ch, data, iBufferLeft);

		case ikashop::SUBHEADER_CG_SHOP_OPEN_OWNER:
			return IkarusShopPacketOpenShowOwner(ch, data, iBufferLeft);

		case ikashop::SUBHEADER_CG_SHOP_BUY_ITEM:
			return IkarusShopPacketBuyItem(ch, data, iBufferLeft);

		case ikashop::SUBHEADER_CG_SHOP_ADD_ITEM:
			return IkarusShopPacketAddItem(ch, data, iBufferLeft);

		case ikashop::SUBHEADER_CG_SHOP_REMOVE_ITEM:
			return IkarusShopPacketRemoveItem(ch, data, iBufferLeft);

		case ikashop::SUBHEADER_CG_SHOP_REMOVE_ALL_ITEM:
			return IkarusShopPacketRemoveAllItem(ch, data, iBufferLeft);

		case ikashop::SUBHEADER_CG_SHOP_EDIT_ITEM:
			return IkarusShopPacketEditItem(ch, data, iBufferLeft);

		case ikashop::SUBHEADER_CG_SHOP_CHANGE_NAME:
			return IkarusShopPacketChangeName(ch, data, iBufferLeft);

		//case ikashop::SUBHEADER_CG_SHOP_FILTER_REQUEST:
		//	return IkarusShopPacketFilterRequest(ch, data, iBufferLeft);

		//case ikashop::SUBHEADER_CG_SHOP_SEARCH_FILL_REQUEST:
		//	return IkarusShopPacketSearchFillRequest(ch, data, iBufferLeft);

		//case ikashop::SUBHEADER_CG_SHOP_OFFER_CREATE:
		//	return IkarusShopPacketCreateOffer(ch, data, iBufferLeft);

		//case ikashop::SUBHEADER_CG_SHOP_OFFER_ACCEPT:
		//	return IkarusShopPacketAcceptOffer(ch, data, iBufferLeft);

		//case ikashop::SUBHEADER_CG_SHOP_REQUEST_OFFER_LIST:
		//	return IkarusShopPacketOfferListRequest(ch);

		//case ikashop::SUBHEADER_CG_SHOP_OFFER_CANCEL:
		//	return IkarusShopPacketOfferCancel(ch, data, iBufferLeft);

		case ikashop::SUBHEADER_CG_SHOP_SAFEBOX_OPEN:
			return IkarusShopPacketOpenSafebox(ch, data, iBufferLeft);

		//case ikashop::SUBHEADER_CG_SHOP_SAFEBOX_GET_ITEM:
		//	return IkarusShopPacketGetItemSafebox(ch, data, iBufferLeft);

		case ikashop::SUBHEADER_CG_SHOP_SAFEBOX_GET_VALUTES:
			return IkarusShopPacketGetValutesSafebox(ch, data, iBufferLeft);

		case ikashop::SUBHEADER_CG_SHOP_SAFEBOX_CLOSE:
			return IkarusShopPacketCloseSafebox(ch, data, iBufferLeft);

		//case ikashop::SUBHEADER_CG_AUCTION_LIST_REQUEST:
		//	return IkarusShopPacketListRequest(ch, data, iBufferLeft);

		//case ikashop::SUBHEADER_CG_AUCTION_OPEN_REQUEST:
		//	return IkarusShopPacketOpenAuctionRequest(ch, data, iBufferLeft);

		//case ikashop::SUBHEADER_CG_MY_AUCTION_OPEN_REQUEST:
		//	return IkarusShopPacketOpenMyAuctionRequest(ch, data, iBufferLeft);

		//case ikashop::SUBHEADER_CG_MY_AUCTION_CLOSE:
		//	return IkarusShopPacketCloseMyAuction(ch, data, iBufferLeft);

		//case ikashop::SUBHEADER_CG_MY_AUCTION_CANCEL:
		//	return IkarusShopPacketCancelMyAuction(ch, data, iBufferLeft);

		//case ikashop::SUBHEADER_CG_CREATE_AUCTION:
		//	return IkarusShopPacketCreateAuction(ch, data, iBufferLeft);

		//case ikashop::SUBHEADER_CG_AUCTION_ADD_OFFER:
		//	return IkarusShopPacketAddOffer(ch, data, iBufferLeft);

		//case ikashop::SUBHEADER_CG_EXIT_FROM_AUCTION:
		//	return IkarusShopPacketExitFromAuction(ch, data, iBufferLeft);

		case ikashop::SUBHEADER_CG_CLOSE_MY_SHOP_BOARD:
			return IkarusShopPacketCloseMyShopBoard(ch);

		//case ikashop::SUBHEADER_CG_CLOSE_OFFER_LIST:
		//	return IkarusShopPacketCloseOfferList(ch);

		case ikashop::SUBHEADER_CG_CLOSE_SHOP_GUEST:
			return IkarusShopPacketCloseShopGuest(ch);

		case ikashop::SUBHEADER_CG_FIND_MY_SHOP:
			return IkarusShopPacketFindMyShop(ch);

#ifdef ENABLE_IKASHOP_ENTITIES
		case ikashop::SUBHEADER_CG_CLICK_ENTITY:
			return IkarusShopPacketClickEntity(ch, data, iBufferLeft);
//#ifdef EXTEND_IKASHOP_PRO
//		case ikashop::SUBHEADER_CG_MOVE_SHOP_ENTITY:
//			return IkarusShopPacketMoveEntity(ch);
//#endif
#endif
#ifdef EXTEND_IKASHOP_PRO
		case ikashop::SUBHEADER_CG_NOTIFICATION_LIST_REQUEST:
			return IkarusShopPacketNotificationListRequest(ch);
		case ikashop::SUBHEADER_CG_NOTIFICATION_LIST_CLOSE:
			return IkarusShopPacketNotificationListClose(ch);
#endif

		case ikashop::SUBHEADER_CG_SHOP_SEARCH:
			return IkarusShopPacketSearchItem(ch, data, iBufferLeft);

		case ikashop::SUBHEADER_CG_REQUEST_EDIT:
			return IkarusShopPacketRequestEdit(subheader, ch, data, iBufferLeft);

		default:
			sys_err("UNKNOWN SUBHEADER %d ", subheader);
			return -1;
		}
	}

	catch(const DecodeException& e)
	{
		sys_err("FAILED TO DECODE SUBHEADER %u", subheader);
		return -1;
	}
}
#endif

int CInputMain::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		sys_err("no character on desc");
		d->SetPhase(PHASE_CLOSE);
		return (0);
	}

	int iExtraLen = 0;

	if (test_server && bHeader != HEADER_CG_MOVE)
		sys_log(0, "CInputMain::Analyze() ==> Header [%d] ", bHeader);

	// copy item aeldra
	if (!ch->GetDesc()->IsPhase(PHASE_GAME) && !ch->GetDesc()->IsPhase(PHASE_DEAD))
	{
		sys_err("no character in game");
		sys_log(0, "no character in game %u %u %u", ch->GetPlayerID(), ch->GetDesc()->GetPhase(), bHeader);
		d->SetPhase(PHASE_CLOSE);
		return (0);
	}

	if (ch && ch->IsPC())
	{
		if (get_global_time() < ch->analyze_protect)
		{
			ch->analyze_protect_count = ch->analyze_protect_count + 1;
			//ch->ChatDebug("<test server> analyze_protect_count. Count %d", ch->analyze_protect_count);

			if (ch->analyze_protect_count >= 300)
			{
				char buf[32 + 1];
				sprintf(buf, "FLOOD_HEADER_%d", bHeader);
				LogManager::instance().HackLog(buf, ch);

				ch->analyze_protect_count = 0;
				d->SetPhase(PHASE_CLOSE);
				return (0);
			}
		}
		else
			ch->analyze_protect_count = 0;

		ch->analyze_protect = get_global_time() + 1;
	}

	switch (bHeader)
	{
		case HEADER_CG_PONG:
			Pong(d);
			break;

		case HEADER_CG_TIME_SYNC:
			Handshake(d, c_pData);
			break;

		case HEADER_CG_CHAT:
			if (test_server)
			{
				char* pBuf = (char*)c_pData;
				sys_log(0, "%s", pBuf + sizeof(TPacketCGChat));
			}

			if ((iExtraLen = Chat(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_CG_WHISPER:
			if ((iExtraLen = Whisper(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_CG_MOVE:
			Move(ch, c_pData);
			// @fixme103 (removed CheckClientVersion since useless in here)
			break;

		case HEADER_CG_CHARACTER_POSITION:
			Position(ch, c_pData);
			break;

		case HEADER_CG_ITEM_USE:
			if (!ch->IsObserverMode())
				ItemUse(ch, c_pData);
			break;

		case HEADER_CG_ITEM_DROP:
			if (!ch->IsObserverMode())
			{
				ItemDrop(ch, c_pData);
			}
			break;

		case HEADER_CG_ITEM_DROP2:
			if (!ch->IsObserverMode())
				ItemDrop2(ch, c_pData);
			break;

		case HEADER_CG_ITEM_MOVE:
			if (!ch->IsObserverMode())
				ItemMove(ch, c_pData);
			break;

		case HEADER_CG_ITEM_PICKUP:
			if (!ch->IsObserverMode())
				ItemPickup(ch, c_pData);
			break;

		case HEADER_CG_ITEM_USE_TO_ITEM:
			if (!ch->IsObserverMode())
				ItemToItem(ch, c_pData);
			break;

		case HEADER_CG_ITEM_GIVE:
			if (!ch->IsObserverMode())
				ItemGive(ch, c_pData);
			break;

		case HEADER_CG_EXCHANGE:
			if (!ch->IsObserverMode())
				Exchange(ch, c_pData);
			break;

		case HEADER_CG_ATTACK:
		case HEADER_CG_SHOOT:
			if (!ch->IsObserverMode())
			{
				Attack(ch, bHeader, c_pData);
			}
			break;

		case HEADER_CG_USE_SKILL:
			if (!ch->IsObserverMode())
				UseSkill(ch, c_pData);
			break;

		case HEADER_CG_QUICKSLOT_ADD:
			QuickslotAdd(ch, c_pData);
			break;

		case HEADER_CG_QUICKSLOT_DEL:
			QuickslotDelete(ch, c_pData);
			break;

		case HEADER_CG_QUICKSLOT_SWAP:
			QuickslotSwap(ch, c_pData);
			break;

		case HEADER_CG_SHOP:
			if ((iExtraLen = Shop(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_CG_MESSENGER:
		{
			if ((iExtraLen = Messenger(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
		}
			break;

		case HEADER_CG_ON_CLICK:
			OnClick(ch, c_pData);
			break;

		case HEADER_CG_SYNC_POSITION:
			if ((iExtraLen = SyncPosition(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_CG_FLY_TARGETING:
			FlyTarget(ch, c_pData, bHeader);
			break;

		case HEADER_CG_SCRIPT_BUTTON:
			ScriptButton(ch, c_pData);
			break;

			// SCRIPT_SELECT_ITEM
		case HEADER_CG_SCRIPT_SELECT_ITEM:
			ScriptSelectItem(ch, c_pData);
			break;
			// END_OF_SCRIPT_SELECT_ITEM

		case HEADER_CG_SCRIPT_ANSWER:
			ScriptAnswer(ch, c_pData);
			break;

		case HEADER_CG_QUEST_INPUT_STRING:
			QuestInputString(ch, c_pData);
			break;

		case HEADER_CG_QUEST_CONFIRM:
			QuestConfirm(ch, c_pData);
			break;

		case HEADER_CG_TARGET:
			Target(ch, c_pData);
			break;

		case HEADER_CG_WARP:
			Warp(ch, c_pData);
			break;

		case HEADER_CG_SAFEBOX_CHECKIN:
			SafeboxCheckin(ch, c_pData);
			break;

		case HEADER_CG_SAFEBOX_CHECKOUT:
			SafeboxCheckout(ch, c_pData, false);
			break;

		case HEADER_CG_SAFEBOX_ITEM_MOVE:
			SafeboxItemMove(ch, c_pData);
			break;

		case HEADER_CG_MALL_CHECKOUT:
			SafeboxCheckout(ch, c_pData, true);
			break;

		case HEADER_CG_PARTY_INVITE:
			PartyInvite(ch, c_pData);
			break;

		case HEADER_CG_PARTY_REMOVE:
			PartyRemove(ch, c_pData);
			break;

		case HEADER_CG_PARTY_INVITE_ANSWER:
			PartyInviteAnswer(ch, c_pData);
			break;

		case HEADER_CG_PARTY_SET_STATE:
			PartySetState(ch, c_pData);
			break;

		case HEADER_CG_PARTY_USE_SKILL:
			PartyUseSkill(ch, c_pData);
			break;

		case HEADER_CG_PARTY_PARAMETER:
			PartyParameter(ch, c_pData);
			break;

		case HEADER_CG_ANSWER_MAKE_GUILD:
#ifdef ENABLE_NEWGUILDMAKE
			ch->ChatPacket(CHAT_TYPE_INFO, "<%s> AnswerMakeGuild disabled", __FUNCTION__);
#else
			AnswerMakeGuild(ch, c_pData);
#endif
			break;

		case HEADER_CG_GUILD:
			if ((iExtraLen = Guild(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_CG_FISHING:
			Fishing(ch, c_pData);
			break;

		case HEADER_CG_HACK:
			Hack(ch, c_pData);
			break;

		case HEADER_CG_MYSHOP:
			if ((iExtraLen = MyShop(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		case HEADER_CG_ACCE:
			Acce(ch, c_pData);
			break;
#endif

		case HEADER_CG_REFINE:
			Refine(ch, c_pData);
			break;

		case HEADER_CG_DRAGON_SOUL_REFINE:
			{
				TPacketCGDragonSoulRefine* p = reinterpret_cast <TPacketCGDragonSoulRefine*>((void*)c_pData);
				switch(p->bSubType)
				{
				case DS_SUB_HEADER_CLOSE:
					ch->DragonSoul_RefineWindow_Close();
					break;
				case DS_SUB_HEADER_DO_REFINE_GRADE:
					{
						DSManager::instance().DoRefineGrade(ch, p->ItemGrid);
					}
					break;
				case DS_SUB_HEADER_DO_REFINE_STEP:
					{
						DSManager::instance().DoRefineStep(ch, p->ItemGrid);
					}
					break;
				case DS_SUB_HEADER_DO_REFINE_STRENGTH:
					{
						DSManager::instance().DoRefineStrength(ch, p->ItemGrid);
					}
					break;
				}
			}

			break;
#ifdef ENABLE_IKASHOP_RENEWAL
		case HEADER_CG_NEW_OFFLINESHOP:
			if((iExtraLen = IkarusShopPacket(c_pData, ch, m_iBufferLeft))< 0)
				return -1;
			break;
#endif


		case HEADER_CG_REQUEST_INFO_RESPONSE:
			RequestInfoResponse(ch, c_pData);
			break;
	}
	return (iExtraLen);
}

int CInputDead::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		sys_err("no character on desc");
		d->SetPhase(PHASE_CLOSE);
		return 0;
	}

	if (d && !PulseManager::Instance().IncreaseCount(inet_addr(d->GetHostName()), ePulse::AnalyzeMain, std::chrono::milliseconds(1000), 10))
	{
		d->SetPhase(PHASE_CLOSE);
		return 0;
	}

	int iExtraLen = 0;

	switch (bHeader)
	{
		case HEADER_CG_PONG:
			Pong(d);
			break;

		case HEADER_CG_TIME_SYNC:
			Handshake(d, c_pData);
			break;

		case HEADER_CG_CHAT:
			if ((iExtraLen = Chat(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;

			break;

		case HEADER_CG_WHISPER:
			if ((iExtraLen = Whisper(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;

			break;

		case HEADER_CG_HACK:
			Hack(ch, c_pData);
			break;

		default:
			return (0);
	}

	return (iExtraLen);
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top

