#pragma once
#include <string>
#include <vector>
#include "maths.h"

#define Zero(x) ZeroMemory(&x, sizeof(x))

#define RCOPY_PROTECT(ClassName) ClassName(ClassName const& other) = delete;\
								ClassName& operator=(ClassName const& other) = delete;
#define RMOVE_PROTECT(ClassName) ClassName(ClassName&& other) = delete;\
								ClassName& operator=(ClassName&& other) = delete;
#define RCOPY_MOVE_PROTECT(ClassName) RCOPY_PROTECT(ClassName) RMOVE_PROTECT(ClassName)

#define RASSERT(expr, ...) assert(expr)

template<typename TEnum>
constexpr auto Denum(TEnum e)
{
	return static_cast<std::underlying_type_t<TEnum>>(e);
}

template<typename TInt>
class range
{
public:
	class iterator
	{
		friend class range;

	public:
		TInt		operator*() const { return i_; }
		const iterator& operator++()
		{
			++i_;
			return *this;
		}
		iterator operator++(int)
		{
			iterator copy(*this);
			++i_;
			return copy;
		}

		bool operator==(const iterator& other) const { return i_ == other.i_; }
		bool operator!=(const iterator& other) const { return i_ != other.i_; }

	protected:
		iterator(TInt start)
			: i_(start) {}

	private:
		TInt i_;
	};

	iterator begin() const { return begin_; }
	iterator end() const { return end_; }
	range(TInt begin, TInt end)
		: begin_(begin), end_(end) {}

private:
	iterator begin_;
	iterator end_;
};

template <typename T, bool IsConst>
struct TApplyConst
{
	using Type = T;
};
template <typename T >
struct TApplyConst<T, true>
{
	using Type = const T;
};

template <typename T, bool IsConst>
using ApplyConst = TApplyConst<T,IsConst>::Type;

template <typename TCont>
class BracketAccessed
{
public:
	using size_type = u32;

	template <bool IsConst>
	class Row
	{
	public:
		auto operator[](size_type y) -> decltype(this->m_Cont.at(this->m_x,y)) {
			return m_Cont->at(m_x,y);
		}
	private:
		ApplyConst<TCont, IsConst>* m_Cont;
		size_type m_x;
	};

	template <typename TAccessor, bool IsConst>
	class TemplateRow
	{
	public:
		auto operator[](TAccessor y) -> decltype(this->m_Cont.at(this->m_x,y)) {
			return m_Cont->at(m_x,y);
		}
	private:
		ApplyConst<TCont, IsConst>* m_Cont;
		TAccessor m_x;
	};

	Row<false> operator[](size_type x) {
		return Row<false>{this, x};
	}
	Row<true> operator[](size_type x) const {
		return Row<false>{this, x};
	}

	template<typename TAccessor>
	TemplateRow<TAccessor, false> operator[](TAccessor x) {
		return TemplateRow<TAccessor, false>{this, x};
	}
	template<typename TAccessor>
	TemplateRow<TAccessor, true> operator[](TAccessor x) const {
		return TemplateRow<TAccessor, false>{this, x};
	}
};


template<typename T, typename TContainer = std::vector<std::vector<T>>>
std::vector<T> Flatten(TContainer const& vecs)
{
	std::vector<T> ret;
	for (const auto& v : vecs)
	{
		ret.insert(ret.end(), v.begin(), v.end());
	}
	return ret;
}

std::vector<std::string> Split(const std::string& s, char delim);

template<typename T, size_t N>
struct ArrayHasher
{
	std::size_t operator()(const std::array<T, N>& a) const
	{
		std::size_t h = 0;

		for (auto e : a)
		{
			h ^= std::hash<T>{}(e) + 0x9e3779b9 + (h << 6) + (h >> 2);
		}
		return h;
	}
};

template<typename T>
constexpr auto Addr(T const& t)
{
	return &t;
}

template<typename T>
constexpr auto Addr(T const* t)
{
	return t;
}

template<typename T>
constexpr auto Addr(std::vector<T> const& v)
{
	return v.empty() ? nullptr : &v[0];
}

template<typename T>
constexpr u32 Size(T const& t)
{
	return static_cast<u32>(std::size(t));
}

template<typename T>
constexpr u32 Sizeof(T const& t)
{
	return static_cast<u32>(sizeof(t));
}

template<typename T>
constexpr u32 Sizeof(std::vector<T> const& v)
{
	return static_cast<u32>(v.size() * sizeof(T));
}

template<typename TStream>
auto& operator<<(TStream& stream, vec3& v)
{
	return stream << '(' << v.x << ',' << v.y << ',' << v.z << ')';
}

#define CHECK(Type, member)   \
	requires(const Type& t) { \
		t.member;\
	}

template<typename TTo, typename TFrom>
TTo& Convert(TFrom const& in, TTo& out)
{
#define TRANSFERVAL(valname)                                    \
{                        \
		constexpr bool a = CHECK(TTo, valname);\
		constexpr bool b = CHECK(TFrom, valname);\
		if constexpr (a && b) \
		{                                                           \
			out.valname = in.valname;\
		}\
	}

	TRANSFERVAL(r);
	TRANSFERVAL(g);
	TRANSFERVAL(b);
	TRANSFERVAL(a);
	TRANSFERVAL(x);
	TRANSFERVAL(y);
	TRANSFERVAL(z);
	TRANSFERVAL(w);
	return out;
}

template<typename TTo, typename TFrom>
TTo Convert(TFrom const& in)
{
	TTo out;
	return Convert(in, out);
}

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


//template<char... cs>
//constexpr auto operator ""_static() -> StaticString < sizeof...(cs) + 1>
//{
//
//	result.str[sizeof...(cs)] = '\0';
//	char* dst = result.str;
//	for (char c : {cs...})
//	{
//		*dst++ = c;
//	}
//	return result;
//}

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

// https://stackoverflow.com/questions/23999573/convert-a-number-to-a-string-literal-with-constexpr
namespace detail
{
	template <unsigned... digits>
	struct to_chars
	{
		constexpr static const char value[] = { ('0' + digits)..., 0 };
	};

	template <unsigned rem, unsigned... digits>
	struct explode : explode<rem / 10, rem % 10, digits...>
	{
	};

	template <unsigned... digits>
	struct explode<0, digits...> : to_chars<digits...>
	{
	};
} // namespace detail

template <unsigned num>
struct U32ToStr : ::detail::explode<num>
{
};


template <unsigned num>
using U32ToStr_v = U32ToStr<num>::value;
