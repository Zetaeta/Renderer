cbuffer PSCB
{
    float4 colour;
};

Texture2D tex;
SamplerState splr;

float4 TestVS1( uint vertId : SV_VertexID ) : SV_POSITION
{
    float4 position = float4(0, 0, 0.5, 1);
    position.x = float(vertId) / 2 - 0.5;
    position.y = (vertId == 1) ? 1 : -1;
    return position;
}

struct VSOut
{
    float2 uv : TEXCOORD;
    float4 pos : SV_Position;
};

VSOut TestVS2(float3 position : Position) 
{
    VSOut vso;
    vso.uv = position.xy;
    vso.pos = 
    float4(position, 1);
    return vso;
}

float4 TestPS(float2 uv : TexCoord) : SV_Target
{
    return tex.Sample(splr, uv) + float4(uv / 2, 0, 1) + colour / 3;
}