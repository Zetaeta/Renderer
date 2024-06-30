
cbuffer PointShadow
{
#ifdef MASKED
	float maskCutoff;
#endif
	float far;
}

struct PSOut {
	float4 color : SV_Target;
	float depth: SV_Depth;
};
#ifdef MASKED
Texture2D baseColourWithMask;
SamplerState splr;
#endif

PSOut main(float3 lightSpacePos: LightSpacePos
#ifdef MASKED
	, float2  uv: TexCoord
#endif
)
{
#ifdef MASKED
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