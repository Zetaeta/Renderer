
#ifndef MATERIAL_HLSLI
#define MATERIAL_HLSLI

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
    float3 viewDir;
};

#define pdot(a,b) saturate(dot(a,b));

static const float PI = 3.14159265f;
static const float dielectricF0 = 0.04;

#define BLINN
float3 ComputeLighting_BlinnPhong(float3 normal, float3 diffuseCol, float3 lightCol, float3 lightDir, float roughness, float3 viewDir, float metalness)
{
	float alpha = roughness * roughness;
	float alphasq = max(alpha * alpha,0.001);
	float specularExp = 2/alphasq - 2;
	float diffInt = saturate(dot(-lightDir, normal)) * ambdiffspec.y;
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
	float specInt = (1-roughness)*(specularExp+8)/(8*PI) * pow(f, specularExp) * (specularExp > 0);// * ambdiffspec.z; 
	//return diffuseCol * (lightCol * diffInt) + lightCol * specInt;
	return diffuseCol * (lightCol * diffInt) + lightCol * specInt;
}

float3 ComputeLighting_GGX(float3 normal, float3 diffuseCol, float3 lightCol, float3 lightDir, float roughness, float3 viewDir, float metalness)
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
	float3 Fresnel = specularCol + (1-specularCol) * pow(1-NL, 5);
	float NDF = (NH > 0) * alphasq / (PI * pow(NH * NH * (alphasq - 1) + 1, 2));
	float k = alpha/2;

	float G11 = NV / max((NV * (1-k) + k), 0.0001);
	float G12 = NL / max((NL * (1-k) + k), 0.0001);
	float GeomShadowing = G11 * G12;

	float3 specularPart = lightCol * (Fresnel * GeomShadowing * NDF) / max((4 * NL * NV),0.0001) ;
	
	float diffInt =  ambdiffspec.y;
	float3 diffusePart = (1-metalness) * diffuseCol * lightCol * diffInt;

    return (diffusePart + specularPart) * NL;
}

float3 ComputeLighting(float3 normal, float3 diffuseCol, float3 lightCol, float3 lightDir, float roughness, float3 viewDir, float metalness)
{
    float3 result;
	if (brdf == 1)
	{
        result = ComputeLighting_GGX(normal, diffuseCol, lightCol, lightDir, roughness, viewDir, metalness);
    }
    else
    {
        result = ComputeLighting_BlinnPhong(normal, diffuseCol, lightCol, lightDir, roughness, viewDir, metalness);
    }
    return result;
}

#endif
