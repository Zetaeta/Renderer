#pragma once
#include "core/CoreTypes.h"
#include "core/Types.h"
#include "core/StringView.h"

class CommandLine
{
public:
	class Iterator
	{
	public:
		Iterator(const CommandLine& cmdLine = CommandLine::Get());
		StringView GetKey() const;
		StringView GetValue();
		bool HasValue() const
		{
			return mValEnd != mValBegin;
		}

		Iterator& operator++();

		operator bool() const
		{
			return mArgEnd != mArgStart;
		}
	private:
		const CommandLine& mCmdLine;
		u32 mArgIdx = 0;
		u32 mArgStart = 0;
		u32 mArgEnd = 0;
		u32 mValBegin = 0;
		u32 mValEnd = 0;
		String mParsedValue;
	};

	static CommandLine const& Get();

	operator const String&() const
	{
		return mValue;
	}

	Vector<String> GetValues(StringView key) const;

	const Vector<String>& Argv() const { return argv; }

private:
	String mValue;
	Vector<String> argv;
	CommandLine();
};
