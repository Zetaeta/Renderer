
#ifndef MATHS_HLSLI
#define MATHS_HLSLI

float square(float f)
{
	return f*f;
}
float squareLen(float3 v)
{
	return dot(v,v);
}

float squareLen(float2 v)
{
	return dot(v,v);
}

float2 UvToClip(float2 uv)
{
    return float2(uv.x * 2 - 1, 1 - uv.y * 2);
}

float2 ClipToUv(float2 clip)
{
    return float2((clip.x + 1) / 2, (1 - clip.y) / 2);
}

float3 Dehomogenize(float4 homoCoords)
{
    return homoCoords.xyz / homoCoords.w;
}

#define pdot(a,b) saturate(dot(a,b));

static const float PI = 3.14159265f;
static const float dielectricF0 = 0.04;

#endif
