

//#define POINT_LIGHT
//#define DIR_LIGHT
//#define SPOTLIGHT

#ifndef	TEXTURED
#define TEXTURED 1
#endif

#define DEBUG_MODE_ALBEDO 1
#define DEBUG_MODE_NORMAL 2
#define DEBUG_MODE_ROUGHNESS 3
#define DEBUG_MODE_METALNESS 4
#define DEBUG_MODE_DEPTH 5


cbuffer PerFramePSData : register(b0) {
#ifdef BASE_LAYER
	float3 ambient;
#endif
#ifdef DIR_LIGHT
#define USE_NORMAL
#define SHADOWMAP_2D
	float3 dirLightCol;
	float3 dirLightDir;
	Matrix world2Light;
#endif

#ifdef POINT_LIGHT
#define USE_NORMAL
	float3 pointLightPos;
	float pointLightRad;
	float3 pointLightCol;
	float pointLightFalloff;
	Matrix world2Light;
#endif

#ifdef SPOTLIGHT
#define USE_NORMAL
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
};

#if TEXTURED
Texture2D diffuse;// : register(t0);
#ifndef BASE_LAYER
Texture2D normalMap;// : register(t1);
Texture2D roughnessMap;
#else
Texture2D emissiveMap;// : register(t2);
#endif
#endif

#ifdef SHADOWMAP_2D
Texture2D spotShadowMap;
#endif

#ifdef POINT_LIGHT
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

//template<class Vec>
//float pdot(Vec a, Vec b)
//{
//	return saturate(dot(a,b));
//}

#define pdot(a,b) saturate(dot(a,b));

static const float PI = 3.14159265f;
static const float dielectricF0 = 0.04;

float4 Greyscale(float value)
{
	value = pow(value, debugGrayscaleExp);
	return float4(value, value, value, value);
}

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
	//return float3(1-NL, 1-NL, 1-NL) + 0.01 * Fresnel;
	//return Fresnel;
	float NDF = (NH > 0) * alphasq / (PI * pow(NH * NH * (alphasq - 1) + 1, 2));
	//return float3(NDF, NDF, NDF);
	//return Fresnel * NDF * lightCol;
	float k = alpha/2;

	float G11 = NV / max((NV * (1-k) + k), 0.0001);
	float G12 = NL / max((NL * (1-k) + k), 0.0001);
	float GeomShadowing = G11 * G12;
//	return float3(GeomShadowing, GeomShadowing, GeomShadowing);

	float3 specularPart = lightCol * (Fresnel * GeomShadowing * NDF) / max((4 * NL * NV),0.0001) ;
//	return specularPart;
	
	float diffInt =  ambdiffspec.y;
	float3 diffusePart = (1-metalness) * diffuseCol * lightCol * diffInt;

    return (diffusePart + specularPart) * NL;
	
}

float3 ComputeLighting(float3 normal, float3 diffuseCol, float3 lightCol, float3 lightDir, float roughness, float3 viewDir, float metalness)
{
	if (brdf == 1)
	{
        return ComputeLighting_GGX(normal, diffuseCol, lightCol, lightDir, roughness, viewDir, metalness);
    }
	return ComputeLighting_BlinnPhong(normal, diffuseCol, lightCol, lightDir, roughness, viewDir, metalness);
}

float4 main(
	#if TEXTURED
	float2 uv: TexCoord,
	#endif
	float3 normal: Normal, float3 tangent : Tangent, float3 viewDir: ViewDir, float3 shadeSpacePos : Colour//, float4 lightPos: LightPos
	) : SV_TARGET
{
//	if (debugMode < 0)
//	{
//		return float4(1,0,1,1);
//	}
	float4 texColour = matColour;
	#if TEXTURED
	texColour = diffuse.Sample(splr, float2(uv.x, uv.y));
	#endif
	float alpha = texColour.w;
	if (alphaMask < 1.f && alpha < alphaMask)
	{
		discard;
	}
	float3 colour = float3(0,0,0);
	float3 emiss = emissiveColour;
#ifdef BASE_LAYER
	if (debugMode == DEBUG_MODE_ALBEDO)
	{
		return texColour;
	}
#if TEXTURED
	if (useEmissiveMap)
	{
		emiss = emissiveMap.Sample(splr, float2(uv.x, uv.y)).xyz;
	}
	colour += emiss;
#endif
	colour += (ambient * ambdiffspec.x) * texColour;
#else // BASE_LAYER
	if (debugMode == DEBUG_MODE_ALBEDO)
	{
		return float4(0,0,0,0);
	}
	normal = normalize(normal);
	float rough = roughness;
	float metalness = matMetalness;
#if TEXTURED
	if (useNormalMap)
	{
		float3 texNorm = 2 * normalMap.Sample(splr, float2(uv.x, uv.y)) - float3(1,1,1);
		float3 bitangent = cross(tangent, normal);
		float3 newnormal = texNorm.x * tangent + texNorm.y * bitangent +
		texNorm.z * normal;
		normal = normalize(newnormal);
	}
	if (useRoughnessMap)
	{
		float3 fullRough = roughnessMap.Sample(splr, uv);
		rough = rough + fullRough.g;
		metalness += fullRough.r;
	}
	
#endif // TEXTURED
	viewDir = normalize(viewDir);
#endif // !BASE_LAYER
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