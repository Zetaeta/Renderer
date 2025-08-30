#define TEXTURED 0
#define VERTEX_COLOUR 1
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

static const float SnowCutoff = 1.2f;
static const float RockCutoff = .5f;
static const float WaterCutoff = -1.5f;

VSInputs GenerateVertexInputs(LandscapeInput input, out float4 colour)
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
    if (height > SnowCutoff)
	{
		colour = float4(1, 1, 1, 1);
	}
    else if (normal.y < 0.02 || height > RockCutoff)
	{
		colour = float4(0.5, 0.5, 0.5, 1);
	}
	else if (height > WaterCutoff)
	{
		colour = float4(0, 0.5, 0, 1);
	}
	else
	{
		colour = float4(0, 0, 0.5, 1);
	}
//	colour = float4(normal, 1);
    return vsi;
}

VSOut VSMain(LandscapeInput input)
{
	float4 colour;
    VSInputs vertexInput = GenerateVertexInputs(input, colour);
    VSOut result = DefaultVertexShader(vertexInput);
	result.vertexColour = colour;
	return result;
}


