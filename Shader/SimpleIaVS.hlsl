struct Vertex
{
	float3 Position;
	float3 Normal;
	float3 Tangent;
	float2 TexCoord;
};

struct VertexIn
{
	uint SubsetId : SubsetID;
	uint MaterialId : MaterialID;
	uint ObjectId : ObjectID;
	uint VertexId : SV_VertexID;
};

struct ObjectInstance
{
	float4x4 World;
	uint Color;
	uint Param[3];
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

StructuredBuffer<Vertex> g_Vertices : register(t0);
StructuredBuffer<ObjectInstance> g_ObjInstances : register(t1);

VertexOut main(VertexIn vin)
{
	VertexOut vout;

	ObjectInstance objIns = g_ObjInstances[vin.ObjectId];
	Vertex vert = g_Vertices[vin.SubsetId * g_VertexPerMesh + vin.VertexId];

	float4 posW = mul(float4(vert.Position, 1.0f), objIns.World);
	const float3 normW = mul(vert.Normal, (float3x3) objIns.World);
	const float4 posH = mul(posW, g_ViewProj);

	vout.PositionW = posW.xyz;
	vout.PositionH = posH;
	vout.Normal = normW;
	vout.Tangent = vert.Tangent;
	vout.TexCoord = vert.TexCoord;
	vout.MatIdx = vin.MaterialId;
	vout.Color = objIns.Color;

	return vout;
}
