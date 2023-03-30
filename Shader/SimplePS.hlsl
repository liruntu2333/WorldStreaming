struct PixelIn
{
	float4 PositionH : SV_Position;
	float3 PositionW : POSITION;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float2 TexCoord : TEXCOORD;
};

Texture2D g_Textures : register( t0 );
SamplerState g_LinearWrap : register( s0 );

float4 main( PixelIn pin ) : SV_TARGET
{
	return g_Textures.Sample(g_LinearWrap, pin.TexCoord);
}