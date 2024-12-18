
#ifndef USE_UVS
#define USE_UVS 1
#endif

struct VSOut {
#if USE_UVS
    float2 uv : TexCoord;
#endif
	float4 pos : SV_POSITION;
};

struct VSIn
{
    float3 pos : POSITION;
#if USE_UVS
    float2 uv : TEXCOORD;
#endif
};

VSOut main(VSIn vsi)
{

	VSOut vso;
	vso.pos = float4(vsi.pos.xyz,1);
#if USE_UVS
    vso.uv = vsi.uv;
#endif
	return vso;
}