float4 TestVS( uint vertId : SV_VertexID ) : SV_POSITION
{
    float4 position = float4(0, 0, 0.5, 1);
    position.x = float(vertId) / 2 - 0.5;
    position.y = (vertId == 1) ? 1 : -1;
    return position;
}

float4 TestPS() : SV_Target
{
    return float4(1, 0, 0, 1);
}