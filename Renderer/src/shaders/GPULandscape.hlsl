#include "VertexShader.hlsli"

Texture2D<float> Heightmap;

SamplerState BilinearSampler;

static const float Scale = 1.f;
static const float Delta = 1.f / 1024.f;

struct LandscapeInput
{
    float3 Position : POSITION;
    float2 UV : TexCoord;
};

float SampleHeight(float2 uv)
{
    return Heightmap.SampleLevel(BilinearSampler, uv + float2(Delta, 0), 0);
}

VSInputs GenerateVertexInputs(LandscapeInput input)
{
    VSInputs vsi;
    float2 uv = input.UV;
//    float height = Heightmap.SampleLevel(BilinearSampler, uv, 0);
    float height = 0;
    vsi.Position = input.Position + float3(0, 0, height);
#if SHADED
    //float heightPX = Heightmap.SampleLevel(BilinearSampler, uv + float2(Delta, 0), 0);
    //float heightMX = Heightmap.SampleLevel(BilinearSampler, uv - float2(Delta, 0), 0);
    //float heightPY = Heightmap.SampleLevel(BilinearSampler, uv + float2(0, Delta), 0);
    //float heightMY = Heightmap.SampleLevel(BilinearSampler, uv - float2(0, Delta), 0);

    //float dhdx = (heightPX - heightMX) * 0.5f / Delta;
    //float dhdy = (heightPY - heightMY) * 0.5f / Delta;
    float dhdy = 0;
    float dhdx = 0;

    // For a parametrized surface, cross product of partial derivatives is normal to surface
    float3 normal = normalize(cross(float3(1, 0, dhdx), float3(0, 1, dhdy)));
    float3 TX = normalize(float3(1, 0, dhdx));

    vsi.Normal = normal;
    vsi.Tangent = TX;
#endif
    vsi.UV = input.UV; 
    return vsi;
}

VSOut VSMain(LandscapeInput input)
{
    VSInputs vertexInput = GenerateVertexInputs(input);
    return DefaultVertexShader(vertexInput);
}


