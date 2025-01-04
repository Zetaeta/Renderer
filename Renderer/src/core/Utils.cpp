#include "core/Utils.h"
#include <sstream>
#include <algorithm>
#include <cstdarg>
#include "Logging.h"
#include "platform/Platform.h"

unsigned char const ZerosArray[1024] = {0};

DEFINE_LOG_CATEGORY_STATIC(LogCore);

void AssertionFailed(bool fatal, const char* file, u32 line, const char* fmt, ...)
{
	String platformError = Platform::GetPlatformError();

	RLOG(LogCore, Error, "Assertion failed at %s:%d", file, line);
	va_list va;
	va_start(va, fmt);
	RLogVa(LogCore, ELogVerbosity::Error, fmt, va);
	va_end(va);
	if (!platformError.empty())
	{
		RLOG(LogCore, Error, "Platform error: %s", platformError.c_str());
	}
	FlushLog();
}

std::vector<std::string> Split(const std::string& s, char delim)
{
	std::vector<std::string> result;
	std::stringstream		 ss(s);
	std::string				 item;

	while (getline(ss, item, delim))
	{
		result.push_back(item);
	}

	return result;
}

bool FindIgnoreCase(const std::string_view& haystack, const std::string_view& needle)
{
	auto it = std::search(
		haystack.begin(), haystack.end(),
		needle.begin(), needle.end(),
		[](unsigned char ch1, unsigned char ch2) { return std::toupper(ch1) == std::toupper(ch2); });
	return (it != haystack.end());
}
