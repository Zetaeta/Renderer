
#include "DeferredShadingCommon.hlsli"

#define NOISE_SIZE 4
#define MAX_SAMPLES 64
#define hemisphere 1
#define normalType 3

#ifndef PIXEL_DEBUGGING
#define PIXEL_DEBUGGING 1
#endif

cbuffer FrameInfo
{
    float4x4 projectionMat;
    float4x4 inverseProjection;
    float4x4 world2ScreenSpace;
    uint2 screenSize;
    float threshold;
    float radius;
    float4 randomRotationVecs[NOISE_SIZE * NOISE_SIZE / 2];
    float4 samples[MAX_SAMPLES];
    uint numSamples;
#if PIXEL_DEBUGGING
    int _pad;
    uint2 debugPixel;
#endif
//    uint normalType;
//    bool hemisphere;
};

Texture2D<float> sceneDepth;
Texture2D<float4> sceneNormals;
RWTexture2D<float> ambientOcclusion;
#if PIXEL_DEBUGGING
RWTexture2D<float4> pixelDebugOut;
#endif

//float3 GetWorldFromScreen(uint2 pixCoord, float bufferDepth)
//{
//    float2 uv = float2(pixCoord) / screenSize;
//    float3 clipSpacePos = float3(uv.x * 2 - 1, 1 - uv.y * 2, bufferDepth);
//    float4 homog = mul(screen2World, float4(clipSpacePos, 1));
//    return homog.xyz / homog.w;
//}

float3 ScreenSpaceToViewSpace(uint2 pixCoord, float bufferDepth)
{
    
    float2 uv = float2(pixCoord) / screenSize;
    float3 clipSpacePos = float3(uv.x * 2 - 1, 1 - uv.y * 2, bufferDepth);
    float4 homog = mul(inverseProjection, float4(clipSpacePos, 1));
    return homog.xyz / homog.w;
}

//float GetOcclusionAtOffset(uint2 offset, uint2 pixCoord, float3 normal, float depth)
//{
//    float otherDepth = sceneDepth.Load(uint3(pixCoord + offset, 0));
////    float3 distanceKinda = float3(offset.x, offset.y, (otherDepth - depth) / depth) * 0.0000000001;
//    float3 distanceSS = float3(offset / screenSize * 2, otherDepth - depth);
//    float3 difference = GetWorldFromScreen(pixCoord + offset, otherDepth) - GetWorldFromScreen(pixCoord, depth);
////    return dot(normal, difference);
//    if (dot(normal, distanceSS) < threshold)
//    {
//        return 1;
//    }
//    return 0;

//}

#define CLOSER_THAN <

float3 GetScreenSpaceNormal(uint3 pixCoord3)
{
    float3 packedNormal = sceneNormals.Load(pixCoord3).xyz;
    float3 wsNormal = UnpackNormalFromUnorm(packedNormal);
    float4 homog = mul(world2ScreenSpace, float4(wsNormal, 0));
    return normalize(homog.xyz);
}

[numthreads(8, 8, 1)]
void main( uint3 pixCoord3 : SV_DispatchThreadID )
{
    if (pixCoord3.x >= screenSize.x || pixCoord3.y >= screenSize.y)
    {
        return;
    }
    
    float depth = sceneDepth.Load(pixCoord3);
    float3 normal = float3(0, 0, 1);
    //if (normalType == 1)
    //{
    //    normal = float3(0, 1, 0);
    //}
    //else if (normalType == 2)
    //{
    //    normal = float3(1, 0, 0);
    //}
    //else if (normalType == 3)
    {
        normal = GetScreenSpaceNormal(pixCoord3);
    }
    uint2 noiseCoord = uint2(pixCoord3.x % NOISE_SIZE, pixCoord3.y % NOISE_SIZE);
    float2 unpackedNoise[NOISE_SIZE * NOISE_SIZE] = (float2[NOISE_SIZE * NOISE_SIZE]) randomRotationVecs;
    float3 baseTangent = float3(unpackedNoise[noiseCoord.x + noiseCoord.y * NOISE_SIZE], 0);
    float3 tangent = normalize(baseTangent - dot(baseTangent, normal) * normal);
    float3 bitangent = cross(tangent, normal);
    float3x3 tbn = float3x3(tangent, bitangent, normal);

    float3 screenSpacePos = ScreenSpaceToViewSpace(pixCoord3.xy, depth);

    float occlusion = 0;

    for (int i = 0; i < numSamples; ++i)
    {
        float3 rawSample = samples[i].xyz;
        if (rawSample.z <= 0)
        {
            occlusion = 100;
        }
        float3 sampleVec = rawSample.x * tangent + rawSample.y * bitangent + rawSample.z * normal;
//        float3 sampleVec = mul(tbn, rawSample);
        float4 sample = float4(sampleVec * radius + screenSpacePos, 1);
        float4 sampleClip = mul(projectionMat, sample);
        sampleClip /= sampleClip.w;
        float2 sampleUV = float2(sampleClip.x + 1, 1-sampleClip.y) * 0.5;
        if (abs(sampleClip).x >= 1 || abs(sampleClip).y >= 1)
        {
            continue;
        }
        uint2 samplePixel = uint2(sampleUV * screenSize);
        float depthAtSample = sceneDepth.Load(uint3(samplePixel, 0));
        bool isOccluded = sampleClip.z - depthAtSample > 0.0001f;
        if (isOccluded)
        {
            float3 occludingPos = ScreenSpaceToViewSpace(sampleUV * screenSize, depthAtSample);
            isOccluded = length(occludingPos - screenSpacePos) < threshold;
        }
//        bool isInRange = (sampleClip.z / depthAtSample) - 1 < threshold;
        occlusion += isOccluded;
#if PIXEL_DEBUGGING
        if (debugPixel.x == pixCoord3.x && debugPixel.y == pixCoord3.y)
        {
            float4 debug = float4(0,0,0,1);
            //debug.b = 1;
            if (isOccluded)
            {
                debug.r = 1;// 10000 * ;
            debug.b = (sampleClip.z - depthAtSample) > 0.1f;
            }
            else
            {
                debug.g = 1; //100 * (-sampleClip.z + depthAtSample);
            }

//            debug.
            pixelDebugOut[samplePixel] = debug;
        }
#endif
    }
    occlusion = occlusion / numSamples;
    //pow(occlusion / numSamples, 0.5);
//    occlusion /= numSamples;
    ambientOcclusion[pixCoord3.xy] = 1 - occlusion;

}