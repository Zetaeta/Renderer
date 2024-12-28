#pragma once
#include "core/BaseDefines.h"
#include <cstddef>
#include <array>

using byte = unsigned char;

template<size_t Size, size_t Alignment = 8>
struct CopyableMemory
{
	alignas(Alignment)
	std::array<byte, Size> Bytes;

	template<typename T>
	T& As()
	{
		static_assert(sizeof(T) <= Size, "Can't interpret CopyableMemory as larger type");
		static_assert(std::is_trivially_copyable_v<T>, "Trying to store non-trivially-copyable type in CopyableMemory");
		return *reinterpret_cast<T*>(&Bytes[0]);
	}

	template<typename T>
	T const& As() const
	{
		return const_cast<CopyableMemory*>(this)->As<T>();
	}
};
