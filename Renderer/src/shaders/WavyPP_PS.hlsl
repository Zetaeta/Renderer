Texture2D sceneTexture;

cbuffer CBTime
{
	float time;
}

float4 main(float4 pos: SV_POSITION) : SV_TARGET
{
	float phase = (sin(pos.x / 100) + cos(pos.y / 100) + time) * 6.28;
	float x = 1, y = 0;
	sincos(phase, x, y);
	const float Waviness = 2;
	float2 d = float2(x,y) * Waviness;
	return sceneTexture[ d + pos.xy];
}