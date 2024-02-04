
cbuffer PerFramePSData {
	float3 ambient;
	float3 lightCol;
	float3 lightDir;
};

cbuffer PerInstancePSData : register(b1) {
	float3 colour;
	float3 ambdiffspec;
	int specularExp;
};

#define BLINN

float4 main(float3 normal: Normal, float3 viewDir: ViewDir, float3 vertColour : Colour) : SV_TARGET
{
	normal = normalize(normal);
	//return float4(lightDir, 1);
	float diffInt = lightCol * saturate(dot(-lightDir, normal)) * ambdiffspec.y;

#ifdef BLINN
	float f = max(dot(normalize(-normalize(viewDir) - lightDir),normal), 0.00001f);
#else
	float3 outRay = normalize(lightDir - 2 * dot(lightDir, normal) * normal);
	float f = max(dot(normalize(viewDir),-outRay), 0.00001f);
#endif
//return f;

	float specInt = pow(f, specularExp) * (specularExp > 0);// * ambdiffspec.z; 
	float x = specularExp / 500.0;//specularExp  > 0 ? 1 : 0;
	return float4(colour * (ambient * ambdiffspec.x + lightCol * diffInt) + lightCol * specInt, 1.0f);
}