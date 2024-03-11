

cbuffer PS2DCBuff {
	float exponent;
	bool isDepth;
}

Texture2D tex;
SamplerState splr;

float4 main(float2 uv: TexCoord) : SV_TARGET
{
	//return float4(uv,0,1);
	float4 col = tex.Sample(splr, uv);
	if (isDepth)
	{
		col = float4(col.x, col.x, col.r, 1);
	}
	col = pow(col, float4(exponent, exponent, exponent, 1));
	return col;
}