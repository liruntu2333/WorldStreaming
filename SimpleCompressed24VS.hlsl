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

struct PackedVertex
{
	uint PxPy;
	uint PzNx;
	uint NyNz;
	uint TxTy;
	uint TzPad;
	uint Uv;
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

StructuredBuffer<PackedVertex> g_CompressedVertices : register(t0);
StructuredBuffer<ObjectInstance> g_ObjInstances : register(t1);

Vertex DecompressVertex(PackedVertex cv)
{
	Vertex vert;
	
	vert.Position = float3(f16tof32(cv.PxPy), f16tof32(cv.PxPy >> 16), f16tof32(cv.PzNx));
	vert.Normal = float3(f16tof32(cv.PzNx >> 16), f16tof32(cv.NyNz), f16tof32(cv.NyNz >> 16));
	vert.Tangent = float3(f16tof32(cv.TxTy), f16tof32(cv.TxTy >> 16), f16tof32(cv.TzPad));
	vert.TexCoord = float2(f16tof32(cv.Uv), f16tof32(cv.Uv >> 16));
	return vert;
}

VertexOut main(VertexIn vin)
{
	VertexOut vout;

	ObjectInstance objIns = g_ObjInstances[vin.ObjectId];
	const uint vi = vin.SubsetId * g_VertexPerMesh + vin.VertexId;
	const PackedVertex vCompressed = g_CompressedVertices[vi];
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
