#pragma once

#include <string>
#include "core/CoreTypes.h"
#include "core/ClassTypeInfoFwd.h"
#include "core/Hash.h"
#include "TypeTraits.h"

using String = std::string;

class HashString
{
	DECLARE_STI_EXT_NOBASE(HashString);
public:

	HashString(char const* str);
	HashString(std::string_view str);
	HashString(String const& str)
	: HashString(str.c_str())
	{
	}
	HashString(String&& str);

	HashString();

	String ToString() const;

	std::strong_ordering operator<=>(const HashString& other) const
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

	bool operator==(HashString const& other) const
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

	// For debugging : gets the string without locking
	const char* GetDataUnsafe() const;
};


template<>
struct std::hash<HashString>
{
	size_t operator()(const HashString& name) const
	{
		return CombineHash(name.GetHash(), name.GetIndex());
	}
};

template<>
struct TClassTypeTraits<HashString> : public TClassTypeTraitsBase<HashString>
{
	enum
	{
		HasSerialize = true
	};
};

//DECLARE_CLASS_TYPEINFO()
