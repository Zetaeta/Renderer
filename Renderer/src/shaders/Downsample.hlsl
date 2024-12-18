
Texture2D inTex;
RWTexture2D outTex;

[numthreads(8, 8, 1)]
void main( uint3 pixCoord3 : SV_DispatchThreadID )
{
    uint outWidth, outHeight;
    outTex.GetDimensions(outWidth, outHeight);
    if (pixCoord3.x < outWidth || pixCoord3.x < outHeight)
    {
        return;
    }

    uint inDim;
    inTex.GetDimensions(inDim.x, inDim.y);
    outTex[pixCoord3.xy] =
        (inTex.Load(clamp(pixCoord3 * 2, 0, inDim))
      + inTex.Load(clamp(pixCoord3 * 2 + float2(1, 0), 0, inDim))
      + inTex.Load(clamp(pixCoord3 * 2 + float2(0, 1), 0, inDim))
      + inTex.Load(clamp(pixCoord3 * 2 + float2(1, 1), 0, inDim))) / 4;

}