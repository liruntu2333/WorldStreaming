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

Texture2D<uint4> g_CompressedVertices : register(t0);
StructuredBuffer<ObjectInstance> g_ObjInstances : register(t1);

float3 DecodeOctahedron(uint n)
{
	float2 octahedron = float2(n >> 8 & 0xFF, n & 0xFF) / 128.0f - 1.0f;
	float3 normal = float3(octahedron, 1 - dot(1, abs(octahedron)));
	if (normal.z < 0)
	{
		normal.xy = (1 - abs(normal.yx)) * (normal.xy >= 0 ? float2(1, 1) : float2(-1, -1));
	}
	return normalize(normal);
}

void DecodeNormalAndTangent(in uint nt, out float3 normal, out float3 tangent)
{
	normal = DecodeOctahedron(nt >> 16 & 0xFFFF);
	tangent = DecodeOctahedron(nt & 0xFFFF);
}

Vertex DecompressVertex(uint4 compressedV)
{
	Vertex vert;
	const uint vp1 = compressedV.x;
	const uint vp2 = compressedV.y;
	const uint vnt = compressedV.z;
	const uint vtc = compressedV.w;

	vert.Position = float3(vp1 >> 16 & 0xFFFF, vp1 & 0xFFFF, vp2 >> 16 & 0xFFFF) / 32768.0f - 1.0f;
	DecodeNormalAndTangent(vnt, vert.Normal, vert.Tangent);
	vert.TexCoord = (float2(vtc >> 16 & 0xFFFF, vtc & 0xFFFF) / 32768.0f - 1.0f) * 8.0f;
	return vert;
}

VertexOut main(VertexIn vin)
{
	VertexOut vout;

	ObjectInstance objIns = g_ObjInstances[vin.ObjectId];
	const uint globalVi = vin.SubsetId * g_VertexPerMesh + vin.VertexId;
	const uint4 vCompressed = g_CompressedVertices[uint2(globalVi % 512, globalVi / 512)];
	Vertex vert = DecompressVertex(vCompressed);

	float4 posW = mul(float4(vert.Position, 1.0f), objIns.World);
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
