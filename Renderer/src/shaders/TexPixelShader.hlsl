

//#define POINT_LIGHT
//#define DIR_LIGHT
//#define SPOTLIGHT

#ifndef	TEXTURED
#define TEXTURED 1
#endif

#ifndef USE_STENCIL
#define USE_STENCIL 0
#endif

#define DEBUG_MODE_ALBEDO 1
#define DEBUG_MODE_NORMAL 2
#define DEBUG_MODE_ROUGHNESS 3
#define DEBUG_MODE_METALNESS 4
#define DEBUG_MODE_DEPTH 5

#ifndef BASE_LAYER
#define BASE_LAYER 1
#endif

cbuffer PerFramePSData : register(b0) {
#if BASE_LAYER
	float3 ambient;
#endif
#ifdef DIR_LIGHT
#define USE_NORMAL 1
#define SHADOWMAP_2D
	float3 dirLightCol;
	float3 dirLightDir;
	Matrix world2Light;
#endif

#ifdef POINT_LIGHT
#define USE_NORMAL 1
	float3 pointLightPos;
	float pointLightRad;
	float3 pointLightCol;
	float pointLightFalloff;
	Matrix world2Light;
#endif

#ifdef SPOTLIGHT
#define USE_NORMAL 1
#define SHADOWMAP_2D
	float3 spotLightPos;
	float spotLightTan;
	float3 spotLightDir;
	float spotLightFalloff;
	float3 spotLightCol;
	Matrix world2Light;
#endif
	int debugMode;
	float debugGrayscaleExp;
	int brdf = 0;
};
#ifndef USE_NORMAL
#define USE_NORMAL 0
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

#ifdef POINT_LIGHT
#if !USE_NORMAL
#error Ohno
#endif
TextureCube pointShadowMap;
#endif

SamplerState splr : register(s0);
SamplerComparisonState shadowSampler : register(s1);

float square(float f)
{
	return f*f;
}
float squareLen(float3 v)
{
	return dot(v,v);
}

#include "Material.hlsli"

//template<class Vec>
//float pdot(Vec a, Vec b)
//{
//	return saturate(dot(a,b));
//}

float4 Greyscale(float value)
{
	value = pow(value, debugGrayscaleExp);
	return float4(value, value, value, value);
}
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
    result.normalMetal = float4(li.normal, li.metalness);
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
    float3 normal : Normal;
    float3 tangent : Tangent;
    float3 viewDir : ViewDir;
    float3 worldPos : WorldPos;
};

PixelLightingInput GetLightingInput(BasePassPSIn interp)
{
    PixelLightingInput result;
	float4 colour = matColour;
#if TEXTURED
	colour = diffuse.Sample(splr, interp.uv);
#endif
	{
    #if (MASKED || TRANSLUCENT)
        float alpha = texColour.w;
        if (alphaMask < 1.f && alpha < alphaMask)
        {
            discard;
        }
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
    result.roughness = roughness;
    result.metalness = matMetalness;
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
		float3 fullRough = roughnessMap.Sample(splr, interp.uv.xy);
		result.roughness += fullRough.g;
		result.metalness += fullRough.r;
	}
#endif
#endif
    return result;
}

struct ShadingOut
{
    float4 colour : SV_Target;
};



float4 CalcColour(BasePassPSIn interp)
{
    PixelLightingInput li = GetLightingInput(interp);
	float4 texColour = matColour;
#if TEXTURED
	texColour = diffuse.Sample(splr, interp.uv);
#endif
	float alpha = texColour.w;
#if (MASKED || TRANSLUCENT)
	if (alphaMask < 1.f && alpha < alphaMask)
	{
		discard;
	}
#endif
	float3 colour = float3(0,0,0);
#if BASE_LAYER
	if (debugMode == DEBUG_MODE_ALBEDO)
	{
        return float4(li.colour, 1);
    }
	colour += li.emissive;
	colour += (ambient * ambdiffspec.x) * li.colour;
#endif
#if USE_NORMAL
	if (debugMode == DEBUG_MODE_ALBEDO)
	{
		return float4(0,0,0,0);
	}
	float3 normal = li.normal;
	float rough = li.roughness;
	float metalness = li.metalness;
	float3 viewDir = normalize(interp.viewDir);
#endif // USE_NORMAL
	if (debugMode == DEBUG_MODE_ROUGHNESS)
	{
		#ifdef DIR_LIGHT
		return Greyscale(rough);
		#else
		return float4(0,0,0,1);
		#endif
	}
	if (debugMode == DEBUG_MODE_METALNESS)
	{
		#ifdef DIR_LIGHT
		return Greyscale(metalness);
		#else
		return float4(0,0,0,1);
		#endif
	}
	if (debugMode == DEBUG_MODE_NORMAL)
	{
		#ifdef DIR_LIGHT
		return float4((normal + 1)/2, 1);
		#else
		return float4(0,0,0,0);
		#endif
	}

	float unshadowed = 0.;
	const float bias = 0.0005;
    float3 shadeSpacePos = interp.worldPos;
#ifdef SHADOWMAP_2D
	{
		float4 lightPos = mul(world2Light, float4(shadeSpacePos, 1));
		float2 lightCoord = float2((lightPos.x / lightPos.w + 1)/2, (1 - lightPos.y / lightPos.w)/2);
		float shadowSpl = spotShadowMap.Sample(splr, lightCoord);
		//return shadowSpl + 0.1 * float4(ComputeLighting(normal, texColour * ambdiffspec.y, dirLightCol, dirLightDir, specularExp, viewDir),1);;
		if (lightCoord.x <= 1 && lightCoord.x >= 0 && lightCoord.y <= 1 && lightCoord.y >= 0) {
			float depth = lightPos.z / lightPos.w;
//			shadowed = !( shadowSpl + bias >= depth);
			unshadowed = spotShadowMap.SampleCmp(shadowSampler, lightCoord, depth - bias );
		}
		if (debugMode == 1)
		{
        	return float4(shadowSpl, shadowSpl, shadowSpl, 1);
        }
		else if (debugMode == 2)
		{
			return float4(lightCoord, lightPos.w, 1);
		}
		else if (debugMode == 3)
		{
			return float4(shadeSpacePos, 1);
		}

//		return mul(world2Light, float4(1,1,1,1))* 100;
//		return lightPos;
//		return float4(shadowSpl, shadowSpl, lightPos.z / lightPos.w, 1);
	}
#endif

#ifdef POINT_LIGHT
	{

		float3 lightDist = -pointLightPos + shadeSpacePos;
		//float3 lightPos = shadeSpacePos - pointLightPos
		float3 lightDir = normalize(lightDist);
		float d2 = dot(lightDist, lightDist) / square(pointLightFalloff) ;
		float r = pointLightRad;
		float3 lightCol = pointLightCol / (d2 + r*r);

		float4 dummy = 0.001 * float4(ComputeLighting(normal, texColour * ambdiffspec.y, lightCol, lightDir, rough, viewDir, metalness),1);

		float4 lightPos = mul(world2Light, float4(shadeSpacePos, 1));
		float depth = length(lightDist.xyz) / 100;
		unshadowed = pointShadowMap.SampleCmp(shadowSampler, lightDist, depth - bias );


		colour += unshadowed * ComputeLighting(normal, texColour * ambdiffspec.y, lightCol, lightDir, rough, viewDir, metalness);
	}
#endif

#ifdef DIR_LIGHT
	{
	//	unshadowed = 1 - 0.01*unshadowed;
		colour += (unshadowed) * ComputeLighting(normal, texColour * ambdiffspec.y, dirLightCol, dirLightDir, rough, viewDir, metalness);
	}
#endif
#ifdef SPOTLIGHT
	{
		float3 lightDist = shadeSpacePos - spotLightPos;
		float3 lightDir = normalize(lightDist);
		float d2 = dot(lightDist, lightDist) / square(spotLightFalloff) ;
		float3 lightCol = spotLightCol / (d2);

		float3 straightDist = dot(lightDist, spotLightDir) * spotLightDir;
		float tangleSq = squareLen(lightDist - straightDist) / squareLen(straightDist);

		colour += (unshadowed) * ((tangleSq < square(spotLightTan) && (dot(spotLightDir, lightDir) > 0))) * ComputeLighting(normal, texColour , lightCol, lightDir, rough, viewDir, metalness);
	}
#endif

	return float4(colour, alpha);
}

#if TEXTURED
#define IF_TEXTURED(x) x
#else
#define IF_TEXTURED(x)
#endif

GBufferOut GBufferPSMain(BasePassPSIn interp)
{
    PixelLightingInput lightingInput = GetLightingInput(interp);
    return PackGBuffer(lightingInput);
}

PSOut main(BasePassPSIn interp)
{
//	if (debugMode < 0)
//	{
//		return float4(1,0,1,1);
//	}
	PSOut result;
	result.colour = CalcColour(interp);
#if USE_STENCIL
	result.stencil = stencilVal;
#endif
	return result;
}