#include "CommandLine.h"
#include "platform/Platform.h"
#include "core/Maths.h"

static Vector<String> ParseCommandLine(const String& cmdLine)
{
	Vector<String> result;
	String arg;
	u32 i = 0;
	u32	size = NumCast<u32>(cmdLine.size());
	bool quoted = false;
	bool escaped = false;
	bool argStarted = false;
	while (i < size)
	{
		char c = cmdLine[i];
		bool escapeNext = false;
		if (escaped)
		{
			arg.push_back(c);
		}
		else
		{
			if (!quoted && isspace(c))
			{
				if (argStarted)
				{
					result.push_back(std::move(arg));
					arg.clear();
					argStarted = false;
				}
			}
			else
			{
				argStarted = true;
				if (c == '\\')
				{
					escapeNext = true;
				}
				else if (c == '"')
				{
					quoted = !quoted;
				}
				else
				{
					arg.push_back(c);
				}
			}
		}

		escaped = escapeNext;
		++i;
	}

	if (argStarted && !arg.empty())
	{
		result.push_back(std::move(arg));
	}
	return result;
}

CommandLine const& CommandLine::Get()
{
	static CommandLine singleton;
	return singleton;
}

Vector<String> CommandLine::GetValues(StringView key) const
{
	Vector<String> result;
	for (Iterator it(*this); it; ++it)
	{
		if (EqualsIgnoreCase(it.GetKey(), key))
		{
			result.emplace_back(it.GetValue());
		}
	}
	return result;
}

CommandLine::CommandLine()
{
	mValue = Platform::GetCmdLine();
	argv = ParseCommandLine(mValue);
}

CommandLine::Iterator::Iterator(const CommandLine& cmdLine)
	: mCmdLine(cmdLine)
{
	operator++();
}

StringView CommandLine::Iterator::GetKey() const
{
	return StringView(mCmdLine.Argv()[mArgIdx].data() + mArgStart, mArgEnd - mArgStart);
}

StringView CommandLine::Iterator::GetValue()
{
	return StringView(mCmdLine.Argv()[mArgIdx].data() + mValBegin, mValEnd - mValBegin);
}

CommandLine::Iterator& CommandLine::Iterator::operator++()
{
	++mArgIdx;
	if (mArgIdx >= mCmdLine.Argv().size())
	{
		mArgStart = mArgEnd = mValBegin = mValEnd = 0;
		return *this;
	}
	
	u32 i = 0;
	const auto& str = mCmdLine.Argv()[mArgIdx];
	const u32 size = NumCast<u32>(str.size());
	while (i < size && str[i] != '-') ++i;
	if (i == size)
	{
		mValBegin = mValEnd = mArgEnd = mArgStart = i;
	}
	else
	{
		mArgStart = mArgEnd = mValEnd = i + 1;
		while (mArgEnd < size && isalnum(str[mArgEnd]))
		{
			mArgEnd++;
		}
		if (mArgEnd == mArgStart) // "-" followed by nothing
		{
			mValBegin = mValEnd = mArgEnd;
		}
		else if (mArgEnd == size || str[mArgEnd] != '=')
		{
			mValBegin = mValEnd = mArgEnd;
		}
		else
		{
			mValBegin = mArgEnd + 1;
			mValEnd = NumCast<u32>(str.size());
		}
	}
	return *this;
}
