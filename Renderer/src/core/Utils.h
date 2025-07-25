#pragma once
#include "CoreTypes.h"
#include "core/Types.h"
#include <string>
#include <vector>

#define Zero(x) memset(&x, 0, sizeof(x))

#define ZE_COPY_PROTECT(ClassName) ClassName(ClassName const& other) = delete;\
								ClassName& operator=(ClassName const& other) = delete;
#define ZE_MOVE_PROTECT(ClassName) ClassName(ClassName&& other) = delete;\
								ClassName& operator=(ClassName&& other) = delete;
#define RCOPY_MOVE_PROTECT(ClassName) ZE_COPY_PROTECT(ClassName) ZE_MOVE_PROTECT(ClassName)

#define RCOPY_DEFAULT(ClassName) ClassName(ClassName const& other) = default;\
								ClassName& operator=(ClassName const& other) = default;

#define RMOVE_DEFAULT(ClassName) ClassName(ClassName&& other) noexcept = default;\
								ClassName& operator=(ClassName&& other) noexcept = default;

void AssertionFailed(bool fatal, const char* file, u32 line, const char* fmt, ...);

#define RASSERT_IMPL(expr, stringexpr, ...)\
	if (!(expr))\
	{\
		AssertionFailed(true, __FILE__, __LINE__, "Assertion failed: " stringexpr, __VA_ARGS__);\
		__debugbreak();\
	}

#define ZE_ASSERT(expr, ...) RASSERT_IMPL(expr, #expr, __VA_ARGS__)
#define Assertf(expr, msg, ...) RASSERT_IMPL(expr, msg, __VA_ARGS__)
// An assertion that will always evaluate the containing expression even if assertions are disabled
#define CHECK_SUCCEEDED(expr, ...) RASSERT_IMPL(expr, #expr, __VA_ARGS__)
#define ZE_ASSERT_DEBUG(expr, ...) ZE_ASSERT(expr, __VA_ARGS__)
#define RCHECK(expr, ...) ((expr) || ([&] { RASSERT_IMPL(false, #expr); return false; })())
#define ZE_ENSURE(expr, ...) RCHECK(expr, __VA_ARGS__)
#define ZE_REQUIRE(expr, ...) \
	if (!ZE_ENSURE(expr))              \
	{                         \
		return __VA_ARGS__;\
	}

#define NULL_OR(ptr, expr) (((ptr) == nullptr) ? nullptr : ((ptr)->expr))

template<typename TEnum>
constexpr auto Denum(TEnum e)
{
	return static_cast<std::underlying_type_t<TEnum>>(e);
}

#define DECLARE_ENUM_OP(EType, op)\
	constexpr EType operator op (EType a, EType b) \
	{                                 \
		return EnumCast<EType>(Denum(a) op Denum(b));      \
	}\
	constexpr EType operator op (EType a, std::underlying_type_t<EType> b) \
	{                                 \
		return EnumCast<EType>(Denum(a) op b);      \
	}

#define DECLARE_ENUM_ASSOP(EType, op, assop)\
	constexpr EType& operator assop (EType& a, EType b) \
	{                                 \
		return (a = a op b);\
	}

#define FLAG_ENUM(EType)\
	DECLARE_ENUM_OP(EType, |)\
	DECLARE_ENUM_OP(EType, &)\
	DECLARE_ENUM_OP(EType, ^)\
	DECLARE_ENUM_ASSOP(EType, |, |=)\
	DECLARE_ENUM_ASSOP(EType, &, &=)\
	DECLARE_ENUM_ASSOP(EType, ^, ^=)\
	constexpr bool operator==(EType enumVal, std::underlying_type_t<EType> other)\
	{\
		return Denum(enumVal) == other;\
	}\
	constexpr bool operator!=(EType enumVal, std::underlying_type_t<EType> other)\
	{\
		return Denum(enumVal) != other;\
	}\
	constexpr bool operator !(EType enumVal)\
	{\
		return enumVal == 0;\
	}\
	constexpr EType operator~(EType enumVal)\
	{\
		return EnumCast<EType>(~Denum(enumVal));\
	}\

#define DECLARE_ENUM_UNOP(EType, op, valop)\
	constexpr EType& operator op(EType& val)\
	{                                \
		return (val = EnumCast<EType>(Denum(val) valop));\
	}

#define ITER_ENUM(EType)\
	DECLARE_ENUM_UNOP(EType, ++, +1)\
	DECLARE_ENUM_UNOP(EType, --, -1)

#define POD_EQUALITY(Type)\
	bool operator==(Type const& other) const\
	{\
		static_assert(sizeof(*this) == sizeof(Type));\
		return memcmp(this, &other, sizeof(Type)) == 0;\
	}\
	bool operator!=(Type const& other) const\
	{\
		return !(*this == other);\
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
using ApplyConst = typename TApplyConst<T,IsConst>::Type;

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
constexpr auto Addr(std::vector<T>& v)
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
constexpr u32 Sizeof(std::span<T> s)
{
	return static_cast<u32>(s.size() * sizeof(T));
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

inline bool IsValid(s32 ref)
{
	return ref >= 0;
}

template<typename T>
bool IsValid(const std::weak_ptr<T>& weakPtr)
{
	return !weakPtr.expired();
}

template<typename T>
T* Get(const std::weak_ptr<T>& weakPtr)
{
	if (auto strong = weakPtr.lock())
	{
		return strong.get();
	}

	return nullptr;
}

//template<typename TPtr>
//	requires requires(TPtr p) { *p; }
template<typename T>
inline bool IsValid(T* const ptr)
{
	return ptr != nullptr;
}

template<typename T>
inline bool IsValid(std::shared_ptr<T> const& ptr)
{
	return ptr != nullptr;
}

template<typename T>
inline bool IsValid(std::unique_ptr<T> const& ptr)
{
	return ptr != nullptr;
}


template<typename T, typename TFrom>
constexpr T EnumCast(TFrom from)
{
	return static_cast<T>(from);
}

template<typename E>
bool HasAnyFlags(E val, E flag)
{
	return !!(val & flag);
}

inline bool HasAnyFlags(u32 val, u32 flag)
{
	return val & flag;
}

template<typename E>
bool HasAllFlags(E val, E flags)
{
	return (val & flags) == flags;
}


template<typename T>
using RefWrap = std::reference_wrapper<T>;

template<typename Derived, typename Base>
concept DerivedStrip = std::derived_from<std::remove_cvref_t<Derived>, Base>;

bool FindIgnoreCase(const std::string_view& haystack, const std::string_view& needle);

#define ARRAY_SIZE(arr) sizeof(arr) / sizeof(arr[0])
#define STR_LEN(str) (ARRAY_SIZE(str) - 1)

#define AT_START(block) static uint8_t sStaticInitializer##__LINE__ = [] { block }();

extern unsigned char const ZerosArray[1024];

template<typename T>
T const* GetZeroData(size_t requiredSize)
{
	ZE_ASSERT(requiredSize * sizeof(T) < sizeof(ZerosArray));
	return reinterpret_cast<T const*>(ZerosArray);
}

template<typename InContainer, typename OutContainer>
void Append(OutContainer& to, InContainer const& from)
{
	to.insert(to.end(), from.begin(), from.end());
}


template<typename InContainer, typename OutContainer>
void AppendMove(OutContainer& to, InContainer&& from)
{
	to.insert(to.end(), std::move_iterator(from.begin()), std::move_iterator(from.end()));
}

template<typename... Args>
struct TMaxSize
{
};
template<typename T, typename... Args>
struct TMaxSize<T, Args...>
{
#ifdef max
#undef max
#endif
	constexpr static u32 size = std::max<u32>(sizeof(T), TMaxSize<Args...>::size);
};

template<>
struct TMaxSize<>
{
	constexpr static u32 size = 0;
};

template<typename... Args>
constexpr u32 MaxSize()
{
	return TMaxSize<Args...>::size;
}

template<typename T>
bool SetWasChanged(T& out, T const& value)
{
	bool wasChanged = out != value;
	out = value;
	return wasChanged;
}

//template<typename... Args>
//using Variant = std::Variant<Args...>;
