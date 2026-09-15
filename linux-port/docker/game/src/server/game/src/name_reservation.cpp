#include "stdafx.h"
#include "constants.h"
#include "name_reservation.h"

CNameReservationManager::CNameReservationManager()
{
}

bool CNameReservationManager::Initialize(TNameReservationTable* p, WORD wSize)
{
	m_map_NameReservations.clear();
	for (WORD i = 0; i < wSize; ++i, ++p)
	{
		char name[CHARACTER_NAME_MAX_LEN + 1];
		lower_string(p->name, name, sizeof(name));
		sys_log(0, "jest rezerwacja %s dla %d", name, p->dwOwner);
		m_map_NameReservations[name] = p->dwOwner;
	}

	char szBuf[256];
	snprintf(szBuf, sizeof(szBuf), "NameReservation loaded! (total %zu name reservations)", m_map_NameReservations.size());
	sys_log(0, szBuf);
	return true;
}

DWORD CNameReservationManager::FindOwner(const char* c_pszString)
{
	auto it = m_map_NameReservations.find(c_pszString);
	if (it != m_map_NameReservations.end()) {
		return it->second;
	}
	else {
		return 0;
	}
	return 0;
}

BYTE CNameReservationManager::CheckReservation(DWORD account_id, const char* c_pszString)
{
	char name[CHARACTER_NAME_MAX_LEN + 1];
	lower_string(c_pszString, name, sizeof(name));

	DWORD owner = FindOwner(name);
	sys_log(0, "found owner %d==%d of name %s", account_id, owner, name);
	if (owner == 0)
		return 0; // the name is not reserved

	if (owner != account_id)
		return 1; // not owner of reservation

	return 2; // good to go
}

// Files shared by GameCore.top
