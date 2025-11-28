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

HashString::HashString()
: HashString("")
{
}

HashString::HashString(char const* str)
{
	mHash = crc32(str);
	FinishConstructing(str);
}

HashString::HashString(std::string_view str)
{
	mHash = crc32(str);
	FinishConstructing(String(str));
}

HashString::HashString(String&& str)
{
	mHash = crc32(str);
	FinishConstructing(std::move(str));
}

template<typename T>
void HashString::FinishConstructing(T&& string)
{
	std::scoped_lock lock(gNameTableMtx);
	NameHashList& entry = gNameTable[mHash];
	for (u32 i = 0; i < entry.Strings.size(); ++i)
	{
		if (entry.Strings[i] == string)
		{
			mIndex = i;
			return;
		}
	}
	entry.Strings.emplace_back(std::forward<T>(string));
	mIndex = static_cast<u32>(entry.Strings.size() - 1);
}

String HashString::ToString() const
{
	std::scoped_lock lock(gNameTableMtx);
	NameHashList& entry = gNameTable[mHash];
	return entry.Strings[mIndex];
}

char const* HashString::c_str() const
{
	std::scoped_lock lock(gNameTableMtx);
	NameHashList& entry = gNameTable[mHash];
	return entry.Strings[mIndex].c_str();
	// TODO : not thread safe with SSO due to SmallVector
}


const char* HashString::GetDataUnsafe() const
{
	auto it = gNameTable.find(mHash);
	if (it == gNameTable.end())
	{
		return "invalid hash";
	}
	if (mIndex >= it->second.Strings.size())
	{
		return "invalid index";
	}
	return it->second.Strings[mIndex].c_str();
}

void HashString::Serialize(Serializer& serializer)
{
	if (serializer.IsLoading())
	{
		String str;
		serializer.SerializeString(str);
		*this = HashString(str);
	}
	else
	{
		String str = ToString();
		serializer.SerializeString(str);
	}
}


DEFINE_CLASS_TYPEINFO_EXT(HashString)
END_CLASS_TYPEINFO()

