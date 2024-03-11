
TextureCube tex : register(t0);
SamplerState splr;

float4 main(float3 uvw: TexCoord) : SV_TARGET
{
	//return float4(1,1,1,1);
//	return float4(uvw, 1);
	//float s = tex.Sample(splr, uvw);
	//s = pow(s,0.5);
	//return float4(s,s,s,1);
	return tex.Sample(splr, uvw);
}