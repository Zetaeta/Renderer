#include "Name.h"
#include "mutex"

template<typename IntType>
struct IdentityHash
{
	size_t operator()(const IntType& intVal) const
	{
		return static_cast<size_t>(intVal);
	}
};

struct NameHashList
{
	SmallVector<String, 1> Strings;
};

static std::mutex gNameTableMtx;
// No point hashing the hashes
static std::unordered_map<u32, NameHashList, IdentityHash<u32>> gNameTable;

Name::Name()
: Name("")
{
}

Name::Name(char const* str)
{
	mHash = crc32(str);
	FinishConstructing(str);
}

Name::Name(std::string_view str)
{
	mHash = crc32(str);
	FinishConstructing(String(str));
}

Name::Name(String&& str)
{
	mHash = crc32(str);
	FinishConstructing(std::move(str));
}

void Name::FinishConstructing(String&& string)
{
	std::scoped_lock lock(gNameTableMtx);
	NameHashList& entry = gNameTable[mHash];
	for (u32 i = 0; i < entry.Strings.size(); ++i)
	{
		if (entry.Strings[i] == string)
		{
			mIndex = i;
			break;
		}
	}
	entry.Strings.push_back(std::move(string));
	mIndex = static_cast<u32>(entry.Strings.size() - 1);
}

String Name::ToString() const
{
	std::scoped_lock lock(gNameTableMtx);
	NameHashList& entry = gNameTable[mHash];
	return entry.Strings[mIndex];
}

char const* Name::c_str() const
{
	std::scoped_lock lock(gNameTableMtx);
	NameHashList& entry = gNameTable[mHash];
	return entry.Strings[mIndex].c_str();
	// TODO : not thread safe with SSO due to SmallVector
}

DEFINE_CLASS_TYPEINFO_EXT(Name)
END_CLASS_TYPEINFO()

