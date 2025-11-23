#pragma once

#include <string_view>
#include "Types.h"

using StringView = std::string_view;

inline bool CharEqualsIgnoreCase(char a, char b)
{
	return std::tolower(static_cast<u8>(a)) == std::tolower(static_cast<u8>(b));
}

template<typename RangeA, typename RangeB>
bool EqualsIgnoreCase(const RangeA& a, const RangeB& b)
	requires(!std::is_pointer_v<RangeA> && !std::is_pointer_v<RangeB>)
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

template<typename StrType = String>
inline bool EqualsIgnoreCase(const StrType& a, char const* b)
{
	return EqualsIgnoreCase(a.c_str(), b);
}

template<typename StrType = String>
inline bool EqualsIgnoreCase(char const* a, StrType const& b)
{
	return EqualsIgnoreCase(a, b.c_str());
}

Vector<StringView> Split(StringView sv, char delim);
