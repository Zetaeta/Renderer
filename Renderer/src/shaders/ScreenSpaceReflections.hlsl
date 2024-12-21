
cbuffer ReflectionParameters
{
    Matrix projWorld;
    Matrix screen2World;
    float reflectionThreshold;
    float maxDist;
    float traceResolution;
    uint maxRefineSteps;
    float hitBias;
    uint2 debugPixel;
//    float globalRoughMod;
//    float globalMetalMod;
};

Texture2D baseColourRough;
Texture2D sceneNormalMetal;
Texture2D sceneDepth;
Texture2D inSceneColour : register(t3);

RWTexture2D<float4> inoutSceneColour;
#if PIXEL_DEBUGGING
RWTexture2D<float4> pixelDebug;
#endif


#define GBUFFER_READ 1
#define EMISSIVE 0
#include "DeferredShadingCommon.hlsli"
#include "Maths.hlsli"

float3 GGXSpecularReflection(float3 normal, float3 diffuseCol, float roughness, float3 viewDir, float metalness, float specularStrength)
{
	viewDir = -viewDir;
	roughness = max(roughness, 0.01);
	float alpha = roughness * roughness;
	float alphasq = alpha * alpha;
    float NH = 1;
	float NV = pdot(normal, viewDir);
    NV	= max (NV, 0.00005);
    float NL = NV;

	float3 specularCol = metalness * diffuseCol + (1-metalness) * float3(dielectricF0,dielectricF0,dielectricF0);
	float3 Fresnel = specularCol + (1-specularCol) * pow(1-NL, 5);
	float NDF = (NH > 0) * alphasq / (PI * pow(NH * NH * (alphasq - 1) + 1, 2));
	float k = alpha/2;

	float G11 = NV / max((NV * (1-k) + k), 0.0001);
	float G12 = NL / max((NL * (1-k) + k), 0.0001);
	float GeomShadowing = G11 * G12;

	return (Fresnel * GeomShadowing * NDF) / max((4 * NL * NV),0.0001) * specularStrength;
}

struct ScreenLocation
{
    uint2 pos;
    float bufferDepth;

};

bool IsValid(ScreenLocation loc, uint2 screenSize)
{
    return (loc.pos.x < screenSize.x) && (loc.pos.y < screenSize.y);
}

float PerspectiveLerpDepth(float start, float end, float alpha)
{
    return (start * end) / lerp(end, start, alpha); // Perspective correct interpolation
}

void DebugPixel(uint2 basePixel, uint2 outPixel, float4 colour)
{
#if PIXEL_DEBUGGING
    if (basePixel.x == debugPixel.x && basePixel.y == debugPixel.y)
    {
        pixelDebug[outPixel] = colour;
        pixelDebug[outPixel + int2(0,1)] = colour;
        pixelDebug[outPixel + int2(1,0)] = colour;
        pixelDebug[outPixel - int2(1,0)] = colour;
        pixelDebug[outPixel - int2(0,1)] = colour;
    }
#endif
}

ScreenLocation ScreenTrace(uint2 screenSize, uint2 startPixel, float startDepth, float3 startPosWld, float3 directionWld, float maxDist, float resolution, uint maxRefineSteps, float distThreshold)
{
    float2 startUv = float2(startPixel) / screenSize;
    float3 endPosWld = startPosWld + directionWld * maxDist;
    float4 endPosH = mul(projWorld, float4(endPosWld, 1));
    float3 endPosClp = endPosH.xyz / endPosH.w;
    float2 endUv = ClipToUv(endPosClp.xy);
    float2 endPixel = endUv * screenSize;
    DebugPixel(startPixel, endPixel, float4(0, 0, 1, 1));
    // TODO: Clamp to screen
    float pixelLengthSq = squareLen(endPixel - startPixel);
    float pixelLength = sqrt(pixelLengthSq);
    float2 step = float2(endPixel - startPixel) / (pixelLength * max(resolution, 0.00001));
    //for (int i = 0; i < 10; ++i)
    //{
    //    DebugPixel(startPixel, startPixel + i * step, float4(1, 0, 1, 1));
    //}
    float stepLen = length(step);
    float length1 = 0.f;
//    float lastDepth 

    ScreenLocation invalid;
    invalid.bufferDepth = 0;
    invalid.pos = screenSize;

    float lastDepth = startDepth;
    float currDepth;
    float2 pixel0 = startPixel+ step;
    float2 pixel1 = pixel0;
    uint2 currPixelu;
    bool foundHit = false;
    float length0;
    // Phase 1 : coarse jumping
    uint numSteps = 0;
    while(length1 <= pixelLength && numSteps++ < 128)
    {
        length1 += stepLen;
        pixel1 += step;
        currPixelu = round(pixel1);
        if (currPixelu.x < 0 || currPixelu.y < 0 || currPixelu.x >= screenSize.x || currPixelu.y >= screenSize.y)
        {
            return invalid;
        }

//#if PIXEL_DEBUGGING
//        if (debugPixel.x == startPixel.x && debugPixel.y == startPixel.y)
//        {
//            pixelDebug[currPixelu] = float4(1,0.5,0,1);
//        }
//#endif
        currDepth = sceneDepth.Load(int3(currPixelu, 0));
        DebugPixel(startPixel, currPixelu, float4(1, 0, 0, 1));
        float lineDepth = PerspectiveLerpDepth(startDepth, endPosClp.z, length1 / pixelLength);

        if (currDepth < lineDepth + distThreshold)
        {
            foundHit = true;
            DebugPixel(startPixel, currPixelu, float4(1, 0, 0, 1));
            DebugPixel(startPixel, currPixelu + int2(1,0), float4(1, 0, 0, 1));
            DebugPixel(startPixel, currPixelu - int2(1,0), float4(1, 0, 0, 1));
            DebugPixel(startPixel, currPixelu - int2(0,1), float4(1, 0, 0, 1));
            DebugPixel(startPixel, currPixelu + int2(0,1), float4(1, 0, 0, 1));
            DebugPixel(startPixel, currPixelu + int2(0,2), float4(1, 0, 0, 1));
            DebugPixel(startPixel, currPixelu + int2(2,0), float4(1, 0, 0, 1));
            break;
        }

        lastDepth = currDepth;
        length0 = length1;
//        lastPixel = currPixelu;
    }
    // Phase 2: refinement (binary search)
    if (!foundHit)
    {
        return invalid;
    }
    for (uint stepNo = 0; stepNo < maxRefineSteps; ++stepNo)
    {
        float2 nextPixel = (pixel1 + pixel0) / 2;
        uint2 nextPixelu = round(nextPixel);
        if (nextPixelu.x == currPixelu.x && nextPixelu.y == currPixelu.y)
        {
            break;
        }

        float currLength = (length0 + length1) / 2;
        float lineDepth = PerspectiveLerpDepth(startDepth, endPosClp.z, currLength / pixelLength); // Perspective correct interpolation
        currDepth = sceneDepth.Load(int3(nextPixelu, 0)).x;
        DebugPixel(startPixel, nextPixelu, float4(0,1,0,1));
        if (currDepth < lineDepth + distThreshold) // behind geometry
        {
            length1 = currLength;
            pixel1 = nextPixel;
        }
        else
        {
            length0 = currLength;
            pixel0 = nextPixel;
        }
    }

    DebugPixel(startPixel, round(pixel1), float4(0, 0, 1, 1));

    ScreenLocation result;
    result.bufferDepth = currDepth;
    result.pos = round(pixel1);
    return result;
}

float3 CalcSSR(uint2 pixel)
{
    
    uint2 screenSize;
    baseColourRough.GetDimensions(screenSize.x, screenSize.y);
    float2 uv = float2(pixel) / screenSize;
    float startDepth = sceneDepth.Load(int3(pixel, 0)).x;
    if (startDepth > 0.99999)
    {
        DebugPixel(pixel, pixel, float4(1, 0, 1, 1));
        return float3(0, 0, 0);
    }
    PixelLightingInput li = UnpackGBuffer(uv, screenSize);

    float3 reflectionStrength = GGXSpecularReflection(li.normal, li.colour, li.roughness, li.viewDir, li.metalness, li.specularStrength);
    if (squareLen(reflectionStrength) < reflectionThreshold * reflectionThreshold)
    {
        DebugPixel(pixel, pixel, float4(1, 1, 0, 1));
        return float3(0, 0, 0);
    }

    float3 revViewDir = -li.viewDir;
    float3 reflectionDir = normalize(-revViewDir + 2 * dot(revViewDir, li.normal) * li.normal);

    ScreenLocation result = ScreenTrace(screenSize, pixel, startDepth, li.worldPos, reflectionDir, maxDist, traceResolution, maxRefineSteps, hitBias);

    if (IsValid(result, screenSize))
    {
        float3 hitNormal = sceneNormalMetal.Load(int3(result.pos, 0)).xyz;
        float visibility = (1 + dot(hitNormal, -reflectionDir)) / 2;
        DebugPixel(pixel, result.pos, float4(1, 0, 1, 1));
        if (visibility > 0)
       {
            DebugPixel(pixel, pixel, float4(1, 1, 1, 1));
        #if COMPUTE
            return visibility * reflectionStrength * inoutSceneColour[result.pos];
        #else
            return visibility * reflectionStrength* inSceneColour.Load(int3(result.pos, 0));
        #endif

        }
    }
    DebugPixel(pixel, pixel, float4(0, 1, 1, 1));
    return float3(0, 0, 0);
}

#if COMPUTE
[numthreads(1, 1, 1)]
void SSR_CS( uint3 pixCoord3 : SV_DispatchThreadID )
{
    inoutSceneColour[pixCoord3.xy] += float4(CalcSSR(pixCoord3.xy), 0);
}
#endif

float4 SSR_PS(float4 pixel : SV_Position) : SV_Target
{
    return inSceneColour.Load(int3(pixel.xy, 0)) * (1 + float4(CalcSSR(pixel.xy), 0));
}