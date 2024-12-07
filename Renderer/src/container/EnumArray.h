#pragma once

#include <array>
#include "core/Utils.h"

// Array indexed by enums
template<typename T, typename Enum, Enum Size = Enum::Count>
class EnumArray : public std::array<T, Denum(Enum::Count)>
{
public:
	using std::array<T, Denum(Enum::Count)>::operator[];
	T& operator[](Enum index)
	{
		return operator[](Denum(index));
	}

	T const& operator[](Enum index) const
	{
		return operator[](Denum(index));
	}
};
