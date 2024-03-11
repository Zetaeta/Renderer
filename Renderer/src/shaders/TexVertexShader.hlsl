#include "VertexShader.hlsli"

#ifndef	TEXTURED
#define TEXTURED 1
#endif

struct VSOut {
#if TEXTURED
	float2 uv: TexCoord;
#endif //TEXTURED
	float3 normal: Normal;
	float3 tangent: Tangent;
	float3 viewDir: ViewDir;
	float3 colour : Colour;
	float4 pos : SV_POSITION;
};


VSOut main( float3 pos : POSITION, float3 n: Normal, float3 t : Tangent, float2 uv : TexCoord)
{
	VSOut vso;
	vso.pos = mul(fullTrans,float4(pos, 1));
	float4 worldPos = mul(model2Shade, float4(pos,1));
	vso.colour = worldPos.xyz;//(n + float3(1,1,1))/2;
	vso.normal = mul(model2ShadeDual, float4(n,0)).xyz;
	vso.tangent = mul(model2Shade, float4(t,0)).xyz;
	vso.viewDir = mul(model2Shade, float4(pos,1)).xyz - cameraPos;
#if TEXTURED
	vso.uv = uv;
#endif //TEXTURED
	return vso;
}
