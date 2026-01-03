#ifndef DEFERREDSHADINGCOMMON_HLSLI
#define DEFERREDSHADINGCOMMON_HLSLI

#include "Maths.hlsli"

float3 PackNormalToUnorm(float3 normalVector)
{
    return (normalVector + float3(1, 1, 1)) * 0.5;
}

float3 UnpackNormalFromUnorm(float3 texNormal)
{
    return (texNormal * 2) - float3(1, 1, 1);
}

float GetActualDepth(float depth)
{
    return depth;
}

struct PixelLightingInput
{
    float3 colour;
#if TRANSLUCENT || MASKED
    float opacity;
#endif
    float3 normal;
    float roughness;
#if EMISSIVE
    float3 emissive;
#endif
    float metalness;
    float3 worldPos;
    float ambientStrength;
    float3 viewDir;
    float diffuseStrength;
    float specularStrength;
};

#if GBUFFER_READ
PixelLightingInput UnpackGBuffer(float2 uv, uint2 screenSize)
{
    PixelLightingInput result;
    int3 texel = int3(screenSize * uv, 0);
    float4 colRough = baseColourRough.Load(texel);
    float4 normMet = sceneNormalMetal.Load(texel);
    result.colour = colRough.xyz;
    result.roughness = colRough.w;
    result.normal = UnpackNormalFromUnorm(normMet.xyz);
    result.metalness = normMet.w;
#if EMISSIVE
    float4 emiss = sceneEmissive.Load(texel);
    result.emissive = emiss.xyz;
#endif
    
    float bufferDepth = sceneDepth.Load(texel).x;
    if (bufferDepth >= 1 || bufferDepth <= 0.00001)
    {
        discard;
    }

    float3 clipSpacePos = float3(uv.x * 2 - 1, 1 - uv.y * 2, GetActualDepth(bufferDepth));
    float4 position = mul(screen2World, float4(clipSpacePos, 1));
    result.worldPos = position.xyz / position.w;
    float3 cameraPos = Dehomogenize(mul(screen2World, float4(0,0,-1,0)));
    result.viewDir = result.worldPos - cameraPos;
    result.specularStrength = 1;
    result.diffuseStrength = 1;
    result.ambientStrength = 1;

    return result;
}
#endif


#endif // DEFERREDSHADINGCOMMON_HLSLI
