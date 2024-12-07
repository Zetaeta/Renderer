#pragma once
#include <algorithm>
#include <vector>

template<typename T>
using Vector = std::vector<T>;

template<typename T>
bool Remove(Vector<T>& vec, T const& value)
{
	if (auto it = std::find(vec.begin(), vec.end(), value); it != vec.end())
	{
		vec.erase(it);
		return true;
	}
	return false;
}
