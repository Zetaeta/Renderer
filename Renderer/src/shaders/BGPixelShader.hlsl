
TextureCube tex : register(t0);
SamplerState splr;

float4 main(float3 uvw: TexCoord) : SV_TARGET
{
	//return float4(1,1,1,1);
//	return float4(uvw, 1);
	return tex.Sample(splr, uvw);
}