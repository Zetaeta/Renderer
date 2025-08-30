
cbuffer ReflectionParameters
{
    float4x4 projWorld;
    float4x4 screen2World;
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

float3 ReflectionStrength(float3 normal, float3 diffuseCol, float roughness, float3 viewDir, float metalness, float specularStrength)
{
	viewDir = -viewDir;
	float NL = dot(normal, viewDir);
    if (NL > 0)
    {
        float3 specularCol = metalness * diffuseCol + (1-metalness) * float3(dielectricF0,dielectricF0,dielectricF0);
        float3 Fresnel = specularCol + (1-specularCol) * pow(1-NL, 5);
        return clamp(Fresnel / roughness, 0, 1);
    }
    else
    {
        return 0;
    }

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
//    return (start * end) / lerp(end, start, alpha); // Perspective correct interpolation
    // depth buffer is already 1/z lol
    return lerp(start, end, alpha);
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

// line = point + t * dir. returns dir
float LinePlaneIntersection(float3 startPoint, float3 direction, float4 planeEquation)
{
    return -dot(planeEquation, float4(startPoint, 1)) / dot(planeEquation, float4(direction, 0));

}
bool IsObscuredOrOffscreen(uint2 rayPos, float rayDepth, uint2 screenSize, float hitBias, out float depthDiff)
{
    if (rayPos.x < 0 || rayPos.y < 0 || rayPos.x >= screenSize.x || rayPos.y >= screenSize.y)
    {
        depthDiff = -1;
        return true;
    }
    float depthAtPos = sceneDepth.Load(int3(rayPos, 0));
    depthDiff = rayDepth + hitBias - depthAtPos;
    return depthDiff > 0;
}

ScreenLocation ScreenTrace(uint2 screenSize, uint2 startPixel, float startDepth, float3 startPosWld, float3 directionWld, float maxDist, float resolution, uint maxRefineSteps, float distThreshold)
{
    float2 startUv = float2(startPixel) / screenSize;
    float4 nearPlaneEq = projWorld[2];
    // Clip line against near plane
    if (dot(nearPlaneEq, float4(directionWld, 0)) < 0)
    {
        maxDist = min(maxDist, 0.80 * LinePlaneIntersection(startPosWld, directionWld, nearPlaneEq));
    }

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
    float stepLen = length(step);
    float length1 = 0.f;
//    float lastDepth 

    ScreenLocation invalid;
    invalid.bufferDepth = 0;
    invalid.pos = screenSize;

    if (stepLen < 0.8)
    {
        return invalid;
    }

    float lastDepth = startDepth;
    float currDepth;
    float2 pixel0 = startPixel+ step;
    float2 pixel1 = pixel0;
    uint2 currPixelu;
    bool foundHit = false;
    float length0;
    // Phase 1 : coarse jumping
    uint numSteps = 0;
    #if PIXEL_DEBUGGING
    if (startPixel.x == debugPixel.x && startPixel.y == debugPixel.y)
    {
        uint steps = 0;
        float debugLength = 0;
        float2 outPixel = startPixel;
        while (debugLength < pixelLength && steps++ < 128)
        {
            debugLength += step;
            outPixel += step;
            pixelDebug[round(outPixel)] = float4(endPosClp.z < 0,0,1,1);
        }
    }
#endif
    while(length1 <= pixelLength && numSteps++ < 1024)
    {
        length1 += stepLen;
        pixel1 += step;
        currPixelu = round(pixel1);
        //if (currPixelu.x < 0 || currPixelu.y < 0 || currPixelu.x >= screenSize.x || currPixelu.y >= screenSize.y)
        //{
        //    return invalid;
        //}

//#if PIXEL_DEBUGGING
//        if (debugPixel.x == startPixel.x && debugPixel.y == startPixel.y)
//        {
//            pixelDebug[currPixelu] = float4(1,0.5,0,1);
//        }
//#endif
        DebugPixel(startPixel, currPixelu, float4(1, 0.5, 0.5, 1));
        float lineDepth = PerspectiveLerpDepth(startDepth, endPosClp.z, length1 / pixelLength);
        currDepth = sceneDepth.Load(int3(currPixelu, 0));
        float depthDiff;
        if (IsObscuredOrOffscreen(currPixelu, lineDepth, screenSize, distThreshold, depthDiff))
        {
            DebugPixel(startPixel, currPixelu, float4(1, 0, 0, 1));
            foundHit = true;
            break;
        }

        //if (currDepth < lineDepth + distThreshold)
        //{
        //    foundHit = true;
        //    DebugPixel(startPixel, currPixelu + int2(1,0), float4(1, 0, 0, 1));
        //    DebugPixel(startPixel, currPixelu - int2(1,0), float4(1, 0, 0, 1));
        //    DebugPixel(startPixel, currPixelu - int2(0,1), float4(1, 0, 0, 1));
        //    DebugPixel(startPixel, currPixelu + int2(0,1), float4(1, 0, 0, 1));
        //    DebugPixel(startPixel, currPixelu + int2(0,2), float4(1, 0, 0, 1));
        //    DebugPixel(startPixel, currPixelu + int2(2,0), float4(1, 0, 0, 1));
        //    break;
        //}

        lastDepth = currDepth;
        length0 = length1;
        pixel0 = pixel1;
//        lastPixel = currPixelu;
    }
    // Phase 2: refinement (binary search)
    if (!foundHit)
    {
        return invalid;
    }
    float depthDiff = 0;
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
//        currDepth = sceneDepth.Load(int3(nextPixelu, 0)).x;
        DebugPixel(startPixel, nextPixelu, float4(0,1,0,1));
        if (IsObscuredOrOffscreen(nextPixelu, lineDepth, screenSize, distThreshold, depthDiff)) // behind geometry
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

    if (abs(depthDiff) < 0.01)
    {
        DebugPixel(startPixel, round(pixel1), float4(0, 0, 1, 1));

        ScreenLocation result;
        result.bufferDepth = currDepth;
        result.pos = round(pixel1);
        return result;
        
    }
    return invalid;

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

    float3 reflectionStrength = ReflectionStrength(li.normal, li.colour, li.roughness, li.viewDir, li.metalness, li.specularStrength);
    reflectionStrength = clamp(reflectionStrength, 0, 10);
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
        float3 hitNormal = UnpackNormalFromUnorm(sceneNormalMetal.Load(int3(result.pos, 0)).xyz);
        float visibility = dot(hitNormal, -reflectionDir);// / 2;
        DebugPixel(pixel, result.pos, float4(1, 0, 1, 1));

        if (visibility > 0)
       {
            float4 resultPos = mul(screen2World, float4(float2(result.pos) / screenSize, result.bufferDepth, 1));
            float distance = length(resultPos.xyz / resultPos.w - li.worldPos);
            float falloff = 1 / clamp(distance, 1, 1000);
            DebugPixel(pixel, pixel, float4(1, 1, 1, 1));
        #if COMPUTE
            return visibility * reflectionStrength * inoutSceneColour[result.pos];
        #else
            return visibility * reflectionStrength * //falloff*
            inSceneColour.Load(int3(result.pos, 0)).xyz;
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
    return inSceneColour.Load(int3(pixel.xy, 0)) + float4(CalcSSR(pixel.xy), 0);
}