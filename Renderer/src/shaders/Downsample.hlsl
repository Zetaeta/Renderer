
#ifndef NUM_DOWNSAMPLE_LEVELS
#define NUM_DOWNSAMPLE_LEVELS 4
#endif

Texture2D inTex;
RWTexture2D<float4> outTex;
#if NUM_DOWNSAMPLE_LEVELS > 1
RWTexture2D<float4> outTex_1;
#endif

#if NUM_DOWNSAMPLE_LEVELS > 2
RWTexture2D<float4> outTex_2;
#endif

#if NUM_DOWNSAMPLE_LEVELS > 3
RWTexture2D<float4> outTex_3;
#endif

SamplerState trilinearSampler : register(s0);
SamplerState pointClampedSampler : register(s2);

groupshared float4 scratch[8][8];

float3 encodeSRGB(float3 rgb)
{
    return pow(rgb, 1/2.2);
}

void WriteOut(RWTexture2D<float4> tex, uint2 coord, float4 value)
{
#if IS_SRGB
    value = float4(encodeSRGB(value.rgb), value.a);
//#else
//    #error help
//    value = float4(1, 0, 0, 1);
#endif
    tex[coord] = value;
}

[numthreads(8, 8, 1)]
void DownsampleCS( uint3 pixCoord3 : SV_DispatchThreadID, uint3 threadId : SV_GroupThreadID )
{
    uint outWidth, outHeight;
    uint inWidth, inHeight;
    outTex.GetDimensions(outWidth, outHeight);
    inTex.GetDimensions(inWidth, inHeight);
    if (pixCoord3.x >= outWidth || pixCoord3.x >= outHeight)
    {
        return;
    }
    float2 uv = (pixCoord3.xy + 0.5) / outWidth;

    float4 avg = inTex.SampleLevel(trilinearSampler, uv, 0);
    WriteOut(outTex, pixCoord3.xy, avg);

#if NUM_DOWNSAMPLE_LEVELS > 1
    scratch[threadId.x][threadId.y] = avg;
    uint2 coord = pixCoord3.xy;
    for (int i = 0; i < NUM_DOWNSAMPLE_LEVELS - 1; ++i)
    {
        if (any(coord.xy & 1))
        {
            return;
        }
        coord = coord >> 1;
        int offset = 1 << i;
        GroupMemoryBarrier();
        float4 avg = scratch[threadId.x][threadId.y] + scratch[threadId.x + offset][threadId.y]
            + scratch[threadId.x][threadId.y + offset] + scratch[threadId.x + offset][threadId.y + offset];
        avg /= 4;
        scratch[threadId.x][threadId.y] = avg;
        if (i == 0)
            WriteOut(outTex_1, coord, avg);
        #if NUM_DOWNSAMPLE_LEVELS > 2
        if (i == 1)
            WriteOut(outTex_2, coord, avg);
        #endif
        #if NUM_DOWNSAMPLE_LEVELS > 3
        if (i == 2)
            WriteOut(outTex_3, coord, avg);
        #endif

    }
#endif

    //uint inDim;
    //inTex.GetDimensions(inDim.x, inDim.y);
    //outTex[pixCoord3.xy] =
    //    (inTex.Load(clamp(pixCoord3 * 2, 0, inDim))
    //  + inTex.Load(clamp(pixCoord3 * 2 + float2(1, 0), 0, inDim))
    //  + inTex.Load(clamp(pixCoord3 * 2 + float2(0, 1), 0, inDim))
    //  + inTex.Load(clamp(pixCoord3 * 2 + float2(1, 1), 0, inDim))) / 4;

}