
cbuffer PerFramePSData : register(b0) {
#if BASE_LAYER || GBUFFER
	float3 ambient;
#endif
#if DIR_LIGHT
#define USE_NORMAL 1
#define SHADOWMAP_2D 1
	float3 dirLightCol;
	float3 dirLightDir;
	Matrix world2Light;
#endif

#if POINT_LIGHT
#define USE_NORMAL 1
	float3 pointLightPos;
	float pointLightRad;
	float3 pointLightCol;
	float pointLightFalloff;
	Matrix world2Light;
#endif

#if SPOTLIGHT
#define USE_NORMAL 1
#define SHADOWMAP_2D 1
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


#ifdef SHADOWMAP_2D
#define SHADOWMAP_DECLARATION(reg) Texture2D spotShadowMap reg;
#else

#if POINT_LIGHT
#if !USE_NORMAL
#error Ohno
#endif
#define SHADOWMAP_DECLARATION(reg) TextureCube pointShadowMap reg
#else
#define SHADOWMAP_DECLARATION(reg)
#endif

#endif
