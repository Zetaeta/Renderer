
cbuffer PointShadow
{
#if MASKED
	float maskCutoff;
#endif
	float far;
}

struct PSOut {
	float4 color : SV_Target;
	float depth: SV_Depth;
};
#if MASKED
Texture2D baseColourWithMask;
SamplerState splr;
#endif

PSOut main(float3 lightSpacePos: LightSpacePos
#if MASKED
	, float2  uv: TexCoord
#endif
)
{
#if MASKED
	if (baseColourWithMask.Sample(splr, uv) < maskCutoff)
	{
		discard;
	}
#endif
	PSOut pso;
	//pso.color = float4(1,1,1,1);
	pso.depth = (length(lightSpacePos)/ 100);
	return pso;
}