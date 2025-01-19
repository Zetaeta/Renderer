

Texture2D sceneTexture;

#ifndef USE_MSAA
#define USE_MSAA 0
#endif

#if USE_MSAA
Texture2DMS<uint2> sceneStencil;
#else
Texture2D<uint2> sceneStencil;
#endif
//SamplerState splr;

float4 main(float4 pos: SV_POSITION) : SV_TARGET
{
    float4 col = sceneTexture[pos.xy];
//    return float4(pos.xy / 1000, 0, 1);
 //   return sceneStencil[pos.xy] / 255.f;
//	return float4(val, val, val, 1);
    uint myVal = sceneStencil[pos.xy].g;
    if (myVal != sceneStencil[pos.xy - uint2(0,1)].g
		|| myVal != sceneStencil[pos.xy - uint2(1,0)].g
		|| myVal != sceneStencil[pos.xy + uint2(1,0)].g
		|| myVal != sceneStencil[pos.xy + uint2(0,1)].g
		)
    {
        return float4(1,1,0,1);
    }
    return col; 
}
