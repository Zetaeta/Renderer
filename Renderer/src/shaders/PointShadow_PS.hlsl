
cbuffer PointShadow
{
	float far;
}

struct PSOut {
	float4 color : SV_Target;
	float depth: SV_Depth;
};

PSOut main(float3 lightSpacePos: LightSpacePos)
{
	PSOut pso;
	pso.color = float4(1,1,1,1);
	pso.depth = (length(lightSpacePos) / 100);
	return pso;
}