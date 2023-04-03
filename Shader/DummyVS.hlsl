cbuffer PassConstants : register(b0)
{
	float4x4 g_ViewProj;
	float3 g_EyePosition;
	float g_DeltaTime;
	float4x4 g_View;
	float4x4 g_Proj;
	uint g_VertexPerMesh;
	uint pad0;
	uint pad1;
	uint pad2;
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