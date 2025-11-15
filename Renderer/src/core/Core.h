#pragma once

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

#define RASSERT_IMPL(expr, stringexpr, fatal, ...)\
	if (!(expr))\
	{\
		AssertionFailed(true, __FILE__, __LINE__, "Assertion failed: " stringexpr, __VA_ARGS__);\
		__debugbreak();\
		if (fatal)\
		{                                                                                        \
			std::terminate();\
		}\
	}

#define ZE_ASSERT(expr, ...) RASSERT_IMPL(expr, #expr, true, __VA_ARGS__)
#define Assertf(expr, msg, ...) RASSERT_IMPL(expr, msg, true, __VA_ARGS__)
// An assertion that will always evaluate the containing expression even if assertions are disabled
#define CHECK_SUCCEEDED(expr, ...) RASSERT_IMPL(expr, #expr, true, __VA_ARGS__)
#define ZE_ASSERT_DEBUG(expr, ...) ZE_ASSERT(expr, __VA_ARGS__)
#define RASSERT_IMPL_INLINE(expr, str, fatal, ...)  ((expr) || ([&] { RASSERT_IMPL(false, str, fatal, __VA_ARGS__); return false; })())
#define RCHECK(expr, ...) RASSERT_IMPL_INLINE(expr, #expr, true, __VA_ARGS__)
#define ZE_ENSURE(expr) RASSERT_IMPL_INLINE(expr, #expr, false)
#define ZE_Ensuref(expr, format, ...) RASSERT_IMPL_INLINE(expr, #expr, false, __VA_ARGS__)
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


#define INTEGRAL_PRIMITIVE_TYPES \
	X(u8)\
	X(s8)\
	X(u16)\
	X(s16)\
	X(u32)\
	X(s32)\
	X(u64)\
	X(s64)

#define NUMERIC_PRIMITIVE_TYPES\
	INTEGRAL_PRIMITIVE_TYPES\
	X(float)\
	X(double)

#define PRIMITIVE_TYPES\
	NUMERIC_PRIMITIVE_TYPES\
	X(bool)

