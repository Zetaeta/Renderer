#include "String.h"

#include <Windows.h>

template<typename T>
T ToWideStringImpl(char const* str, ECharEncoding encoding)
{
	auto codepoint = (encoding == ECharEncoding::Ansi) ? CP_ACP : CP_UTF8;
	int count = MultiByteToWideChar(codepoint, 0, str, -1, NULL, 0);
	T wstr = StringFactory<T>::MakeSized(count + 1);
	MultiByteToWideChar(codepoint, 0, str, -1, &wstr[0], count);

	return wstr;
}

std::wstring ToWideString(char const* str, ECharEncoding encoding)
{
	return ToWideStringImpl<std::wstring>(str, encoding);
}

HeapWString ToWideHeapString(char const* str, ECharEncoding encoding)
{
	return ToWideStringImpl<HeapWString>(str, encoding);
}

template<typename T>
T ToNarrowStringImpl(wchar_t const* wstr, ECharEncoding encoding)
{
	auto codepoint = (encoding == ECharEncoding::Ansi) ? CP_ACP : CP_UTF8;
	int count = WideCharToMultiByte(codepoint, 0, wstr, -1, NULL, 0, NULL, NULL);
	T str = StringFactory<T>::MakeSized(count);
	WideCharToMultiByte(codepoint, 0, wstr, -1, &str[0], count, NULL, NULL);
	return str;
}

std::string ToNarrowString(wchar_t const* wstr, ECharEncoding encoding)
{
	return ToNarrowStringImpl<std::string>(wstr, encoding);
}

HeapString ToNarrowHeapString(wchar_t const* wstr, ECharEncoding encoding)
{
	return ToNarrowStringImpl<HeapString>(wstr, encoding);
}
