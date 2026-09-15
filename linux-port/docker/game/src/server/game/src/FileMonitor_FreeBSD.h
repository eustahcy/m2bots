#ifndef FILEMONITOR_FREEBSD_INCLUDED
#define FILEMONITOR_FREEBSD_INCLUDED

#include "IFileMonitor.h"
#if !defined(__linux__)
#include <unistd.h>
#include <sys/event.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/time.h>

struct FileIOContext_FreeBSD
{
	int		fhMonitor;
	int		idxToEventList;	// evtTrigger & evtMonitor index should be same
	PFN_FileChangeListener pListenFunc;
};

class FileMonitorFreeBSD : public IFileMonitor
{
private:
	FileMonitorFreeBSD(); //hidden

public:
	virtual ~FileMonitorFreeBSD();

	void AddWatch	(const std::string& strFileName, PFN_FileChangeListener pListenerFunc);
	void Update		(DWORD dwPulses);

	static FileMonitorFreeBSD& Instance()
	{
		static FileMonitorFreeBSD theMonitor;
		return theMonitor;
	}

private:
	typedef boost::unordered_map<std::string, FileIOContext_FreeBSD> TMonitorFileHashMap;
	typedef std::vector<struct kevent>	TEventList;

	TMonitorFileHashMap	m_FileLists;
	TEventList			m_MonitoredEventLists;
	TEventList			m_TriggeredEventLists;

	int					m_KernelEventQueue;
};

#endif // !__linux__
#endif //FILEMONITOR_FREEBSD_INCLUDED
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
