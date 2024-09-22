#pragma once
#include "Types.h"
#include "Utils.h"

enum class ELogVerbosity : u8
{
	Critical,
	Error,
	Warning,
	Info,
	Verbose
};

class LogCategory
{
	RCOPY_MOVE_PROTECT(LogCategory);
	String mName;
public:
	LogCategory(const char* name)
		: mName(name) {}
};

constexpr size_t LogBufferSize = 1024;
namespace LogPrivate
{
	extern thread_local char sBuffer[LogBufferSize];

	void LogImpl(LogCategory const& category, ELogVerbosity verbosity, String&& message);
}

extern LogCategory const LogGlobal; 

template<typename... Args>
void RLog(LogCategory const& category, ELogVerbosity verbosity, char const* fmt, Args&&... args)
{
	if constexpr (sizeof...(args) > 0)
	{
		int len = snprintf(LogPrivate::sBuffer, LogBufferSize, fmt, std::forward<Args>(args)...);
		LogImpl(category, verbosity, String(LogPrivate::sBuffer, len));
	}
	else
	{
		LogPrivate::LogImpl(category, verbosity, fmt);
	}
}

#define RLOG(...) RLog(__VA_ARGS__)
