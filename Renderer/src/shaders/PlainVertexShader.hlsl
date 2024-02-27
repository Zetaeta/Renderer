#include "VertexShader.hlsli"

struct VSOut {
	float3 normal: Normal;
	float3 tangent: Tangent;
	float3 viewDir: ViewDir;
	float3 colour : Colour;
//	float4 lightPos: LightPos;
	float4 pos : SV_POSITION;
	float2 uv: TexCoord;
};


VSOut main( float3 pos : POSITION, float3 n: Normal, float3 t : Tangent, float2 uv : TexCoord
)
{
	VSOut vso;
	vso.pos = mul(fullTrans,float4(pos, 1));
	float4 worldPos = mul(model2Shade, float4(pos,1));
//	vso.lightPos = mul(world2Light, worldPos);
	vso.colour = (n + float3(1,1,1))/2;
	vso.colour = worldPos.xyz;//(n + float3(1,1,1))/2;
	vso.normal = mul(model2Shade, float4(n,0)).xyz;
	vso.viewDir = mul(model2Shade, float4(pos,1)).xyz - cameraPos;
	vso.uv = uv;
	return vso;
}