struct MergedSubmeshInstance
{
	float4x4 World;
	uint Color;
	uint SubsetId;
	uint MatIdx;
	uint ObjectIdx;
};

struct Vertex
{
	float3 Position;
	float3 Normal;
	float3 Tangent;
	float2 TexCoord;
};

struct VertexOut
{
	float4 PositionH : SV_Position;
	float3 PositionW : POSITION;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float2 TexCoord : TEXCOORD;
	nointerpolation uint MatIdx : Material;
	nointerpolation uint Color : PackedColor;
};


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

StructuredBuffer<MergedSubmeshInstance> g_SubInstances : register(t0);
StructuredBuffer<Vertex> g_Vertices : register(t1);

VertexOut main(uint vi : SV_VertexID, uint ii : SV_InstanceID)
{
	VertexOut vout;

	const MergedSubmeshInstance subIns = g_SubInstances[ii];
	Vertex vert = g_Vertices[subIns.SubsetId * g_VertexPerMesh + vi];

	float4 posW = mul(float4(vert.Position, 1.0f), subIns.World);
	const float3 normW = mul(vert.Normal, (float3x3) subIns.World);
	const float4 posH = mul(posW, g_ViewProj);

	vout.PositionW = posW.xyz;
	vout.PositionH = posH;
	vout.Normal = normW;
	vout.Tangent = vert.Tangent;
	vout.TexCoord = vert.TexCoord;
	vout.MatIdx = subIns.MatIdx;
	vout.Color = subIns.Color;

	return vout;
}