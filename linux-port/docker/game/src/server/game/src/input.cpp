#include "stdafx.h"
#include <sstream>

#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "buffer_manager.h"
#include "config.h"
#include "profiler.h"
#include "p2p.h"
#include "log.h"
#include "db.h"
#include "questmanager.h"
#include "login_sim.h"
#include "fishing.h"
#include "priv_manager.h"
#include "castle.h"
#ifndef __WIN32__
	#include "limit_time.h"
#endif
#include "../../common/CommonDefines.h"

#include "utils.h"
#include "../../common/PulseManager.h"

bool IsEmptyAdminPage()
{
	return g_stAdminPageIP.empty();
}

bool IsAdminPage(const char * ip)
{
	for (size_t n = 0; n < g_stAdminPageIP.size(); ++n)
	{
		if (g_stAdminPageIP[n] == ip)
			return 1;
	}
	return 0;
}

void ClearAdminPages()
{
	for (size_t n = 0; n < g_stAdminPageIP.size(); ++n)
		g_stAdminPageIP[n].clear();

	g_stAdminPageIP.clear();
}

CInputProcessor::CInputProcessor() : m_pPacketInfo(NULL), m_iBufferLeft(0)
{
	if (!m_pPacketInfo)
		BindPacketInfo(&m_packetInfoCG);
}

void CInputProcessor::BindPacketInfo(CPacketInfo * pPacketInfo)
{
	m_pPacketInfo = pPacketInfo;
}

bool CInputProcessor::Process(LPDESC lpDesc, const void * c_pvOrig, int iBytes, int & r_iBytesProceed)
{
	const char * c_pData = (const char *) c_pvOrig;

	BYTE	bLastHeader = 0;
	int		iLastPacketLen = 0;
	int		iPacketLen;

	if (!m_pPacketInfo)
	{
		sys_err("No packet info has been binded to");
		return true;
	}

	for (m_iBufferLeft = iBytes; m_iBufferLeft > 0;)
	{
		BYTE bHeader = (BYTE) *(c_pData);
		const char * c_pszName;

		if (bHeader == 0)
			iPacketLen = 1;
		else if (!m_pPacketInfo->Get(bHeader, &iPacketLen, &c_pszName))
		{
			sys_err("UNKNOWN HEADER: %d, LAST HEADER: %d(%d), REMAIN BYTES: %d, fd: %d",
					bHeader, bLastHeader, iLastPacketLen, m_iBufferLeft, lpDesc->GetSocket());
			//printdata((BYTE *) c_pvOrig, m_iBufferLeft);
			lpDesc->SetPhase(PHASE_CLOSE);
			return true;
		}

		if (m_iBufferLeft < iPacketLen)
			return true;

		if (bHeader)
		{
			if (test_server && bHeader != HEADER_CG_MOVE)
				sys_log(0, "Packet Analyze [Header %d][bufferLeft %d] ", bHeader, m_iBufferLeft);

			m_pPacketInfo->Start();

			int iExtraPacketSize = Analyze(lpDesc, bHeader, c_pData);

			if (iExtraPacketSize < 0)
				return true;

			iPacketLen += iExtraPacketSize;
			lpDesc->Log("%s %d", c_pszName, iPacketLen);
			m_pPacketInfo->End();
		}

#ifdef ENABLE_SEQUENCE_SYSTEM
		if (bHeader == HEADER_CG_PONG)
			sys_log(0, "PONG! %u %u", m_pPacketInfo->IsSequence(bHeader), *(BYTE *) (c_pData + iPacketLen - sizeof(BYTE)));

		if (m_pPacketInfo->IsSequence(bHeader))
		{
			BYTE bSeq = lpDesc->GetSequence();
			BYTE bSeqReceived = *(BYTE *) (c_pData + iPacketLen - sizeof(BYTE));

			if (bSeq != bSeqReceived)
			{
				sys_err("SEQUENCE %x mismatch 0x%x != 0x%x header %u", get_pointer(lpDesc), bSeq, bSeqReceived, bHeader);

				LPCHARACTER	ch = lpDesc->GetCharacter();

				char buf[1024];
				int	offset, len;

				offset = snprintf(buf, sizeof(buf), "SEQUENCE_LOG [%s]-------------\n", ch ? ch->GetName() : "UNKNOWN");

				if (offset < 0 || offset >= (int) sizeof(buf))
					offset = sizeof(buf) - 1;

				for (size_t i = 0; i < lpDesc->m_seq_vector.size(); ++i)
				{
					len = snprintf(buf + offset, sizeof(buf) - offset, "\t[%03d : 0x%x]\n",
							lpDesc->m_seq_vector[i].hdr,
							lpDesc->m_seq_vector[i].seq);

					if (len < 0 || len >= (int) sizeof(buf) - offset)
						offset += (sizeof(buf) - offset) - 1;
					else
						offset += len;
				}

				snprintf(buf + offset, sizeof(buf) - offset, "\t[%03d : 0x%x]\n", bHeader, bSeq);
				sys_err("%s", buf);

				lpDesc->SetPhase(PHASE_CLOSE);
				return true;
			}
			else
			{
				lpDesc->push_seq(bHeader, bSeq);
				lpDesc->SetNextSequence();
				//sys_err("SEQUENCE %x match %u next %u header %u", lpDesc, bSeq, lpDesc->GetSequence(), bHeader);
			}
		}
#endif

		c_pData	+= iPacketLen;
		m_iBufferLeft -= iPacketLen;
		r_iBytesProceed += iPacketLen;

		iLastPacketLen = iPacketLen;
		bLastHeader	= bHeader;

		if (GetType() != lpDesc->GetInputProcessor()->GetType())
			return false;
	}

	return true;
}

void CInputProcessor::Pong(LPDESC d)
{
	d->SetPong(true);
}

void CInputProcessor::Handshake(LPDESC d, const char * c_pData)
{
	TPacketCGHandshake * p = (TPacketCGHandshake *) c_pData;

	if (d->GetHandshake() != p->dwHandshake)
	{
		sys_err("Invalid Handshake on %d", d->GetSocket());
		d->SetPhase(PHASE_CLOSE);
	}
	else
	{
		if (d->IsPhase(PHASE_HANDSHAKE))
		{
			if (d->HandshakeProcess(p->dwTime, p->lDelta, false))
			{
#ifdef _IMPROVED_PACKET_ENCRYPTION_
				d->SendKeyAgreement();
#else
				if (g_bAuthServer)
					d->SetPhase(PHASE_AUTH);
				else
					d->SetPhase(PHASE_LOGIN);
#endif // #ifdef _IMPROVED_PACKET_ENCRYPTION_
			}
		}
		else
			d->HandshakeProcess(p->dwTime, p->lDelta, true);
	}
}

void LoginFailure(LPDESC d, const char * c_pszStatus)
{
	if (!d)
		return;

	TPacketGCLoginFailure failurePacket;

	failurePacket.header = HEADER_GC_LOGIN_FAILURE;
	strlcpy(failurePacket.szStatus, c_pszStatus, sizeof(failurePacket.szStatus));

	d->Packet(&failurePacket, sizeof(failurePacket));
}

CInputHandshake::CInputHandshake()
{
	CPacketInfoCG * pkPacketInfo = M2_NEW CPacketInfoCG;
#ifdef ENABLE_SEQUENCE_SYSTEM
	pkPacketInfo->SetSequence(HEADER_CG_PONG, false);
#endif
	m_pMainPacketInfo = m_pPacketInfo;
	BindPacketInfo(pkPacketInfo);
}

CInputHandshake::~CInputHandshake()
{
	if( NULL != m_pPacketInfo )
	{
		M2_DELETE(m_pPacketInfo);
		m_pPacketInfo = NULL;
	}
}

std::map<DWORD, CLoginSim *> g_sim;
std::map<DWORD, CLoginSim *> g_simByPID;
std::vector<TPlayerTable> g_vec_save;

// BLOCK_CHAT
ACMD(do_block_chat);
// END_OF_BLOCK_CHAT

int CInputHandshake::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	//if (c_pData && (strncmp(c_pData, "GET ", 4) == 0 || strncmp(c_pData, "POST ", 5) == 0 ||
	//	strncmp(c_pData, "HEAD ", 5) == 0 || strncmp(c_pData, "PUT ", 4) == 0 ||
	//	strncmp(c_pData, "DELETE ", 7) == 0 || strncmp(c_pData, "OPTIONS ", 8) == 0 ||
	//	strncmp(c_pData, "TRACE ", 6) == 0 || strncmp(c_pData, "CONNECT ", 8) == 0 ||
	//	strncmp(c_pData, "PATCH ", 6) == 0 || strncmp(c_pData, "PROPFIND ", 9) == 0))
	//{
	//	sys_log(0, "Wykryto zapychanie Portu przez %s Naprawiamy.", d->GetHostName());
	//	char szCommand[128];
	//	sprintf(szCommand, "pfctl -t abusive_hosts -T add %s", d->GetHostName());
	//	std::system(szCommand);
	//	return 0;
	//}

	if (bHeader == 10)
		return 0;

	if (bHeader == HEADER_CG_MARK_LOGIN)
	{
		if (!guild_mark_server)
		{
			sys_err("Guild Mark login requested but i'm not a mark server!");
			d->SetPhase(PHASE_CLOSE);
			return 0;
		}

		sys_log(0, "MARK_SERVER: Login");
		d->SetPhase(PHASE_LOGIN);
		return 0;
	}
	else if (bHeader == HEADER_CG_STATE_CHECKER)
	{
		if (d->isChannelStatusRequested()) {
			return 0;
		}

#ifdef ENABLE_CHANNEL_STATUS_CACHE
		if (!PulseManager::Instance().IncreaseClock(0, ePulse::ChannelStatus, std::chrono::milliseconds(1000)))
			{
			if (!cachedChannelStatus.empty()) // first time check
			{
				d->Packet(cachedChannelStatus.data(), cachedChannelStatus.size());
				return 0;
			}
		}
#endif

		d->SetChannelStatusRequested(true);
		db_clientdesc->DBPacket(HEADER_GD_REQUEST_CHANNELSTATUS, d->GetHandle(), NULL, 0);
	}
	else if (bHeader == HEADER_CG_PONG)
		Pong(d);
	else if (bHeader == HEADER_CG_HANDSHAKE)
		Handshake(d, c_pData);
#ifdef _IMPROVED_PACKET_ENCRYPTION_
	else if (bHeader == HEADER_CG_KEY_AGREEMENT)
	{
		// Send out the key agreement completion packet first
		// to help client to enter encryption mode
		d->SendKeyAgreementCompleted();
		// Flush socket output before going encrypted
		d->ProcessOutput();

		TPacketKeyAgreement* p = (TPacketKeyAgreement*)c_pData;
		if (!d->IsCipherPrepared())
		{
			sys_err ("Cipher isn't prepared. %s maybe a Hacker.", inet_ntoa(d->GetAddr().sin_addr));
			d->DelayedDisconnect(5);
			return 0;
		}
		if (d->FinishHandshake(p->wAgreedLength, p->data, p->wDataLength)) {
			// Handshaking succeeded
			if (g_bAuthServer) {
				d->SetPhase(PHASE_AUTH);
			} else {
				d->SetPhase(PHASE_LOGIN);
			}
		} else {
			sys_log(0, "[CInputHandshake] Key agreement failed: al=%u dl=%u",
				p->wAgreedLength, p->wDataLength);
			d->SetPhase(PHASE_CLOSE);
		}
	}
#endif // _IMPROVED_PACKET_ENCRYPTION_
	else
		sys_err("Handshake phase does not handle packet %d (fd %d) from %s:%u", bHeader, d->GetSocket(), d->GetHostName(), d->GetPort()); //@warme016 host and port

	return 0;
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
