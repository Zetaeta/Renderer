#pragma once

#include "core/CoreTypes.h"
#include "core/StringView.h"

template <class>
inline constexpr bool always_false_v = false;

enum class EConfigVarFlags : u8
{
	Default = 0
};

enum class EConfigVarType : u8
{
	String,
	Int,
	Float,
	Bool
};

template<typename T>
constexpr EConfigVarType GetCVType()
{
	static_assert(always_false_v<T>, "Invalid config var type");
}

template<>
constexpr EConfigVarType GetCVType<int>()
{
	return EConfigVarType::Int;
}

template<>
constexpr EConfigVarType GetCVType<float>()
{
	return EConfigVarType::Float;
}

template<>
constexpr EConfigVarType GetCVType<bool>()
{
	return EConfigVarType::Bool;
}

template<>
constexpr EConfigVarType GetCVType<String>()
{
	return EConfigVarType::String;
}


class IConfigVariable
{
public:
	virtual bool SetValueFromString(StringView stringValue) = 0;
	virtual String GetValueAsString() const = 0;

	virtual EConfigVarType GetType() const = 0;
};

