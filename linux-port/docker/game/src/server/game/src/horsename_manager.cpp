#include "stdafx.h"
#include "horsename_manager.h"
#include "desc_client.h"
#include "char_manager.h"
#include "char.h"
#include "affect.h"
#include "utils.h"

CHorseNameManager::CHorseNameManager()
{
	m_mapHorseNames.clear();
}

const char* CHorseNameManager::GetHorseName(DWORD dwPlayerID)
{
	std::map<DWORD, std::string>::iterator iter;

	iter = m_mapHorseNames.find(dwPlayerID);

	if ( iter != m_mapHorseNames.end() )
	{
		return iter->second.c_str();
	}
	else
	{
		return NULL;
	}
}

void CHorseNameManager::UpdateHorseName(DWORD dwPlayerID, const char* szHorseName, bool broadcast)
{
	if ( szHorseName == NULL )
	{
		sys_err("HORSE_NAME: NULL NAME (%u)", dwPlayerID);
		szHorseName = "";
	}

	sys_log(0, "HORSENAME: update %u %s", dwPlayerID, szHorseName);

	m_mapHorseNames[dwPlayerID] = szHorseName;

	if ( broadcast == true )
	{
		BroadcastHorseName(dwPlayerID, szHorseName);
	}
}

void CHorseNameManager::BroadcastHorseName(DWORD dwPlayerID, const char* szHorseName)
{
	TPacketUpdateHorseName packet;
	packet.dwPlayerID = dwPlayerID;
	strlcpy(packet.szHorseName, szHorseName, sizeof(packet.szHorseName));

	db_clientdesc->DBPacket(HEADER_GD_UPDATE_HORSE_NAME, 0, &packet, sizeof(TPacketUpdateHorseName));
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
