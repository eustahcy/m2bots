// vim: ts=8 sw=4
#ifndef __INC_MONEY_LOG
#define __INC_MONEY_LOG

#include <map>

class CMoneyLog : public singleton<CMoneyLog>
{
    public:
	CMoneyLog();
	virtual ~CMoneyLog();

	void Save();
	void AddLog(BYTE bType, DWORD dwVnum, YANG iGold);

    private:
	std::map<DWORD, YANG> m_MoneyLogContainer[MONEY_LOG_TYPE_MAX_NUM];
};

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
