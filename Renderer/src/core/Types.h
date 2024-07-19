#pragma once

#include <vector>
#include <string>
#include <memory>

template<typename T>
using Vector = std::vector<T>;

using String = std::string;

using Name = String;

template<typename T>
using OwningPtr = std::unique_ptr<T>;

template<typename T, typename... Args>
OwningPtr<T> MakeOwning(Args&&... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

