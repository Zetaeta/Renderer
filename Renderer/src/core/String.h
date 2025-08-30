#pragma once

#include <string>
#include "Types.h"

enum class ECharEncoding
{
	Ansi,
	Utf8
};

// RAII wrapper for char* buffers, because std::(w)string has SSO so less safe/convenient to use as a container
template<typename CharType>
class THeapString
{
public:
	THeapString() = default;

	THeapString(size_t size)
	:mString(MakeOwning<CharType[]>(size)) {}

	THeapString(THeapString&& other) noexcept
		: mString(std::move(other.mString))
	{
	}

	THeapString& operator=(THeapString&& other) noexcept
	{
		mString = std::move(other.mString);
	}

	CharType const* Get() const
	{
		return mString.get();
	}

	CharType* GetBuffer()
	{
		return mString.get();
	}

	CharType& operator[](size_t index)
	{
		return mString.get()[index];
	}

private:
	OwningPtr<CharType[]> mString;
};

using HeapString = THeapString<char>;
using HeapWString = THeapString<wchar_t>;

template<typename T>
struct StringFactory { };

template<typename CharType>
struct StringFactory<THeapString<CharType>>
{
	static THeapString<CharType> MakeSized(size_t size)
	{
		return THeapString<CharType>(size);
	}
};

template<typename CharType>
struct StringFactory<std::basic_string<CharType>>
{
	static std::basic_string<CharType> MakeSized(size_t size)
	{
		return std::basic_string<CharType>(size, 0);
	}
};

std::wstring ToWideString(char const* str, ECharEncoding encoding = ECharEncoding::Utf8);
std::string ToNarrowString(wchar_t const* string, ECharEncoding encoding = ECharEncoding::Utf8);

HeapWString ToWideHeapString(char const* str, ECharEncoding encoding = ECharEncoding::Utf8);
HeapString ToNarrowHeapString(wchar_t const* string, ECharEncoding encoding = ECharEncoding::Utf8);

inline std::wstring ToWideString(std::string const& str, ECharEncoding encoding = ECharEncoding::Utf8)
{
	return ToWideString(str.c_str(), encoding);
}
inline std::string ToNarrowString(std::wstring const& str, ECharEncoding encoding = ECharEncoding::Utf8)
{
	return ToNarrowString(str.c_str(), encoding);
}

inline HeapWString ToWideHeapString(std::string const& str, ECharEncoding encoding = ECharEncoding::Utf8)
{
	return ToWideHeapString(str.c_str(), encoding);
}

inline HeapString ToNarrowHeapString(std::wstring const& str, ECharEncoding encoding = ECharEncoding::Utf8)
{
	return ToNarrowHeapString(str.c_str(), encoding);
}
