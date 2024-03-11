
cbuffer VS2DCBuff
{
	float2 offset;
	float2 size;
}

struct VSOut {
	float2 uv: TexCoord;
	float4 pos : SV_POSITION;
};

VSOut main( float3 pos : POSITION , float2 uv : TexCoord)
{

	VSOut vso;
	float2 pos2 = pos.xy * size + offset;
	vso.pos = float4(pos2, pos.z,1);
	vso.uv = uv;
	return vso;
}