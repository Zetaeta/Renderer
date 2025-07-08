#pragma once
#include "core/Maths.h"
#include "Noise.h"
#include <numeric>
#include <algorithm>

// Ref https://mrl.cs.nyu.edu/~perlin/noise/

// https://en.wikipedia.org/wiki/Smoothstep
template<typename T>
inline T SmoothStep2(T t)
{
	return t * t * t * (t * (6 * t - 15) + 10);
}

class PerlinNoise2 : public NoiseFunction<vec2, float>
{
public:
	template<typename URBG>
	PerlinNoise2(float frequency, u32 periodicity, URBG&& randomGen)
		: mFrequency(frequency)
	{
		assert(RoundUpToPowerOf2(periodicity) == periodicity);
		periodicityMask = periodicity - 1;
		permutation.resize(periodicity);
		auto begin = permutation.begin();
		auto end = permutation.end();
		std::iota(begin, end, 0);
		std::shuffle(begin, end, randomGen);
	}

	float Evaluate(const vec2& inCoords) override;

	NoiseFunction& operator*=(float mul) override
	{
		amplitude *= mul;
		return *this;
	}

	IMPLEMENT_CLONE(NoiseFunction, PerlinNoise2)

private:
	Vector<u16> permutation;
	u32 periodicityMask;
	float amplitude = 1.f;
	float mFrequency = 1.f;
};
