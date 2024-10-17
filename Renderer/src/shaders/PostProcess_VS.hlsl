
struct VSOut {
	float4 pos : SV_POSITION;
};

VSOut main( float3 pos : POSITION)
{

	VSOut vso;
	vso.pos = float4(pos.xyz,1);
	return vso;
}