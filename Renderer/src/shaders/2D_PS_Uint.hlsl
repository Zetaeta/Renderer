

cbuffer PerInstancePSData
{
    uint width;
    uint height;
}

Texture2D<uint> tex;

unsigned int unhash(unsigned int x) {
    x = ((x >> 16) ^ x) * 0x119de1f3;
    x = ((x >> 16) ^ x) * 0x119de1f3;
    x = (x >> 16) ^ x;
    return x;
}

float4 main(float2 uv: TexCoord) : SV_TARGET
{
	//return float4(uv,0,1);
    int x = uv.x * width;
    int y = uv.y * height;
    uint num = tex.Load(int3(x, y, 0));
    uint hash = unhash(num);
//    if (num > 0)
//    {
//        return float4(1,0,1,1);
//    }
//
//    return float4(0,1,1,1);
    float b = (hash & 0xff) / 255.f;
    float g = ((hash >> 8) & 0xff) / 255.f;
    float r = ((hash >> 16) & 0xff) / 255.f;
    return float4(r,g,b,1);
}