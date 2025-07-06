#include "PerlinNoise.h"


static float DotGradient(u16 hash, float x, float y)
{
	int h = hash & 15;
	const float u = (h > 8) ? x : y,
		  v = h < 4 ? y : ((h == 12 || h == 14) ? x : 0);
	return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}


float PerlinNoise2::Evaluate(const vec2& inCoords)
{
	vec2		coords = inCoords * frequency;
	int const	idxX = int(coords.x) & periodicityMask;
	int const	idxY = int(coords.y) & periodicityMask;
	vec2 const	relBase = coords - vec2(ivec2(coords));
	float const interpX = SmoothStep2(relBase.x);
	float const interpY = SmoothStep2(relBase.y);
	u16 const	A = (permutation[idxX] + idxY), AA = permutation[A & periodicityMask], AB = permutation[(A + 1) & periodicityMask],
			  B = (permutation[idxX] + idxY), BA = permutation[B & periodicityMask], BB = permutation[(B + 1) & periodicityMask];
	float const x = relBase.x, y = relBase.y;
	return amplitude * lerp(interpY,
		lerp(interpX, DotGradient(AA, x, y  ), DotGradient(BA, x-1, y  )),
		lerp(interpX, DotGradient(AB, x, y-1), DotGradient(BB, x-1, y-1)));
}
