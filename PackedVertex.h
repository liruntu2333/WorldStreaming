#pragma once
#include <DirectXPackedVector.h>
#include "VertexPositionNormalTangentTexture.h"

struct PackedVertex
{
	DirectX::PackedVector::XMHALF2 PxPy {};
	DirectX::PackedVector::XMHALF2 PzNx {};
	DirectX::PackedVector::XMHALF2 NyNz {};
	DirectX::PackedVector::XMHALF2 TxTy {};
	DirectX::PackedVector::XMHALF2 TzPad {};
	DirectX::PackedVector::XMHALF2 Uv {};

	PackedVertex() = default;

	explicit PackedVertex(const VertexPositionNormalTangentTexture& vtx)
	{
		PxPy = DirectX::PackedVector::XMHALF2(vtx.Pos.x, vtx.Pos.y);
		PzNx = DirectX::PackedVector::XMHALF2(vtx.Pos.z, vtx.Nor.x);
		NyNz = DirectX::PackedVector::XMHALF2(vtx.Nor.y, vtx.Nor.z);
		TxTy = DirectX::PackedVector::XMHALF2(vtx.Tan.x, vtx.Tan.y);
		TzPad = DirectX::PackedVector::XMHALF2(vtx.Tan.z, 0.0f);
		Uv = DirectX::PackedVector::XMHALF2(vtx.Tc.x, vtx.Tc.y);
	}
};
