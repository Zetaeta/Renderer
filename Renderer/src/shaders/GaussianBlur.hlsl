
#ifndef MAX_RAD
#define MAX_RAD 16
#endif

Texture2D inTex;
RWTexture2D<float4> outTex;

cbuffer BlurData
{
    uint2 direction;
    uint radius;
    float coeffs[MAX_RAD + 1];
}

[numthreads(8, 8, 1)]
void main( uint3 pixCoord3 : SV_DispatchThreadID )
{
    float4 result = coeffs[0].x * inTex.Load(pixCoord3);
    for (int i = 0; i < radius; ++i)
    {
        result += coeffs[i + 1].x * (inTex.Load(uint3(pixCoord3.xy + direction * i, 0)) + inTex.Load(uint3(pixCoord3.xy - direction * i, 0)));
    }
    outTex[pixCoord3.xy] = result;

}