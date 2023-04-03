struct PixelIn
{
	float4 PositionH : SV_Position;
	float3 PositionW : POSITION;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float2 TexCoord : TEXCOORD;
	nointerpolation uint MatIdx : Material;
	nointerpolation uint Color : PackedColor;
	nointerpolation uint Param : Parameter;
};

float4 LoadColor(uint col)
{
	float r = ((col & 0xFF000000) >> 24) / 255.0f;
	float g = ((col & 0x00FF0000) >> 16) / 255.0f;
	float b = ((col & 0x0000FF00) >> 8 ) / 255.0f;
	float a = ((col & 0x000000FF) >> 0 ) / 255.0f;
	return float4(r, g, b, a);
}

Texture2DArray g_Textures : register( t0 );
SamplerState g_Sampler : register( s0 );

float4 main( PixelIn pin ) : SV_TARGET
{
	const float3 light = float3(0.0f, 1.0f, 0.0f);
	// sample a textur2darray
	const float4 diffuse = g_Textures.Sample(g_Sampler, float3(pin.TexCoord, pin.MatIdx));
	const float4 kd = LoadColor(pin.Color);
	return diffuse * saturate(dot(light, normalize(pin.Normal)));
}