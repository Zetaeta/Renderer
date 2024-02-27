
//cbuffer PerFramePSData {
//	float3 ambient;
//	float3 lightCol;
//	float3 lightDir;
//};
//
//cbuffer PerInstancePSData : register(b1) {
//	float3 colour;
//	float3 ambdiffspec;
//	int specularExp;
//};
//
//#define BLINN
//
//float4 main(float3 normal: Normal, float3 viewDir: ViewDir, float3 vertColour : Colour) : SV_TARGET
//{
//	normal = normalize(normal);
//	//return float4(lightDir, 1);
//	float diffInt = lightCol * saturate(dot(-lightDir, normal)) * ambdiffspec.y;
//
//#ifdef BLINN
//	float f = max(dot(normalize(-normalize(viewDir) - lightDir),normal), 0.00001f);
//#else
//	float3 outRay = normalize(lightDir - 2 * dot(lightDir, normal) * normal);
//	float f = max(dot(normalize(viewDir),-outRay), 0.00001f);
//#endif
////return f;
//
//	float specInt = pow(f, specularExp) * (specularExp > 0);// * ambdiffspec.z; 
//	float x = specularExp / 500.0;//specularExp  > 0 ? 1 : 0;
//	return float4(colour * (ambient * ambdiffspec.x + lightCol * diffInt) + lightCol * specInt, 1.0f);
//}

cbuffer PerFramePSData : register(b0) {
#ifdef BASE_LAYER
	float3 ambient;
#endif
#ifdef DIR_LIGHT
#define USE_NORMAL
	float3 dirLightCol;
	float3 dirLightDir;
#endif

#ifdef POINT_LIGHT
#define USE_NORMAL
	float3 pointLightPos;
	float pointLightRad;
	float3 pointLightCol;
	float pointLightFalloff;
#endif

#ifdef SPOTLIGHT
#define USE_NORMAL
	float3 spotLightPos;
	float spotLightTan;
	float3 spotLightDir;
	float spotLightFalloff;
	float3 spotLightCol;
	Matrix world2Light;
#endif
};

cbuffer PerInstancePSData : register(b1) {
	float3 colour;
	float3 ambdiffspec;
	int specularExp;
};

#ifdef SPOTLIGHT
Texture2D spotShadowMap;
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

float3 ComputeLighting(float3 normal, float3 diffuseCol, float3 lightCol, float3 lightDir, float specularExp, float3 viewDir)
{
	
	float diffInt = lightCol * saturate(dot(-lightDir, normal)) * ambdiffspec.y;
	float3 outRay = normalize(lightDir - 2 * dot(lightDir, normal) * normal);
	float f = max(dot(viewDir,-outRay), 0.00001f);
	float specInt = pow(f, specularExp) * (specularExp > 0);// * ambdiffspec.z; 
	return diffuseCol * (lightCol * diffInt) + lightCol * specInt;
}

float4 main(float3 normal: Normal, float3 tangent : Tangent, float3 viewDir: ViewDir, float3 shadeSpacePos : Colour//, float4 lightPos: LightPos
) : SV_TARGET
{
	float3 texColour = colour;
	float3 colour = float3(0,0,0);
#ifdef BASE_LAYER
	//colour += emissiveMap.Sample(splr, float2(uv.x, uv.y));
	colour += (ambient * ambdiffspec.x) * texColour;
#else 
	normal = normalize(normal);
	viewDir = normalize(viewDir);
#endif


#ifdef POINT_LIGHT
	{
		float3 lightDist = -pointLightPos + shadeSpacePos;
		float3 lightDir = normalize(lightDist);
		float d2 = dot(lightDist, lightDist) / square(pointLightFalloff) ;
		float r = pointLightRad;
		float3 lightCol = pointLightCol / (d2 + r*r);
		colour += ComputeLighting(normal, texColour * ambdiffspec.y, lightCol, lightDir, specularExp, viewDir);
	}
#endif
#ifdef DIR_LIGHT
	{
		colour += ComputeLighting(normal, texColour * ambdiffspec.y, dirLightCol, dirLightDir, specularExp, viewDir);
	}
#endif
#ifdef SPOTLIGHT
	{
		float3 lightDist = shadeSpacePos - spotLightPos;
		float3 lightDir = normalize(lightDist);
		float d2 = dot(lightDist, lightDist) / square(spotLightFalloff) ;
		float3 lightCol = spotLightCol / (d2);

		float4 lightPos = mul(world2Light, float4(shadeSpacePos, 1));
		float3 straightDist = dot(lightDist, spotLightDir) * spotLightDir;
		float tangleSq = squareLen(lightDist - straightDist) / squareLen(straightDist);

		float2 lightCoord = float2((lightPos.x / lightPos.w + 1)/2, (1 - lightPos.y / lightPos.w)/2);
		float shadowSpl = spotShadowMap.Sample(splr, lightCoord);
		//return float4((lightPos.x+1)/2, (lightPos.y+1)/2, lightPos.z, 1);
//		return float4((lightPos.x / lightPos.w + 1)/2, (lightPos.y / lightPos.w+1)/2, lightPos.z / lightPos.w, 1)
float bias = 0.001;
		bool shadowed = false;
		if (lightCoord.x <= 1 && lightCoord.x >= 0 && lightCoord.y <= 1 && lightCoord.y >= 0) {
		 //	return float4(shadowSpl, shadowSpl, shadowSpl, 1)
		//		+ float4(colour * 0.01, 0);
			shadowed = !( shadowSpl + bias >= lightPos.z / lightPos.w);
		}

		colour += (!shadowed) * ((tangleSq < square(spotLightTan) && (dot(spotLightDir, lightDir) > 0))) * ComputeLighting(normal, texColour , lightCol, lightDir, specularExp, viewDir);
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