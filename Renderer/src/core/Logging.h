#pragma once
#include "Types.h"
#include "Utils.h"
#include "WorkerThread.h"

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

	String const& GetName() const
	{
		return mName;
	}
};

constexpr size_t LogBufferSize = 4096;
namespace LogPrivate
{
	extern thread_local char sBuffer[LogBufferSize];

	void LogImpl(LogCategory const& category, ELogVerbosity verbosity, String&& message);
}


template<typename... Args>
void RLog(LogCategory const& category, ELogVerbosity verbosity, char const* fmt, Args&&... args)
{
	if constexpr (sizeof...(args) > 0)
	{
		int len = snprintf(LogPrivate::sBuffer, LogBufferSize, fmt, std::forward<Args>(args)...);
		LogPrivate::LogImpl(category, verbosity, String(LogPrivate::sBuffer, len));
	}
	else
	{
		LogPrivate::LogImpl(category, verbosity, fmt);
	}
}

inline void RLogVa(LogCategory const& category, ELogVerbosity verbosity, char const* fmt, va_list args)
{
	int len = vsnprintf(LogPrivate::sBuffer, LogBufferSize, fmt, args);
	LogPrivate::LogImpl(category, verbosity, String(LogPrivate::sBuffer, len));
}

#define DEFINE_LOG_CATEGORY_STATIC(...) static DEFINE_LOG_CATEGORY_IMPL(__VA_ARGS__)
#define DEFINE_LOG_CATEGORY(...) DEFINE_LOG_CATEGORY_IMPL(__VA_ARGS__)
#define DECLARE_LOG_CATEGORY(Name, ...) extern LogCategory const Name;
#define DEFINE_LOG_CATEGORY_IMPL(Name, ...) LogCategory const Name(#Name);

DECLARE_LOG_CATEGORY(LogGlobal);

#define RLOG(category, verb, ...) RLog(category, ELogVerbosity::verb, __VA_ARGS__)
//#define RLOGVA(category, verb, format, val) RLogVa(category, ELogVerbosity::verb, format, val)

class LogConsumerThread : public WorkerThread
{
public:
	void Flush() {}

protected:
	virtual void DoWork() override;
};
