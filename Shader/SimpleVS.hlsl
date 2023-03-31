struct Vertex
{
	float3 Position;
	float3 Normal;
	float3 Tangent;
	float2 TexCoord;
};

struct InstanceData
{
	float4x4 World;
	uint GeoIdx;
	uint MatIdx;
	uint Color;
	uint Param;
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
	nointerpolation uint Param : Parameter;
};

cbuffer PassConstants : register( b0 )
{
	float4x4 g_ViewProj;
}

StructuredBuffer<Vertex> g_Vertices : register( t0 );
StructuredBuffer<InstanceData> g_Instances : register( t1 );

VertexOut main(uint vi : SV_VertexID, uint ii : SV_InstanceID)
{
	VertexOut vout = (VertexOut)0;

	Vertex vert = g_Vertices[vi];
	const InstanceData inst = g_Instances[ii];

	float4 posW = mul(float4(vert.Position, 1.0f), inst.World);
	const float3 normW = mul(vert.Normal, (float3x3)inst.World);
	const float4 posH = mul(posW, g_ViewProj);

	vout.PositionW = posW.xyz;
	vout.PositionH = posH;
	vout.Normal = normW;
	vout.Tangent = vert.Tangent;
	vout.TexCoord = vert.TexCoord;
	vout.MatIdx = inst.MatIdx;
	vout.Color = inst.Color;
	vout.Param = inst.Param;

	return vout;
}