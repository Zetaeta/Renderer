#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/matrix_operation.hpp>

#define ASSERT(...) assert(__VA_ARGS__)

using namespace glm;

template<typename TVec>
constexpr auto operator,(const TVec& v1, const TVec& v2)
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

constexpr inline vec4 rgbaToVec(u32 colour)
{
	return vec4(r(colour)/255.f, g(colour)/255.f, b(colour)/255.f, a(colour)/255.f);
}

constexpr inline u32 vecToRGBA(vec4 colour)
{
	return rgba(colour.r * 255.f, colour.g * 255.f, colour.b * 255.f, colour.a * 255.f);
}

template <typename TCont>
auto LinearCombo(vec3 coeffs, TCont const& vecs) -> std::remove_reference_t<decltype(vecs[0])> {
	return vecs[0] * coeffs[0] + vecs[1] * coeffs[1] + vecs[2] * coeffs[2];
}
