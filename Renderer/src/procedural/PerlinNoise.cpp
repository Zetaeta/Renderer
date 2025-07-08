#include "PerlinNoise.h"
#include "unordered_set"


static float DotGradient(u16 hash, float x, float y)
{
	static const vec2 grads[] = 
	{
		{1,1},
		{1,-1},
		{-1,1},
		{-1,-1},
		{1,0},
		{0,-1},
		{-1,0},
		{0,1}
	};
	return dot(normalize(grads[hash & 7]), vec2(x, y));

	//int h = hash & 15;
	//const float u = (h > 8) ? x : y,
	//	  v = h < 4 ? y : ((h == 12 || h == 14) ? x : 0);
	//return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

static std::unordered_set<u32> printed;
auto linerp(auto a, auto x, auto y)
{
	return lerp(x,y,a);
}

float PerlinNoise2::Evaluate(const vec2& inCoords)
{
	vec2		coords = inCoords * mFrequency;
	int const ix = int(coords.x);
	int const iy = int(coords.y);
	int const	idxX = ix & periodicityMask;
	int const	idxY = iy & periodicityMask;
	vec2 const	relBase = coords - vec2(ix, iy);
	float const interpX = SmoothStep2(relBase.x);
	float const interpY = SmoothStep2(relBase.y);
	u16 const	A = (permutation[idxX] + idxY), AA = permutation[A & periodicityMask], AB = permutation[(A + 1) & periodicityMask],
			  B = (permutation[(idxX+1) & periodicityMask] + idxY), BA = permutation[B & periodicityMask], BB = permutation[(B + 1) & periodicityMask];
	u32 key = idxX;
	key |= (u32(idxY) << 16);
	//if (!printed.contains(key))
	//{
	//	printf("idx=(%d,%d), AA=%d, AB=%d, BA=%d, BB=%d\n", idxX, idxY, AA, AB, BA, BB);
	//	printed.insert(key);
	//}
	assert(relBase.x < 1.001 && relBase.y < 1.001);
	float const x = relBase.x, y = relBase.y;
	return amplitude * linerp(interpY,
		linerp(interpX, DotGradient(AA, x, y  ), DotGradient(BA, x-1, y  )),
		linerp(interpX, DotGradient(AB, x, y-1), DotGradient(BB, x-1, y-1)));
}
