
cbuffer PerInstanceVSData : register(b0)
{
	Matrix fullTrans;
	Matrix model2Shade;
	Matrix model2ShadeDual;
}

cbuffer PerFrameVertexData : register(b1)
{
	Matrix screen2World;
	Matrix world2Light;
	float3 cameraPos;
}
