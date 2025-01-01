#pragma once
#include "core/BaseDefines.h"
#include <cstddef>
#include <array>
#include <type_traits>

using byte = unsigned char;

template<size_t Size, size_t Alignment = Size>
struct CopyableMemory
{
	CopyableMemory() = default;

	template<typename T>
	struct FitsInMe_
	{
		constexpr static bool Value = sizeof(T) <= Size && std::is_trivially_copyable_v<T>;
	};

	template<typename T>
	constexpr static bool FitsInMe = FitsInMe_<T>::Value;
	

	template<typename T>
		requires(FitsInMe<T>)
	CopyableMemory(T const& value)
	{
		As<T>() = value;
	}

	template<typename T>
		requires(FitsInMe<T>)
	CopyableMemory& operator=(T const& value)
	{
		As<T>() = value;
		return *this;
	}
	
	CopyableMemory(CopyableMemory const& other) = default;
	CopyableMemory& operator=(CopyableMemory const& other) = default;

	alignas(Alignment)
	std::array<byte, Size> Bytes;

	template<typename T>
		requires(FitsInMe<T>)
	T& As()
	{
		return *reinterpret_cast<T*>(&Bytes[0]);
	}

	template<typename T>
		requires(FitsInMe<T>)
	T const& As() const
	{
		return *reinterpret_cast<T const*>(&Bytes[0]);
	}
};
