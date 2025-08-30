

//#define POINT_LIGHT
//#define DIR_LIGHT
//#define SPOTLIGHT

#ifndef	TEXTURED
#define TEXTURED 1
#endif

#ifndef USE_STENCIL
#define USE_STENCIL 0
#endif

#ifndef POINT_LIGHT
#define POINT_LIGHT 0
#endif

#ifndef DIR_LIGHT
#define DIR_LIGHT 0
#endif

#ifndef SPOTLIGHT
#define SPOTLIGHT 0
#endif

#ifndef GBUFFER
#define GBUFFER 0
#endif

#define DEBUG_MODE_ALBEDO 1
#define DEBUG_MODE_NORMAL 2
#define DEBUG_MODE_ROUGHNESS 3
#define DEBUG_MODE_METALNESS 4
#define DEBUG_MODE_DEPTH 5

#ifndef BASE_LAYER
#define BASE_LAYER 1
#endif

#ifndef FW_DEBUGGING
#define FW_DEBUGGING !GBUFFER
#endif

#include "PerLightShading.hlsli"
#ifndef USE_NORMAL
#define USE_NORMAL GBUFFER
#endif

cbuffer PerInstancePSData : register(b1) {
	float4 matColour;
	float3 ambdiffspec;
	float roughness;
	float3 emissiveColour;
	float alphaMask;
	float matMetalness;
#if TEXTURED
	bool useNormalMap;
	bool useEmissiveMap;
	bool useRoughnessMap;
#endif
#if USE_STENCIL
	uint stencilVal;
#endif
};
#define EMISSIVE BASE_LAYER

#if TEXTURED
Texture2D diffuse;// : register(t0);
#if BASE_LAYER
Texture2D emissiveMap;// : register(t2);
#endif
#if USE_NORMAL
Texture2D normalMap;// : register(t1);
Texture2D roughnessMap;
#endif
#endif

#ifdef SHADOWMAP_2D
Texture2D spotShadowMap;
#endif

#if POINT_LIGHT
#if !USE_NORMAL
#error Ohno
#endif
TextureCube pointShadowMap;
#endif

SamplerState splr : register(s0);
SamplerComparisonState shadowSampler : register(s1);

float4 Greyscale(float value)
{
    value = pow(abs(value), debugGrayscaleExp);
	return float4(value, value, value, value);
}
#include "Material.hlsli"

//template<class Vec>
//float pdot(Vec a, Vec b)
//{
//	return saturate(dot(a,b));
//}

struct PSOut
{
	float4 colour : SV_TARGET;
#if USE_STENCIL
	uint stencil : SV_StencilRef;
#endif
};

struct GBufferOut
{
    float4 colourRough : SV_Target0;
    float4 normalMetal : SV_Target1;
    float4 emissive : SV_Target2;
};

GBufferOut PackGBuffer(PixelLightingInput li)
{
    GBufferOut result;
    result.colourRough = float4(li.colour, li.roughness);
    result.normalMetal = float4(PackNormalToUnorm(li.normal), li.metalness);
#if BASE_LAYER
    result.emissive = float4(li.emissive, 1);
#else
    result.emissive = 0;
#endif
    return result;
}

struct BasePassPSIn
{
#if TEXTURED
    float2 uv : TexCoord;
#endif
#if VERTEX_COLOUR
    float4 vertexColour : Colour;
    float4 GetVertexColour()
    {
        return vertexColour;
    }
#else
    float4 GetVertexColour()
    {
        return 0;
    }
#endif
    float3 normal : Normal;
    float3 tangent : Tangent;
    float3 viewDir : ViewDir;
    float3 worldPos : WorldPos;
};

PixelLightingInput GetLightingInput(BasePassPSIn interp)
{
    PixelLightingInput result;
    result.ambientStrength = ambdiffspec.x;
    result.diffuseStrength = ambdiffspec.y;
    result.specularStrength = ambdiffspec.z;
#if VERTEX_COLOUR
    float4 colour = interp.GetVertexColour();
#else
	float4 colour = matColour;
#endif
#if TEXTURED
	colour = diffuse.Sample(splr, interp.uv);
#endif
	{
#if (MASKED || TRANSLUCENT)
    float alpha = colour.w;
    //if (alphaMask < 1.f && alpha < alphaMask)
    //{
    //    discard;
    //}
    #endif
    #if TRANSLUCENT
        result.opacity = alpha;
    #endif
    }
    result.colour = colour.xyz;
#if EMISSIVE
    result.emissive = emissiveColour;
#if TEXTURED
	if (useEmissiveMap)
	{
		result.emissive = emissiveMap.Sample(splr, interp.uv.xy).xyz;
	}
#endif
#endif

#if USE_NORMAL
	result.normal = normalize(interp.normal);
    result.roughness = roughness + roughnessMod;
    result.metalness = matMetalness + metalnessMod;
#if TEXTURED
	if (useNormalMap)
	{
		float3 texNorm = 2 * normalMap.Sample(splr, interp.uv.xy).xyz - float3(1,1,1);
        float3 tangent = normalize(interp.tangent);
		float3 bitangent = cross(tangent, result.normal);
		float3 newnormal = texNorm.x * tangent + texNorm.y * bitangent + texNorm.z * result.normal;
		result.normal = normalize(newnormal);
	}
	if (useRoughnessMap)
	{
		float3 fullRough = roughnessMap.Sample(splr, interp.uv.xy).xyz;
		result.roughness += fullRough.g;
		result.metalness += fullRough.r;
	}
    result.roughness = clamp(result.roughness, 0.08, 1);
    result.metalness = clamp(result.metalness, 0, 1);
#endif
#endif
    result.viewDir = normalize(interp.viewDir);
    result.worldPos = interp.worldPos;
    return result;
}

struct ShadingOut
{
    float4 colour : SV_Target;
};




#if TEXTURED
#define IF_TEXTURED(x) x
#else
#define IF_TEXTURED(x)
#endif

#if GBUFFER
GBufferOut main(BasePassPSIn interp)
{
    PixelLightingInput lightingInput = GetLightingInput(interp);
#if MASKED || TRANSLUCENT
    if (alphaMask < 1.f && lightingInput.opacity < alphaMask)
    {
        discard;
    }
#endif
    return PackGBuffer(lightingInput);
}
#else

PSOut main(BasePassPSIn interp)
{
//	if (debugMode < 0)
//	{
//		return float4(1,0,1,1);
//	}
	PSOut result;
    result.colour = DoShading(GetLightingInput(interp));
#if USE_STENCIL
	result.stencil = stencilVal;
#endif
	return result;
}
#endif