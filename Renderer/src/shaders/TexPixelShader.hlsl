
#define POINT_LIGHT
#define DIR_LIGHT
#define SPOTLIGHT

cbuffer PerFramePSData : register(b0) {
	float3 ambient;
#ifdef POINT_LIGHT
	float3 dirLightCol;
	float3 dirLightDir;
#endif

#ifdef POINT_LIGHT
	float3 pointLightPos;
	float pointLightRad;
	float3 pointLightCol;
	float pointLightFalloff;
#endif

#ifdef SPOTLIGHT
	float3 spotLightPos;
	float spotLightTan;
	float3 spotLightDir;
	float spotLightFalloff;
	float3 spotLightCol;
#endif
};

cbuffer PerInstancePSData : register(b1) {
	float3 colour;
	float3 ambdiffspec;
	int specularExp;
	bool useNormalMap;
};

#ifdef SPOTLIGHT
Texture2D spotShadowMap;
#endif

Texture2D diffuse;// : register(t0);
Texture2D normalMap;
Texture2D emissiveMap;

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

float4 main(float3 normal: Normal, float3 tangent : Tangent, float3 viewDir: ViewDir, float3 shadeSpacePos : Colour, float2 uv: TexCoord, float4 lightPos: LightPos) : SV_TARGET
{
	normal = normalize(normal);
	float4 texColour = diffuse.Sample(splr, float2(uv.x, uv.y));
	if (useNormalMap)
	{
		float3 texNorm = 2 * normalMap.Sample(splr, float2(uv.x, uv.y)) - float3(1,1,1);
		float3 bitangent = cross(tangent, normal);
		float3 newnormal = texNorm.x * tangent + texNorm.y * bitangent +
		texNorm.z * normal;
		normal = newnormal;
	}
	float3 emission = emissiveMap.Sample(splr, float2(uv.x, uv.y));
	float3 colour = emission + (ambient * ambdiffspec.x) * texColour;

	viewDir = normalize(viewDir);

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

		float3 straightDist = dot(lightDist, spotLightDir) * spotLightDir;
		float tangleSq = squareLen(lightDist - straightDist) / squareLen(straightDist);

		float2 lightCoord = float2((lightPos.x / lightPos.w + 1)/2, (1 - lightPos.y / lightPos.w)/2);
		float shadowSpl = spotShadowMap.Sample(splr, lightCoord);
		//return float4((lightPos.x+1)/2, (lightPos.y+1)/2, lightPos.z, 1);
//		return float4((lightPos.x / lightPos.w + 1)/2, (lightPos.y / lightPos.w+1)/2, lightPos.z / lightPos.w, 1)
float bias = 0.001;
		bool lit = false;
		if (lightCoord.x <= 1 && lightCoord.x >= 0 && lightCoord.y <= 1 && lightCoord.y >= 0 && !lit) {
		 //	return float4(shadowSpl, shadowSpl, shadowSpl, 1)
		//		+ float4(colour * 0.01, 0);
			lit =  shadowSpl + bias >= lightPos.z / lightPos.w;
		}

		colour += lit * (tangleSq < square(spotLightTan) && dot(spotLightDir, lightDir) > 0) * ComputeLighting(normal, texColour * ambdiffspec.y, lightCol, lightDir, specularExp, viewDir);
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