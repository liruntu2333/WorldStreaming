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
};

cbuffer PassConstants : register( b0 )
{
	float4x4 g_ViewProj;
}

StructuredBuffer<Vertex> g_Vertices : register( t0 );

VertexOut main(uint vi : SV_VertexID)
{
	VertexOut vout = (VertexOut)0;

	Vertex v = g_Vertices[vi];
	vout.PositionW = v.Position;
	vout.PositionH = mul(float4(v.Position, 1.0f), g_ViewProj);
	vout.Normal = v.Normal;
	vout.Tangent = v.Tangent;
	vout.TexCoord = v.TexCoord;

	return vout;
}