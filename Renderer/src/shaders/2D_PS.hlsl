
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
//    return col + float4(1,1, 0, 1);
    const bool isDepth = renderMode == 1;
    if (isDepth)
    {
        col = float4(col.x, col.x, col.r, 1);
    }
    else if (renderMode == 3)
    {
//        col = float4(1,1, 0, 1);
        col = float4(saturate(col.x),0, saturate(-col.x), 1);
    }
//    col = pow(col, float4(exponent, exponent, exponent, 1));
#endif
//    col = float4(uv, 0, 0);
    return col;
}