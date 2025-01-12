#pragma once

#include <array>
#include "core/Utils.h"

// Array indexed by enums
template<typename Enum, typename T, Enum Size = Enum::Count>
class EnumArray : public std::array<T, Denum(Size)>
{
public:
	using std::array<T, Denum(Size)>::operator[];
	T& operator[](Enum index)
	{
		return operator[](Denum(index));
	}

	T const& operator[](Enum index) const
	{
		return operator[](Denum(index));
	}
};
