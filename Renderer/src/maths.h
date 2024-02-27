#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/matrix_operation.hpp>

#define ASSERT(...) assert(__VA_ARGS__)

using namespace glm;

using s32 = i32;
using s64 = i64;
using s16 = i16;
using s8 = i8;

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

struct pos3 : public vec3
{
	template<typename... TArgs>
	pos3(TArgs&&... args)
		: vec3(std::forward<TArgs>(args)...) {}
};

using pos = pos3;
