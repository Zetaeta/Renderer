#pragma once
#include "core/BaseDefines.h"
#include <cstddef>
#include <array>
#include <type_traits>

using byte = unsigned char;

template<size_t Size, size_t Alignment = Size>
struct CopyableMemory
{
	alignas(Alignment)
	std::array<byte, Size> Data;

	CopyableMemory() = default;
	CopyableMemory(CopyableMemory const& other) = default;
	CopyableMemory& operator=(CopyableMemory const& other) = default;

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
	
	template<typename T>
		requires(FitsInMe<T>)
	T& As() &
	{
		return *std::launder(reinterpret_cast<T*>(&Data[0]));
	}

	template<typename T>
		requires(FitsInMe<T>)
	T const& As() const &
	{
		return *std::launder(reinterpret_cast<T const*>(&Data[0]));
	}

	template<typename T>
		requires(FitsInMe<T>)
	T As() &&
	{
		return *std::launder(reinterpret_cast<T const*>(&Data[0]));
	}
};

template<size_t Size, size_t Alignment = Size>
using OpaqueData = CopyableMemory<Size, Alignment>;
