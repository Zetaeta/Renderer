#ifndef MASKED
#define MASKED 0
#endif

cbuffer PerInstancePSData
{
	uint screenObjectId;
#ifdef MASKED
	float maskCutoff;
#endif
}

#if MASKED
Texture2D baseColourWithMask;
SamplerState splr;
#endif

uint main(
#if MASKED
float2 uv: TexCoord
#endif
) : SV_TARGET
{
#if MASKED
	if (baseColourWithMask.Sample(splr, uv) < maskCutoff)
	{
		discard;
	}
#endif
	return screenObjectId;
}