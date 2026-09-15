#include "stdafx.h"
#include "maintenance.h"
#include "special_shop_manager.h"
#include "char.h"
#include "char_manager.h"
#include "utils.h"
#include "config.h"
#include "packet.h"
#include "p2p.h"

CMaintenance::CMaintenance()
{
	m_dwTimeUntil = 0;
	m_dwDuration = 0;
	m_dwExecuteTime = 0;
	m_pkMaintenanceEvent = NULL;
}

CMaintenance::~CMaintenance()
{
}

EVENTINFO(maintenance_event_info)
{
	maintenance_event_info()
	{
	}
};

EVENTFUNC(maintenance_event)
{
	maintenance_event_info* info = dynamic_cast<maintenance_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("maintenance_event> <Factor> Null pointer");
		return 0;
	}

	TPacketGGShutdown p;
	p.bHeader = HEADER_GG_SHUTDOWN;
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShutdown));

	Shutdown(10);
	return 0;
}

void CMaintenance::StartMaintenance(DWORD time_until, DWORD duration, bool start_event)
{
	if (IsMaintenanceScheduled())
		return;

	m_dwTimeUntil = time_until;
	m_dwDuration = duration;
	m_dwExecuteTime = get_global_time();
	SendMaintenanceInfo();

	if (start_event)
	{
		TPacketGGDelayedShutdown p{};
		p.bHeader = HEADER_GG_DELAYED_SHUTDOWN;
		p.dwTimeUntil = time_until;
		p.dwDuration = duration;
		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGDelayedShutdown));

		m_pkMaintenanceEvent = event_create(maintenance_event, AllocEventInfo<maintenance_event_info>(), PASSES_PER_SEC(time_until - 10));
	}
}

void CMaintenance::StopMaintenance(bool send)
{
	m_dwTimeUntil = 0;
	m_dwDuration = 0;
	m_dwExecuteTime = 0;

	if (m_pkMaintenanceEvent)
		event_cancel(&m_pkMaintenanceEvent);

	SendMaintenanceInfo();

	if (send)
	{
		TPacketGGDelayedShutdown p{};
		p.bHeader = HEADER_GG_DELAYED_SHUTDOWN;
		p.dwTimeUntil = 0;
		p.dwDuration = 0;
		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGDelayedShutdown));
	}
}

DWORD CMaintenance::GetTimeUntilMaintenance()
{
	const auto startTime = m_dwExecuteTime + m_dwTimeUntil;
	if (startTime < get_global_time())
		return 0;

	return startTime - get_global_time();
}

DWORD CMaintenance::GetMaintenanceDuration()
{
	return m_dwDuration;
}

void CMaintenance::SendMaintenanceInfo()
{
	auto pc_map = CHARACTER_MANAGER::instance().GetPCMap();
	for (auto it : pc_map)
	{
		auto ch = it.second;
		if (ch)
			SendMaintenanceInfo(ch);
	}
}

void CMaintenance::SendMaintenanceInfo(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (!ch->GetDesc())
		return;

	ch->ChatPacket(CHAT_TYPE_COMMAND, "maintenance %d %d",
		GetTimeUntilMaintenance(),
		GetMaintenanceDuration());
}

bool CMaintenance::IsMaintenanceScheduled()
{
	return m_pkMaintenanceEvent != NULL || GetTimeUntilMaintenance() > 0;
}

// Files shared by GameCore.top
