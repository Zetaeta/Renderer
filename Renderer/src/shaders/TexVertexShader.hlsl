#include "VertexShader.hlsli"

#ifndef	TEXTURED
#define TEXTURED 1
#endif

#ifndef	SHADED
#define SHADED 1
#endif

struct VSOut {
#if TEXTURED
	float2 uv: TexCoord;
#endif //TEXTURED
#if SHADED
	float3 normal: Normal;
	float3 tangent: Tangent;
	float3 viewDir: ViewDir;
	float3 worldPos : WorldPos;
#endif
	float4 pos : SV_POSITION;
};

struct VSIn
{
    float3 pos : POSITION;
#if SHADED
    float3 n : Normal;
    float3 t : Tangent;
#endif
    float2 uv : TexCoord;
};

VSOut main( VSIn vsi)
{
	VSOut vso;
	vso.pos = mul(fullTrans,float4(vsi.pos, 1));
#if SHADED
	float4 worldPos = mul(model2Shade, float4(vsi.pos,1));
	vso.worldPos = worldPos.xyz;//(n + float3(1,1,1))/2;
	vso.normal = mul(model2ShadeDual, float4(vsi.n,0)).xyz;
	vso.tangent = mul(model2Shade, float4(vsi.t,0)).xyz;
	vso.viewDir = mul(model2Shade, float4(vsi.pos,1)).xyz - cameraPos;
#endif
#if TEXTURED
	vso.uv = vsi.uv;
#endif //TEXTURED
	return vso;
}
