#pragma once

#include <string>
#include "core/CoreTypes.h"
#include "core/ClassTypeInfoFwd.h"
#include "TypeTraits.h"

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

	void Serialize(class Serializer& serializer);

	u32 GetHash() const { return mHash; }
	u32 GetIndex() const { return mIndex; }
	private:
	u32 mHash = 0;
	u32 mIndex = 0;
private:
	template<typename T>
	void FinishConstructing(T&& str);
};

template<>
struct TClassTypeTraits<Name> : public TClassTypeTraitsBase<Name>
{
	enum
	{
		HasSerialize = true
	};
};

//DECLARE_CLASS_TYPEINFO()
