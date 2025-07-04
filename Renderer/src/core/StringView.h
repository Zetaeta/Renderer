#pragma once

#include <string_view>
#include "Types.h"

using StringView = std::string_view;

inline bool CharEqualsIgnoreCase(char a, char b)
{
	return std::tolower(static_cast<u8>(a)) == std::tolower(static_cast<u8>(b));
}

template<typename TRange = String>
bool EqualsIgnoreCase(const TRange& a, const TRange& b)
{
	return a.size() == b.size() && std::equal(std::begin(a), std::end(a), std::begin(b), CharEqualsIgnoreCase);
}

inline bool EqualsIgnoreCase(char const* a, char const* b)
{
	int i = 0;
	for (;a[i] && b[i]; ++i)
	{
		if (!CharEqualsIgnoreCase(a[i], b[i]))
		{
			return false;
		}
	}
	return a[i] == b[i];
}

inline bool EqualsIgnoreCase(const String& a, char const* b)
{
	return EqualsIgnoreCase(a.c_str(), b);
}

inline bool EqualsIgnoreCase(char const* a, String const& b)
{
	return EqualsIgnoreCase(a, b.c_str());
}

Vector<StringView> Split(StringView sv, char delim);
