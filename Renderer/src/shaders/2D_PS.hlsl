
#ifndef CUSTOMIZATION
#define CUSTOMIZATION 1
#endif

#if CUSTOMIZATION
cbuffer PS2DCBuff {
	float exponent;
	int renderMode;
}
#endif

Texture2D tex;
SamplerState splr;

float4 main(float2 uv: TexCoord) : SV_TARGET
{
    float4 col = tex.Sample(splr, uv);
#if CUSTOMIZATION
    const bool isDepth = renderMode == 1;
    if (isDepth)
    {
        col = float4(col.x, col.x, col.r, 1);
    }
    col = pow(col, float4(exponent, exponent, exponent, 1));
#endif
    return col;
}