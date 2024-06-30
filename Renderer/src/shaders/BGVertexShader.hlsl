
struct VSOut {
	float3 uvw: TexCoord;
	float4 pos : SV_POSITION;
};

/*cbuffer WorldPos {

	float3 v0;
	float3 v1;
	float3 
}*/

cbuffer PerFrameVertexData
{
	Matrix screen2World;
	Matrix world2Camera;
	float3 cameraPos;
}

VSOut main( float3 pos : POSITION, float2 uv : TexCoord )
{
	VSOut vso;
	vso.pos = float4(pos, 1);
	float4 worldPos = mul(screen2World, float4(pos.xy, 1,1));
	vso.uvw = worldPos.xyz/worldPos.w - cameraPos;
	//vso.uvw = pos;
	//vso.uvw = cameraPos;
	return vso;
}