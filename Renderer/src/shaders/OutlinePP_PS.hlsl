

Texture2D tex;
SamplerState splr;

float4 main(float4 pos: SV_POSITION) : SV_TARGET
{
    float4 col = tex[pos.xy];
//    return float4(pos.xy / 1000, 0, 1);
    return col / 2;
}
