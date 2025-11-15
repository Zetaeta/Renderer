#pragma once

#include <string>
#include "core/CoreTypes.h"
#include "core/ClassTypeInfoFwd.h"

using String = std::string;

class Name
{
	DECLARE_STI_EXT_NOBASE(Name);
public:

	Name(char const* str);
	Name(std::string_view str);
	Name(String const& str)
	: Name(str.c_str())
	{
	}
	Name(String&& str);

	Name();

	String ToString() const;

	std::strong_ordering operator<=>(const Name& other) const
	{
		if (mHash < other.mHash)
		{
			return std::strong_ordering::less;
		}
		else if (mHash > other.mHash)
		{
			return std::strong_ordering::greater;
		}
		else
		{
			return mIndex <=> other.mIndex;
		}
	}

	bool operator==(Name const& other) const
	{
		return mHash == other.mHash && mIndex == other.mIndex;
	}

	char const* c_str() const;

	bool empty() const
	{
		return mHash == 0;
	}

	u32 GetHash() const { return mHash; }
	u32 GetIndex() const { return mIndex; }
	private:
	u32 mHash;
	u32 mIndex;
private:
	void FinishConstructing(String&& str);
};

//DECLARE_CLASS_TYPEINFO()
