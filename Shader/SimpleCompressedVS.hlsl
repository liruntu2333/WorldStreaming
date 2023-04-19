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

StructuredBuffer<uint4> g_CompressedVertices : register(t0);
StructuredBuffer<ObjectInstance> g_ObjInstances : register(t1);

Vertex DecompressVertex(uint4 compressedV)
{
	Vertex vert;
	const uint p = compressedV.x;
	const uint n = compressedV.y;
	const uint t = compressedV.z;
	const uint tc = compressedV.w;

	vert.Position = float3(float(p >> 20 & 0x3FF), float(p >> 10 & 0x3FF), float(p & 0x3FF)) / 512.0f - 1.0f;
	vert.Normal   = float3(float(n >> 20 & 0x3FF), float(n >> 10 & 0x3FF), float(n & 0x3FF)) / 512.0f - 1.0f;
	vert.Tangent  = float3(float(t >> 20 & 0x3FF), float(t >> 10 & 0x3FF), float(t & 0x3FF)) / 512.0f - 1.0f;
	vert.TexCoord = float2(f16tof32(tc), f16tof32(tc >> 16));
	return vert;
}

const uint4 s_Test[] =
{
	uint4(0x000FFE00, 0, 0, 0),
	uint4(0x3FFFFE00, 0, 0, 0),
	uint4(0x00000200, 0, 0, 0),
};

VertexOut main(VertexIn vin)
{
	VertexOut vout;

	ObjectInstance objIns = g_ObjInstances[vin.ObjectId];
	const uint vi = vin.SubsetId * g_VertexPerMesh + vin.VertexId;
	const uint4 vCompressed = g_CompressedVertices[vi];
	Vertex vert = DecompressVertex(vCompressed);

	float4 posW = mul(float4(vert.Position, 1.0f), objIns.World);
	//float4 posW = float4(vert.Position, 1.0f);
	const float3 normW = mul(vert.Normal, (float3x3)objIns.World);
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
