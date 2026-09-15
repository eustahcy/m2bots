#include "stdafx.h"
#include "config.h"
#include "EventFunctionHandler.hpp"

CEventFunctionHandler::CEventFunctionHandler() : bProcessStatus(true)
{
}

CEventFunctionHandler::~CEventFunctionHandler()
{
	Destroy();
}

void CEventFunctionHandler::Destroy()
{
	m_event.clear();
	bProcessStatus = false;
}

bool CEventFunctionHandler::AddEvent(std::function<void(SArgumentSupportImpl*)> func, const std::string& event_name, const size_t& runtime, SArgumentSupportImpl* support_arg)
{
	// if (m_event.size() >= EVENT_MAX_NUM)
		// return false;

	if (GetHandlerByName(event_name))
		return false;

	m_event.insert(std::make_pair(event_name, std::unique_ptr<SFunctionHandler>(new SFunctionHandler(func, runtime, support_arg))));
	return true;
}

void CEventFunctionHandler::RemoveEvent(const std::string& event_name)
{
	auto ptr = GetHandlerByName(event_name);
	if (ptr)
		m_event.erase(event_name);
}

void CEventFunctionHandler::DelayEvent(const std::string& event_name, const size_t& newtime)
{
	auto ptr = GetHandlerByName(event_name);
	if (ptr)
		ptr->UpdateTime(newtime);
}

bool CEventFunctionHandler::FindEvent(const std::string& event_name)
{
	auto ptr = GetHandlerByName(event_name);
	return (ptr != nullptr);
}

DWORD CEventFunctionHandler::GetDelay(const std::string& event_name)
{
	auto ptr = GetHandlerByName(event_name);
	if (ptr)
		return (ptr->time >= static_cast<DWORD>(get_global_time())) ? (ptr->time - get_global_time()) : 0;
	else
		return 0;
}

void CEventFunctionHandler::Process()
{
	if (!bProcessStatus || !m_event.size())
		return;

	std::vector<std::string> v_delete;
	for (const auto& event : m_event)
	{
		if (static_cast<DWORD>(get_global_time()) >= (event.second).get()->time)
		{
			// Data Safety - if event below triggers RemoveEvent, that might crash the channel
			v_delete.push_back(event.first);
			(event.second).get()->func((event.second).get()->SupportArg.get());
		}
	}

	if (!v_delete.size())
		return;

	for (const auto& key : v_delete)
		m_event.erase(key);
}

CEventFunctionHandler::SFunctionHandler* CEventFunctionHandler::GetHandlerByName(const std::string& event_name)
{
	if (m_event.find(event_name) == m_event.end())
		return nullptr;

	return (m_event.find(event_name)->second).get();
}

// Files shared by GameCore.top
