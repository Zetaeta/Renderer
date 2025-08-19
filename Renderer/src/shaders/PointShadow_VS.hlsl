
#include "VertexShader.hlsli"

struct MyVSOut {
	float3 lightSpacePos : LightSpacePos;
	float4 pos : SV_POSITION;
};

//cbuffer PointShadowVS
//{
//	Matrix projection;
//	Matrix world2Light;
//}

MyVSOut main( float3 pos : POSITION )
{
	MyVSOut vso;
	vso.pos = mul(fullTrans,float4(pos, 1));
	vso.lightSpacePos = mul(world2Light, mul(model2Shade, float4(pos, 1))).xyz;
	return vso;
}