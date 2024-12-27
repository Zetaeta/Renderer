#pragma once

#include "BaseDefines.h"
#include <vector>
#include <string>
#include <memory>
#include <span>

template<typename T>
using Vector = std::vector<T>;

using String = std::string;

template<typename T>
using Span = std::span<T>;

//class String : public std::string
//{
//public:
//	String() = default;
//	String(const std::string& str)
//		: std::string(str) {}
//
//	String(const char* str)
//		: std::string(str) {}
//
//	const char* operator*() const
//	{
//		return c_str();
//	}
//};

//template<>
//class std::iterator_traits<String> : public std::iterator_traits<std::string>
//{
//};

//template<>
//struct std::hash<String>
//{
//	size_t operator()(const String& str) const
//	{
//		return std::hash<std::string>()(str);
//	}
//};


using Name = String;

inline const char* GetNameData(Name const& name)
{
	return name.c_str();
}

template<typename T>
using OwningPtr = std::unique_ptr<T>;

template<typename T, typename... Args>
OwningPtr<T> MakeOwning(Args&&... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

