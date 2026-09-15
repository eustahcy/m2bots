#include <memory>
#include "stdafx.h"
#include "../../common/tables.h"
#include "db.h"
#include "Hardware.h"
#include <cryptopp/cryptlib.h>
#include <cryptopp/aes.h>
#include <cryptopp/hex.h>

// The key we use for that enc/dec
namespace
{
	const std::vector<unsigned char> vAESDecKey{ 'V', 'B', 'l', 'k', '2' ,'y', 'Q', 'R', 't', 'i', 'H', 'A', '1', 'o', 'j', '7' };
}

CheckHardware::CheckHardware()
{
	spAES.SetKey(vAESDecKey.data(), vAESDecKey.size());
}

CheckHardware::~CheckHardware()
{
	Destroy();
}

void CheckHardware::Destroy()
{
}

std::string CheckHardware::DecodeHWID(const std::string& hdid)
{
	using namespace CryptoPP;
	std::string sDecryptedString;

	// Decrypting string
	try
	{
		// Dehexing string
		std::string sTemp;
		StringSource ss(hdid, true,
			new HexDecoder(
				new StringSink(sTemp)
			) // HexEncoder
		); // StringSource

		// Decrypting string
		StringSource ss1(sTemp, true,
			new StreamTransformationFilter(spAES,
				new StringSink(sDecryptedString)
			) // StreamTransformationFilter      
		); // StringSource
	}
	catch (const CryptoPP::Exception& e)
	{
		sys_err("CryptoPP exception while decoding hwid: %s", e.what());
		return "";
	}

	// Check if string contains at least real size in its block
	if (!sDecryptedString.size())
	{
		sys_err("Decoded hwid is empty!");
		return "";
	}

	return sDecryptedString;
}

void CheckHardware::Block(const std::string& id)
{
	if (!id.size())
		return;

	DBManager::instance().AccountQuery("UPDATE hardware_status SET status='BLOCK' WHERE hwid='%s'", id.c_str());
}

void CheckHardware::Unblock(std::string id)
{
	DBManager::instance().DirectQuery("UPDATE hardware_status SET status='OK' WHERE hwid='%s'", id.c_str());
}

// Files shared by GameCore.top
