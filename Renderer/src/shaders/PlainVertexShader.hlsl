#include "VertexShader.hlsli"

struct VSOut {
	float3 normal: Normal;
	float3 viewDir: ViewDir;
	float3 colour : Colour;
	float2 uv: TexCoord;
	float4 pos : SV_POSITION;
};


VSOut main( float3 pos : POSITION, float3 n: Normal, float2 uv : TexCoord
)
{
	VSOut vso;
	vso.pos = mul(fullTrans,float4(pos, 1));
	vso.colour = (n + float3(1,1,1))/2;
	vso.normal = mul(model2Shade, float4(n,0)).xyz;
	vso.viewDir = mul(model2Shade, float4(pos,1)).xyz - cameraPos;
	vso.uv = uv;
	return vso;
}