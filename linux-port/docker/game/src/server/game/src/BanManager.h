#pragma once
class BanManager : public singleton<BanManager>
{
	enum {
		BLOCK_INFINITE_DURATION = 5 * 365 * 3600 * 24,
	};

public:
	BanManager();
	virtual ~BanManager();

	struct BlockPlayerData
	{
		DWORD aid;
		DWORD pid;
		char name[24 + 1];
		bool foundOnline;
	};

	struct BlockHwidData
	{
		char hwid[255 + 1];
	};

	BlockPlayerData GetBlockPlayerData(DWORD pid);
	BlockHwidData GetBlockHwidData(DWORD pid);

	bool CloseOfflineShop(DWORD pid);

	bool Block(DWORD aid, DWORD pid, DWORD duration, DWORD who, std::string reason, bool instant_block);
	void BlockHWID(const char * hwid);
	void BlockAccountQuery(DWORD aid, DWORD duration);
	int FlushBlock();
	void BlockHwidQuery(const char* hwid);
	void SendDisconnect(DWORD pid);
	void SendDisconnectHwid(const char * hwid);
	void DisconnectHwid(const char * hwid);

};



// Files shared by GameCore.top
