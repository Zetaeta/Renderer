#pragma once

#include "BaseDefines.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include <glm/gtx/compatibility.hpp>
#define _USE_MATH_DEFINES
#include <math.h>
#include <bit>


#define ASSERT(...) assert(__VA_ARGS__)

using namespace glm;

using s32 = i32;
using s64 = i64;
using s16 = i16;
using s8 = i8;

using byte = u8;

using uint2 = uvec2;
using uint3 = uvec3;
using uint4 = uvec4;

template<typename T, int N, qualifier Q>
constexpr auto operator,(const vec<N,T,Q>& v1, const vec<N,T,Q>& v2)
{
	return glm::dot(v1,v2);
}

constexpr inline u32 rgba(u8 r, u8 g, u8 b, u8 a)
{
	return r | g << 8 | b << 16 | a << 24;
}

constexpr inline u8 r(u32 colour)
{
	return u8(colour);
}

constexpr inline u8 g(u32 colour)
{
	return u8(colour >> 8);
}

constexpr inline u8 b(u32 colour)
{
	return u8(colour >> 16);
}

constexpr inline u8 a(u32 colour)
{
	return u8(colour >> 24);
}

constexpr float const GAMMA = 1.f/2.2f;

constexpr inline vec4 rgbaToVec(u32 colour)
{
	return vec4(pow(r(colour)/255.f, 1/GAMMA), pow(g(colour)/255.f,1/GAMMA), pow(b(colour)/255.f,1/GAMMA), pow(a(colour)/255.f,1/GAMMA));
}

constexpr inline u32 vecToRGBA(vec4 colour)
{
	return rgba(u8(pow(colour.r, GAMMA) * 255.f), u8(pow(colour.g,GAMMA) * 255), u8(pow(colour.b,GAMMA) * 255.f), u8(pow(colour.a,GAMMA) * 255.f));
}

template <typename TCont>
auto LinearCombo(vec3 coeffs, TCont const& vecs) -> std::remove_reference_t<decltype(vecs[0])> {
	return vecs[0] * coeffs[0] + vecs[1] * coeffs[1] + vecs[2] * coeffs[2];
}

inline vec3 TransVec(mat4 const& m, vec3 const& v)
{
	return vec3(m * vec4(v,0.f));
}

inline vec3 TransPos(mat4 const& m, vec3 const& p)
{
	return vec3(m * vec4(p,1.f));
}

using col3 = vec3;

//using pos3 = vec3;

struct pos3 : public vec3
{
	template<typename... TArgs>
	pos3(TArgs&&... args)
		: vec3(std::forward<TArgs>(args)...) {}

	constexpr pos3(float x, float y, float z)
		: vec3(x, y, z) {}

	constexpr pos3(float f)
		: vec3(f) {}

	constexpr pos3(int x)
		: vec3(float(x)) {}

	constexpr static pos3 Max()
	{
		return pos3(std::numeric_limits<value_type>::max());
	}

	constexpr static pos3 Min()
	{
		return pos3(std::numeric_limits<value_type>::max());
	}
};

namespace maths
{

inline pos3 Min(pos3 const& a, pos3 const& b)
{
	return
	pos3 {
		min(a.x, b.x),
		min(a.y, b.y),
		min(a.z, b.z)
	};
}

inline pos3 Max(pos3 const& a, pos3 const& b)
{
	return
	pos3 {
		max(a.x, b.x),
		max(a.y, b.y),
		max(a.z, b.z)
	};
}

}

inline bool operator>=(const vec3& v, float f)
{
	return v.x >= f && v.y >= f && v.z >= f;
}

using pos = pos3;
using col4 = vec4;

template<typename TTo, typename TFrom>
inline TTo NumCast(TFrom from)
{
	return static_cast<TTo>(from);
}

namespace bits
{
constexpr u64 Mask32 = std::numeric_limits<u32>::max();
}

template<typename Unsigned>
constexpr Unsigned RoundUpLog2(Unsigned value)
{
	return sizeof(Unsigned) * 8 - std::countl_zero(value - 1);
} 

template<typename Unsigned>
constexpr Unsigned RoundUpToPowerOf2(Unsigned value)
{
	if (value == 0)
	{
		return 0;
	}

	return 1 << RoundUpLog2(value);
} 

template<typename Unsigned>
constexpr Unsigned Pow2(Unsigned value)
{
	return 1 << value;
}

template<typename T>
	requires(std::is_integral_v<T>)
inline T DivideRoundUp(T x, T y)
{
	return x / y + (x % y != 0);
}

template<typename T, qualifier Q>
vec<2, T, Q> DivideRoundUp(vec<2, T, Q> const& x, vec<2, T, Q> const& y)
{
	return {DivideRoundUp(x.x, y.x), DivideRoundUp(x.y, y.y)};
}

template<typename T, qualifier Q>
vec<3,T, Q> DivideRoundUp(vec<3, T, Q> const& x, vec<3, T, Q> const& y)
{
	return {DivideRoundUp(x.x, y.x), DivideRoundUp(x.y, y.y), DivideRoundUp(x.z, y.z)};
}
