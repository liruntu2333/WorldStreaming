float4 main(float4 PosH : SV_Position) : SV_TARGET
{
	float col = 0.4f;
	return float4(col, col, col, 1.0f);
}