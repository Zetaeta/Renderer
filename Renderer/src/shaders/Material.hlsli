
#ifndef MATERIAL_HLSLI
#define MATERIAL_HLSLI

#include "DeferredShadingCommon.hlsli"
#include "Maths.hlsli"

#define BLINN
float3 ComputeLighting_BlinnPhong(float3 normal, float3 diffuseCol, float3 lightCol, float3 lightDir, float roughness, float3 viewDir, float metalness, float diffuseStrength, float specularStrength)
{
	float alpha = roughness * roughness;
	float alphasq = max(alpha * alpha,0.001);
	float specularExp = 2/alphasq - 2;
	float diffInt = saturate(dot(-lightDir, normal)) * diffuseStrength;
#ifdef BLINN
	float f = max(dot(normalize(-viewDir - lightDir),normal), 0.00001f);
	if (f > 1)
	{
		return float3(0,1,0);
	}
#else
	float3 outRay = normalize(lightDir - 2 * dot(lightDir, normal) * normal);
	float f = max(dot(viewDir,-outRay), 0.00001f);
#endif
	float specInt = (1-roughness)*(specularExp+8)/(8*PI) * pow(f, specularExp) * (specularExp > 0) * specularStrength;
	//return diffuseCol * (lightCol * diffInt) + lightCol * specInt;
	return diffuseCol * (lightCol * diffInt) + lightCol * specInt;
}

float3 ComputeLighting_GGX(float3 normal, float3 diffuseCol, float3 lightCol, float3 lightDir, float roughness, float3 viewDir, float metalness, float diffuseStrength, float specularStrength)
{
	viewDir = -viewDir;
	lightDir = -lightDir;
	roughness = max(roughness, 0.01);
	float3 halfVec = normalize(lightDir + viewDir);
	float alpha = roughness * roughness;
	float alphasq = alpha * alpha;
	float NH = pdot(normal, halfVec);
	float NV = pdot(normal, viewDir);
    NV	= max (NV, 0.00005);
	float NL = pdot(normal, lightDir);

	float3 specularCol = metalness * diffuseCol + (1-metalness) * float3(dielectricF0,dielectricF0,dielectricF0);
    float fresnelStrength = pow(1 - NL, 5);
	float3 Fresnel = specularCol + (1-specularCol) * fresnelStrength;
	float NDF = (NH > 0) * alphasq / (PI * pow(NH * NH * (alphasq - 1) + 1, 2));
	float k = alpha/2;

	float G11 = NV / max((NV * (1-k) + k), 0.0001);
	float G12 = NL / max((NL * (1-k) + k), 0.0001);
	float GeomShadowing = G11 * G12;

	float3 specularPart = lightCol * (Fresnel * GeomShadowing * NDF) / max((4 * NL * NV),0.0001) * specularStrength;
	
    float3 diffusePart = (1 - metalness) * (1 - fresnelStrength) * diffuseCol * lightCol * diffuseStrength;

    return (diffusePart + specularPart) * NL;
}

float3 ComputeLighting(float3 normal, float3 diffuseCol, float3 lightCol, float3 lightDir, float roughness, float3 viewDir, float metalness, float diffuseStrength, float specularStrength)
{
    float3 result;
	if (brdf == 1)
	{
        result = ComputeLighting_GGX(normal, diffuseCol, lightCol, lightDir, roughness, viewDir, metalness, diffuseStrength, specularStrength);
    }
    else
    {
        result = ComputeLighting_BlinnPhong(normal, diffuseCol, lightCol, lightDir, roughness, viewDir, metalness, diffuseStrength, specularStrength);
    }
    return result;
}

float4 DoShading(PixelLightingInput li)
{
	float3 texColour = li.colour;
	float3 colour = float3(0,0,0);
    float alpha = 1;

#if TRANSLUCENT || MASKED
	alpha = li.opacity;
    if (alphaMask < 1.f && alpha < alphaMask)
    {
        discard;
    }
#endif

#if BASE_LAYER
#if FW_DEBUGGING
	if (debugMode == DEBUG_MODE_ALBEDO)
	{
        return float4(li.colour, 1);
    }
#endif
	colour += li.emissive;
	colour += (ambient * li.ambientStrength) * li.colour;
#endif

#if USE_NORMAL
#if FW_DEBUGGING
	if (debugMode == DEBUG_MODE_ALBEDO)
	{
		return float4(0,0,0,0);
	}
#endif
	float3 normal = li.normal;
	float rough = li.roughness;
	float metalness = li.metalness;
	float3 viewDir = li.viewDir;
#endif // USE_NORMAL
#if FW_DEBUGGING
	if (debugMode == DEBUG_MODE_ROUGHNESS)
	{
		#if DIR_LIGHT
		return Greyscale(rough);
		#else
		return float4(0,0,0,1);
		#endif
	}
	if (debugMode == DEBUG_MODE_METALNESS)
	{
		#if DIR_LIGHT
		return Greyscale(metalness);
		#else
		return float4(0,0,0,1);
		#endif
	}
	if (debugMode == DEBUG_MODE_NORMAL)
	{
		#if DIR_LIGHT
		return float4((normal + 1)/2, 1);
		#else
		return float4(0,0,0,0);
		#endif
	}
#endif

	float unshadowed = 0.;
	const float bias = 0.0005;
    float3 worldPos = li.worldPos;
#ifdef SHADOWMAP_2D
	{
		float4 lightPos = mul(world2Light, float4(worldPos, 1));
		float2 lightCoord = float2((lightPos.x / lightPos.w + 1)/2, (1 - lightPos.y / lightPos.w)/2);
		float shadowSpl = spotShadowMap.Sample(splr, lightCoord).x;
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
			return float4(worldPos, 1);
		}

//		return mul(world2Light, float4(1,1,1,1))* 100;
//		return lightPos;
//		return float4(shadowSpl, shadowSpl, lightPos.z / lightPos.w, 1);
	}
#endif
 
#if POINT_LIGHT
	{

		float3 lightDist = -pointLightPos + worldPos;
		//float3 lightPos = worldPos - pointLightPos
		float3 lightDir = normalize(lightDist);
		float d2 = dot(lightDist, lightDist) / square(pointLightFalloff) ;
		float r = pointLightRad;
		float3 lightCol = pointLightCol / (d2 + r*r);

//		float4 dummy = 0.001 * float4(ComputeLighting(normal, (texColour * ambdiffspec.y), lightCol, lightDir, rough, viewDir, metalness),1);

		float4 lightPos = mul(world2Light, float4(worldPos, 1));
		float depth = length(lightDist.xyz) / 100;
		unshadowed = pointShadowMap.SampleCmp(shadowSampler, lightDist, depth - bias );


		colour += unshadowed * ComputeLighting(normal, (texColour * li.diffuseStrength), lightCol, lightDir, rough, viewDir, metalness, li.diffuseStrength, li.specularStrength);
	}
#endif

#if DIR_LIGHT
	{
	//	unshadowed = 1 - 0.01*unshadowed;
		colour += (unshadowed) * ComputeLighting(normal, (texColour * li.diffuseStrength).xyz, dirLightCol, dirLightDir, rough, viewDir, metalness, li.diffuseStrength, li.specularStrength);
	}
#endif
#if SPOTLIGHT
	{
		float3 lightDist = worldPos - spotLightPos;
		float3 lightDir = normalize(lightDist);
		float d2 = dot(lightDist, lightDist) / square(spotLightFalloff) ;
		float3 lightCol = spotLightCol / (d2);

		float3 straightDist = dot(lightDist, spotLightDir) * spotLightDir;
		float tangleSq = squareLen(lightDist - straightDist) / squareLen(straightDist);

		colour += (unshadowed) * ((tangleSq < square(spotLightTan) && (dot(spotLightDir, lightDir) > 0))) *
				ComputeLighting(normal, texColour.xyz, lightCol, lightDir, rough, viewDir, metalness, li.diffuseStrength, li.specularStrength);
	}
#endif

	return float4(colour, alpha);
}

#endif
