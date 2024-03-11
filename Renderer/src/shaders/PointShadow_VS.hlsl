
#define screen2World world2Light;
#include "VertexShader.hlsli"

struct VSOut {
	float3 lightSpacePos : LightSpacePos;
	float4 pos : SV_POSITION;
};

//cbuffer PointShadowVS
//{
//	Matrix projection;
//	Matrix world2Light;
//}

VSOut main( float3 pos : POSITION )
{
	VSOut vso;
	vso.pos = mul(fullTrans,float4(pos, 1));
	vso.lightSpacePos = mul(world2Light, mul(model2Shade, float4(pos, 1))).xyz;
	return vso;
}