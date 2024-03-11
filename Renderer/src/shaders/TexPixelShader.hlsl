

//#define POINT_LIGHT
//#define DIR_LIGHT
//#define SPOTLIGHT

#ifndef	TEXTURED
#define TEXTURED 1
#endif

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
};

cbuffer PerInstancePSData : register(b1) {
	float3 matColour;
	float3 ambdiffspec;
	int specularExp;
#if TEXTURED
	bool useNormalMap;
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

SamplerState splr;

float square(float f)
{
	return f*f;
}
float squareLen(float3 v)
{
	return dot(v,v);
}

static const float PI = 3.14159265f;

#define BLINN
float3 ComputeLighting(float3 normal, float3 diffuseCol, float3 lightCol, float3 lightDir, float roughness, float3 viewDir)
{
	float alpha = roughness * roughness;
	float alphasq = max(alpha * alpha,0.001);
	float specularExp = 2/alphasq - 2;
	float diffInt = lightCol * saturate(dot(-lightDir, normal)) * ambdiffspec.y;
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
	float specInt = (specularExp + 2)/(4*PI * (2-exp(1-1/alphasq))) * pow(f, specularExp) * (specularExp > 0);// * ambdiffspec.z; 
	//return diffuseCol * (lightCol * diffInt) + lightCol * specInt;
	return diffuseCol * (lightCol * diffInt) + lightCol * specInt;
}

float4 main(
	#if TEXTURED
	float2 uv: TexCoord,
	#endif
	float3 normal: Normal, float3 tangent : Tangent, float3 viewDir: ViewDir, float3 shadeSpacePos : Colour//, float4 lightPos: LightPos
	) : SV_TARGET
{
	float4 texColour = float4(matColour, 1);
	#if TEXTURED
	texColour = diffuse.Sample(splr, float2(uv.x, uv.y));
	#endif
	float3 colour = float3(0,0,0);
#ifdef BASE_LAYER
#if TEXTURED
	colour += emissiveMap.Sample(splr, float2(uv.x, uv.y));
#endif
	colour += (ambient * ambdiffspec.x) * texColour;
#else // BASE_LAYER
	normal = normalize(normal);
	float roughness = 1;
#if TEXTURED
	if (useNormalMap)
	{
		float3 texNorm = 2 * normalMap.Sample(splr, float2(uv.x, uv.y)) - float3(1,1,1);
		float3 bitangent = cross(tangent, normal);
		float3 newnormal = texNorm.x * tangent + texNorm.y * bitangent +
		texNorm.z * normal;
		normal = normalize(newnormal);
	}
	float3 fullRough = roughnessMap.Sample(splr, uv);
	roughness = fullRough.g;
	
#endif // TEXTURED
	viewDir = normalize(viewDir);
#endif // !BASE_LAYER



	bool shadowed = false;
	const float bias = 0.0001;
#ifdef SHADOWMAP_2D
	{

		float4 lightPos = mul(world2Light, float4(shadeSpacePos, 1));
		float2 lightCoord = float2((lightPos.x / lightPos.w + 1)/2, (1 - lightPos.y / lightPos.w)/2);
		float shadowSpl = spotShadowMap.Sample(splr, lightCoord);
		//return shadowSpl + 0.1 * float4(ComputeLighting(normal, texColour * ambdiffspec.y, dirLightCol, dirLightDir, specularExp, viewDir),1);;
		if (lightCoord.x <= 1 && lightCoord.x >= 0 && lightCoord.y <= 1 && lightCoord.y >= 0) {
			shadowed = !( shadowSpl + bias >= lightPos.z / lightPos.w);
		}
//		return mul(world2Light, float4(1,1,1,1))* 100;
//		return lightPos;
//		return float4(shadowSpl, shadowSpl, lightPos.z / lightPos.w, 1);
	//	return float4(shadowSpl, shadowSpl, shadowSpl, 1);
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

		float4 dummy = 0.001 * float4(ComputeLighting(normal, texColour * ambdiffspec.y, lightCol, lightDir, roughness, viewDir),1);

//		return float4(shadeSpacePos, 1);
		float4 lightPos = mul(world2Light, float4(shadeSpacePos, 1));
		//return world2Light[3] + dummy;
		//return float4(shadeSpacePos, 1) ;
		//return lightPos;
		//return lightPos + 0.01 * float4(ComputeLighting(normal, texColour * ambdiffspec.y, lightCol, lightDir, specularExp, viewDir),1);
		//return float4(lightDist + 0.01 * ComputeLighting(normal, texColour * ambdiffspec.y, lightCol, lightDir, specularExp, viewDir),1);
		float shadowSpl = pointShadowMap.Sample(splr, lightDist);
		float depth = length(lightDist.xyz) / 100;
		//shadowSpl = abs(-shadowSpl + depth) > 0.005;
	//	shadowSpl = depth;
//shadowSpl = pow(shadowSpl, 1);
		//return float4(shadowSpl, shadowSpl, shadowSpl,1)/1 + 0.001 * float4(ComputeLighting(normal, texColour * ambdiffspec.y, lightCol, lightDir, specularExp, viewDir),1);
		shadowed = shadowSpl + bias < depth;


		//colour += float4(fullRough,1) + 0.001* (!shadowed) * ComputeLighting(normal, texColour * ambdiffspec.y, lightCol, lightDir, roughness, viewDir);
		colour += (!shadowed) * ComputeLighting(normal, texColour * ambdiffspec.y, lightCol, lightDir, roughness, viewDir);
	}
#endif

#ifdef DIR_LIGHT
	{
		colour += (!shadowed) * ComputeLighting(normal, texColour * ambdiffspec.y, dirLightCol, dirLightDir, roughness, viewDir);
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

		colour += (!shadowed) * ((tangleSq < square(spotLightTan) && (dot(spotLightDir, lightDir) > 0))) * ComputeLighting(normal, texColour , lightCol, lightDir, roughness, viewDir);
	}
#endif
//	float3 lightDir = dirLightDir;
//	float3 lightCol = dirLightCol;
//	float diffInt = lightCol * saturate(dot(-lightDir, normal)) * ambdiffspec.y;
//	float3 outRay = normalize(lightDir - 2 * dot(lightDir, normal) * normal);
//	float f = max(dot(normalize(viewDir),-outRay), 0.00001f);
//	float specInt = pow(f, specularExp) * (specularExp > 0);// * ambdiffspec.z; 
//	float x = specularExp / 500.0;//specularExp  > 0 ? 1 : 0;


	return float4(colour, 1);

	//return float4(texColour * (ambient * ambdiffspec.x + lightCol * diffInt) + lightCol * specInt + shadeSpacePos * 0.f + emission, 1.0f);
}