#pragma once

#include <unordered_set>
#include <string>

#include <cryptopp/cryptlib.h>
#include <cryptopp/filters.h>
#include <cryptopp/modes.h>
#include <cryptopp/aes.h>

class CheckHardware : public singleton<CheckHardware>
{
public:
	CheckHardware();
	virtual ~CheckHardware();
	void Destroy();
	std::string DecodeHWID(const std::string& hdid);
	void Block(const std::string& id);
	void Unblock(std::string id);

protected:
	CryptoPP::ECB_Mode< CryptoPP::AES >::Decryption spAES;
};


// Files shared by GameCore.top
