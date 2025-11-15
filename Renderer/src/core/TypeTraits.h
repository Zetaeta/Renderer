#pragma once
#include <type_traits>

template<typename T>
struct is_trivially_movable
{
	constexpr static bool value = std::is_trivially_copyable_v<T>;
};

template<typename T>
constexpr bool is_trivially_movable_v = is_trivially_movable<T>::value;

#define DECLARE_TRIVIALLY_MOVABLE(Type)\
	template<>\
	struct is_trivially_movable<Type> {\
		constexpr static bool value = true;\
	};


template<typename T>
struct TClassTypeTraitsBase
{
	enum
	{
		HasSerialize = false
	};
};

template<typename T>
struct TClassTypeTraits : TClassTypeTraitsBase<T> {};
