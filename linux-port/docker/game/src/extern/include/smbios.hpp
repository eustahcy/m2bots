#pragma once
#include <iostream>
#include <windows.h>
#include <conio.h>
#include <tchar.h>
#include <intrin.h>
#include <d3d9.h>
#include <iomanip>
#include <vector>
#include <map>

#include <boost/format.hpp>

#pragma warning(disable : 4200)

#define out_of_spec "OUT OF SPEC"

namespace smbios 
{
  enum dmi_entry_type
    : BYTE
  {
    DMI_ENTRY_BIOS = 0,
    DMI_ENTRY_SYSTEM,
    DMI_ENTRY_BASEBOARD,
    DMI_ENTRY_CHASSIS,
    DMI_ENTRY_PROCESSOR,
    DMI_ENTRY_MEM_CONTROLLER,
    DMI_ENTRY_MEM_MODULE,
    DMI_ENTRY_CACHE,
    DMI_ENTRY_PORT_CONNECTOR,
    DMI_ENTRY_SYSTEM_SLOT,
    DMI_ENTRY_ONBOARD_DEVICE,
    DMI_ENTRY_OEMSTRINGS,
    DMI_ENTRY_SYSCONF,
    DMI_ENTRY_BIOS_LANG,
    DMI_ENTRY_GROUP_ASSOC,
    DMI_ENTRY_SYSTEM_EVENT_LOG,
    DMI_ENTRY_PHYS_MEM_ARRAY,
    DMI_ENTRY_MEM_DEVICE,
    DMI_ENTRY_32_MEM_ERROR,
    DMI_ENTRY_MEM_ARRAY_MAPPED_ADDR,
    DMI_ENTRY_MEM_DEV_MAPPED_ADDR,
    DMI_ENTRY_BUILTIN_POINTING_DEV,
    DMI_ENTRY_PORTABLE_BATTERY,
    DMI_ENTRY_SYSTEM_RESET,
    DMI_ENTRY_HW_SECURITY,
    DMI_ENTRY_SYSTEM_POWER_CONTROLS,
    DMI_ENTRY_VOLTAGE_PROBE,
    DMI_ENTRY_COOLING_DEV,
    DMI_ENTRY_TEMP_PROBE,
    DMI_ENTRY_ELECTRICAL_CURRENT_PROBE,
    DMI_ENTRY_OOB_REMOTE_ACCESS,
    DMI_ENTRY_BIS_ENTRY,
    DMI_ENTRY_SYSTEM_BOOT,
    DMI_ENTRY_MGMT_DEV,
    DMI_ENTRY_MGMT_DEV_COMPONENT,
    DMI_ENTRY_MGMT_DEV_THRES,
    DMI_ENTRY_MEM_CHANNEL,
    DMI_ENTRY_IPMI_DEV,
    DMI_ENTRY_SYS_POWER_SUPPLY,
    DMI_ENTRY_ADDITIONAL,
    DMI_ENTRY_ONBOARD_DEV_EXT,
    DMI_ENTRY_MGMT_CONTROLLER_HOST,
    DMI_ENTRY_INACTIVE = 126,
    DMI_ENTRY_END_OF_TABLE = 127,
  };

	enum dmi_memory_device_type // 7.18.2
		: BYTE
	{
		DMT_OTHER = 0x01, /* 0x01 */
		DMT_UNKNOWN,
		DMT_DRAM,
		DMT_EDRAM,
		DMT_VRAM,
		DMT_SRAM,
		DMT_RAM,
		DMT_ROM,
		DMT_FLASH,
		DMT_EEPROM,
		DMT_FEPROM,
		DMT_EPROM,
		DMT_CDRAM,
		DMT_3DRAM,
		DMT_SDRAM,
		DMT_SGRAM,
		DMT_RDRAM,
		DMT_DDR,
		DMT_DDR2,
		DMT_DDR2_FB_DIMM,
		DMT_RESERVED1,
		DMT_RESERVED2,
		DMT_RESERVED3,
		DMT_DDR3,
		DMT_FBD2,
		DMT_DDR4,
		DMT_LPDDR,
		DMT_LPDDR2,
		DMT_LPDDR3,
		DMT_LPDDR4,
		DMT_LOGICAL_NONVOLATILE_DEVICE,
		DMT_HBM,
		DMT_HBM2 /* 0x21 */
	};

	//
	// Data structure the consults the hold of extracted data from the the raw
	// SMBIOS firmware table
	//

	typedef struct RawSMBIOSData
	{
		BYTE    Used20CallingMethod;
		BYTE    SMBIOSMajorVersion;
		BYTE    SMBIOSMinorVersion;
		BYTE    DmiRevision;
		DWORD   Length;
		BYTE    SMBIOSTableData[];
	} RawSMBIOSData;



	// definition:
	// Desktop Management Interface (DMI) generates a standard framework 
	// for managing and tracking components in a desktop, notebook or server 
	// computer, by abstracting these components from the software that manages them.
	//

	typedef struct _dmi_header
	{
		BYTE		type;
		BYTE		length;
		WORD		handle;
	} dmi_header;


	// function protoypes
	std::string dmi_system_uuid(const BYTE* p, short version)
	{
		bool only0xff = true;
		bool only0x00 = true;
		int i{};

		//
		// 16 because of the byte uuid
		// this process also makes so that the uuid isn't all
		// 1's and 0's
		//


		for (i = 0; i < 16 && (only0x00 || only0xff); i++) {
			if (p[i] != 0x00) { only0x00 = false; }
			if (p[i] != 0xFF) { only0xff = false; }
		}

		char buf[128] = {};
		if (only0x00) { return buf; }
		if (only0xff) { return buf; }

		if (version >= 0x0206) {
			sprintf_s(buf, sizeof(buf), "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
				p[3], p[2], p[1], p[0], p[5], p[4], p[7], p[6],
				p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
		}
		else {
			sprintf_s(buf, sizeof(buf), "-%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
				p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
				p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
		}

		return buf;
	}

	const char* dmi_string(const dmi_header* dm, BYTE src)
	{
		std::size_t length{};
		std::size_t i{};

		char* bp = (char*)dm;
			//reinterpret_cast<char*> (dm);

		if (src == 0) { return "Not specified"; }
		bp += dm->length;

		while (src > 1 && *bp) {
			bp += std::strlen(bp);
			bp++;
			src--;
		}
		
		if (!*bp) { return "BAD_INDEXING"; }

		
		
		//
		// filtering ASCII
		// removal of junk data
		//

		length = std::strlen(bp);
		for (int i = 0; i < length; i++) {
			if (bp[i] < 32 || bp[i] == 127) { bp[i] = '.'; }
		}
		return bp;

	}
	const char* dmi_memory_device_type(BYTE code)
	{
		std::map<int, const char*> memory_device_type;
		/* 7.18.2 */

		memory_device_type[dmi_memory_device_type::DMT_OTHER]				 = "Other"; /* 0x01 */
		memory_device_type[dmi_memory_device_type::DMT_UNKNOWN]			 = "Unknown";
		memory_device_type[dmi_memory_device_type::DMT_DRAM]				 = "DRAM";
		memory_device_type[dmi_memory_device_type::DMT_EDRAM]				 = "EDRAM";
		memory_device_type[dmi_memory_device_type::DMT_VRAM]				 = "VRAM";
		memory_device_type[dmi_memory_device_type::DMT_SRAM]				 = "SRAM";
		memory_device_type[dmi_memory_device_type::DMT_RAM]					 = "RAM";
		memory_device_type[dmi_memory_device_type::DMT_ROM]					 = "ROM";
		memory_device_type[dmi_memory_device_type::DMT_FLASH]				 = "Flash";
		memory_device_type[dmi_memory_device_type::DMT_EEPROM]			 = "EEPROM";
		memory_device_type[dmi_memory_device_type::DMT_FEPROM]			 = "FEPROM";
		memory_device_type[dmi_memory_device_type::DMT_EPROM]				 = "EPROM";
		memory_device_type[dmi_memory_device_type::DMT_CDRAM]				 = "CDRAM";
		memory_device_type[dmi_memory_device_type::DMT_3DRAM]				 = "3DRAM";
		memory_device_type[dmi_memory_device_type::DMT_SDRAM]				 = "SDRAM";
		memory_device_type[dmi_memory_device_type::DMT_SGRAM]			   = "SGRAM";
		memory_device_type[dmi_memory_device_type::DMT_RDRAM]				 = "RDRAM";
		memory_device_type[dmi_memory_device_type::DMT_DDR]					 = "DDR";
		memory_device_type[dmi_memory_device_type::DMT_DDR2]				 = "DDR2";
		memory_device_type[dmi_memory_device_type::DMT_DDR2_FB_DIMM] = "DDR2 FB-DIMM";
		memory_device_type[dmi_memory_device_type::DMT_RESERVED1]		 = "Reserved";
		memory_device_type[dmi_memory_device_type::DMT_RESERVED2]		 = "Reserved";
		memory_device_type[dmi_memory_device_type::DMT_RESERVED3]		 = "Reserved";
		memory_device_type[dmi_memory_device_type::DMT_DDR3]				 = "DDR3";
		memory_device_type[dmi_memory_device_type::DMT_FBD2]				 = "FBD2";
		memory_device_type[dmi_memory_device_type::DMT_DDR4]				 = "DDR4";
		memory_device_type[dmi_memory_device_type::DMT_LPDDR]				 = "LPDDR";
		memory_device_type[dmi_memory_device_type::DMT_LPDDR2]			 = "LPDDR2";
		memory_device_type[dmi_memory_device_type::DMT_LPDDR3]			 = "LPDDR3";
		memory_device_type[dmi_memory_device_type::DMT_LPDDR4]			 = "LPDDR4";
		memory_device_type[dmi_memory_device_type::DMT_LOGICAL_NONVOLATILE_DEVICE] = "Logical non-volatile device";
		memory_device_type[dmi_memory_device_type::DMT_HBM]					 = "HBM";
		memory_device_type[dmi_memory_device_type::DMT_HBM2]				 = "HBM2"; /* 0x21 */


		if (code >= smbios::dmi_memory_device_type::DMT_OTHER
		&& code <= smbios::dmi_memory_device_type::DMT_HBM2) {
			return memory_device_type[code - 0x01];
		}
			
		return out_of_spec;
	}
};