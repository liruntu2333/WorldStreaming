cbuffer PassConstants : register(b0)
{
	float4x4 g_ViewProj;
}

static const float4 g_Plane[6] =
{
	float4(-300.0f, 0.0f, -300.0f, 1.0f),
	float4(-300.0f, 0.0f, +300.0f, 1.0f),
	float4(+300.0f, 0.0f, +300.0f, 1.0f),
	float4(-300.0f, 0.0f, -300.0f, 1.0f),
	float4(+300.0f, 0.0f, +300.0f, 1.0f),
	float4(+300.0f, 0.0f, -300.0f, 1.0f),
};

float4 main(uint vi : SV_VertexID) : SV_Position
{
	return mul(g_Plane[vi], g_ViewProj);
}