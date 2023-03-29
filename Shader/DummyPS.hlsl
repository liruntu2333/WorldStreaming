float4 main(float4 PosH : SV_Position) : SV_TARGET
{
	float col = PosH.w / 500.0f;
	return float4(col, col, col, 1.0f);
}