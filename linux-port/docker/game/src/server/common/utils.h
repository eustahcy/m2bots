#ifndef _INCLUDE_HEADER_COMMON_UTILS_
#define _INCLUDE_HEADER_COMMON_UTILS_

#include <msl/utils.h>

/*----- atoi function -----*/
inline bool str_to_number (bool& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (strtol(in, NULL, 10) != 0);
	return true;
}

inline bool str_to_number (char& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (char) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (unsigned char& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (unsigned char) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (short& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (short) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (unsigned short& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (unsigned short) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (int& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (int) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (unsigned int& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (unsigned int) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (long& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (long) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (unsigned long& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (unsigned long) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (long long& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (long long) strtoull(in, NULL, 10);
	return true;
}

inline bool str_to_number (unsigned long long& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (unsigned long long) strtoull(in, NULL, 10);
	return true;
}

inline bool str_to_number (float& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (float) strtof(in, NULL);
	return true;
}

inline bool str_to_number (double& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (double) strtod(in, NULL);
	return true;
}

#ifdef __FreeBSD__
inline bool str_to_number (long double& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (long double) strtold(in, NULL);
	return true;
}
#endif

#include "CommonDefines.h"
#include <string>
#ifdef ENABLE_IKASHOP_RENEWAL
template<class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
inline bool str_to_enum(T& out, const char* in) {
	using S = std::underlying_type_t<T>;
	S temp{};
	if (!str_to_number(temp, in))
		return false;
	out = static_cast<T>(temp);
	return true;
}

template <size_t size>
inline bool str_to_cstring(char(&array)[size], const char* from) {
	auto len = strnlen(from, size - 1) + 1;
	strncpy(array, from, len);
	array[size - 1] = 0;
	return true;
}

inline bool str_to_string(std::string& dest, const char* from) {
	auto len = strnlen(from, 4096);
	dest = std::string{ from, len };
	return true;
}
#endif

#include <regex>
inline bool has_proper_characters(const std::string& input)
{
	std::regex pattern("^[\\x20-\\x7E¥¹ÆæÊê£³ÑñÓóŒœŸ¯¿]*$");
	return std::regex_match(input, pattern);
}
#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by Metin2hub.com
