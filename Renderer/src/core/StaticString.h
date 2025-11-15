#pragma once

template <unsigned N>
struct StaticString
{
	constexpr StaticString() {}
	constexpr StaticString(char (&other)[N])
	{
		for (int i=0; i<N; ++i)
		{
			str[i] = other[i];
		}
	}

	char str[N];
};


template<unsigned N>
constexpr auto Static(char const (&str)[N])
{
	StaticString <N> result;
	for (int i=0; i<N; ++i)
	{
		result.str[i] = str[i];
	}
	return result;
}


template <unsigned... Len>
constexpr auto concat(const char (&... strings)[Len])
{
	constexpr unsigned N = (... + Len) - sizeof...(Len);
	StaticString<N + 1>	   result = {};
	result.str[N] = '\0';

	char* dst = result.str;
	for (const char* src : { strings... })
	{
		for (; *src != '\0'; src++, dst++)
		{
			*dst = *src;
		}
	}
	return result;
}
