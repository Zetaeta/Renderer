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
