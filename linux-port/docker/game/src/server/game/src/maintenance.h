#pragma once
class CMaintenance : public singleton<CMaintenance>
{
	public:

		CMaintenance();
		virtual ~CMaintenance();

		void StartMaintenance(DWORD time_until, DWORD duration, bool start_event);
		void StopMaintenance(bool send=false);
		DWORD GetTimeUntilMaintenance();
		DWORD GetMaintenanceDuration();
		void SendMaintenanceInfo();
		void SendMaintenanceInfo(LPCHARACTER ch);
		bool IsMaintenanceScheduled();

	private:
		LPEVENT m_pkMaintenanceEvent;

		DWORD m_dwExecuteTime;
		DWORD m_dwTimeUntil;
		DWORD m_dwDuration;
};


// Files shared by GameCore.top
