#ifndef MATERIALDEFINITIONS_HLSLI
#define MATERIALDEFINITIONS_HLSLI

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

struct VSInputs
{
    float3 Position : POSITION;
#if SHADED
    float3 Normal : Normal;
    float3 Tangent : Tangent;
#endif
    float2 UV : TexCoord;
};

#endif // MATERIALDEFINITIONS_HLSLI
