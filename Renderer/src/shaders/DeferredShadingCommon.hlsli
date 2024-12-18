#ifndef DEFERREDSHADINGCOMMON_HLSLI
#define DEFERREDSHADINGCOMMON_HLSLI

float3 PackNormalToUnorm(float3 normalVector)
{
    return (normalVector + float3(1, 1, 1)) * 0.5;
}

float3 UnpackNormalFromUnorm(float3 texNormal)
{
    return (texNormal * 2) - float3(1, 1, 1);
}

#endif // DEFERREDSHADINGCOMMON_HLSLI
