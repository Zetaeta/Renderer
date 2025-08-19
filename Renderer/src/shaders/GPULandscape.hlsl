#define TEXTURED 0
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
    float height = Heightmap.SampleLevel(BilinearSampler, uv, 0);
    //float height = 0;
    //float dhdy = 0;
    //float dhdx = 0;
    vsi.Position = input.Position + float3(0, height, 0); // + float3(uv, 0) * 0.0001;
#if SHADED
    float heightPX = Heightmap.SampleLevel(BilinearSampler, uv + float2(Delta, 0), 0);
    float heightMX = Heightmap.SampleLevel(BilinearSampler, uv - float2(Delta, 0), 0);
    float heightPY = Heightmap.SampleLevel(BilinearSampler, uv + float2(0, Delta), 0);
    float heightMY = Heightmap.SampleLevel(BilinearSampler, uv - float2(0, Delta), 0);

    float dhdx = (heightPX - heightMX) * 0.5f / Delta;
    float dhdy = (heightPY - heightMY) * 0.5f / Delta;

    // For a parametrized surface, cross product of partial derivatives is normal to surface
    float3 normal = -normalize(cross(float3(1, dhdx, 0), float3(0, dhdy, 1)));
    float3 TX = -normalize(float3(1, dhdx, 0));

    vsi.Normal = normal;
    vsi.Tangent = TX;
#endif
#if TEXTURED
    vsi.UV = input.UV; 
#endif
    return vsi;
}

VSOut VSMain(LandscapeInput input)
{
    VSInputs vertexInput = GenerateVertexInputs(input);
    return DefaultVertexShader(vertexInput);
}


