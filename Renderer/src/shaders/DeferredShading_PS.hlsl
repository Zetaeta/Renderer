
#define EMISSIVE 1
#define OPAQUE 1

#include "PerLightShading.hlsli"

cbuffer ShadingInstanceData
{
    Matrix screen2World;
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
Texture2D ambientOcclusion : register(t4);
#endif

SHADOWMAP_DECLARATION( : register(t5));

#include "Material.hlsli"

float GetActualDepth(float depth)
{
    return depth;
}

PixelLightingInput UnpackGBuffer(float2 uv)
{
    PixelLightingInput result;
    int3 texel = int3(screenSize * uv, 0.5);
    float4 colRough = baseColourRough.Load(texel);
    float4 normMet = sceneNormalMetal.Load(texel);
    float4 emiss = sceneEmissive.Load(texel);
    result.colour = colRough.xyz;
    result.roughness = colRough.w;
    result.normal = UnpackNormalFromUnorm(normMet.xyz);
    result.metalness = normMet.w;
    result.emissive = emiss.xyz;
    
    float bufferDepth = sceneDepth.Load(texel).x;
    if (bufferDepth >= 1 || bufferDepth <= 0.00001)
    {
        discard;
    }

    float3 clipSpacePos = float3(uv.x * 2 - 1, 1 - uv.y * 2, GetActualDepth(bufferDepth));
    float4 position = mul(screen2World, float4(clipSpacePos, 1));
    result.worldPos = position.xyz / position.w;
    result.viewDir = result.worldPos - cameraPos;
    result.specularStrength = 1;
    result.diffuseStrength = 1;
    result.ambientStrength = 1;

    return result;
}

float4 main(float2 uv : TexCoord) : SV_TARGET
{
    PixelLightingInput li = UnpackGBuffer(uv);
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