
#define EMISSIVE 1
#define OPAQUE 1

#include "PerLightShading.hlsli"

cbuffer ShadingInstanceData
{
    float4x4 screen2World;
    float3 cameraPos;
    uint2 screenSize;
};

Texture2D baseColourRough : register(t0);
Texture2D sceneNormalMetal : register(t1);
Texture2D sceneEmissive : register(t2);
Texture2D sceneDepth : register(t3);

SamplerState splr : register(s0);
SamplerComparisonState shadowSampler : register(s1);

#if AMBIENT_OCCLUSION
Texture2D<float> ambientOcclusion : register(t4);
#endif

SHADOWMAP_DECLARATION( : register(t5));

#define GBUFFER_READ 1

#include "Material.hlsli"


float4 main(float2 uv : TexCoord) : SV_TARGET
{
    PixelLightingInput li = UnpackGBuffer(uv, screenSize);
#if AMBIENT_OCCLUSION
    li.ambientStrength *= ambientOcclusion.Sample(splr, uv);
#endif
//    return float4(1, 1, 0, 1);
    float4 colour = DoShading(li);

    return colour// * 0.01
//    + float4(li.viewDir/1000, 1);
//    + float4(li.worldPos/10 + float3(0.5,0.5,0.5), 1);
    //* 0.01 + float4(li.colour, 1);
    ;

}