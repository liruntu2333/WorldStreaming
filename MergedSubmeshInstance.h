#pragma once
#include <directxtk/SimpleMath.h>
#include "SubmeshInstance.h"
#include "ObjectInstance.h"

// Remember to change in shader as well.
struct MergedSubmeshInstance
{
	DirectX::SimpleMath::Matrix World;
	uint32_t Color {};
	uint32_t SubsetId {};
	uint32_t MatIdx {};
	uint32_t ObjectIdx {};

	MergedSubmeshInstance() = default;

	MergedSubmeshInstance(const ObjectInstance& object, const SubmeshInstance& submesh)
		: World(object.World), Color(object.Color), SubsetId(submesh.SubsetId), MatIdx(submesh.MatIdx),
		ObjectIdx(submesh.ObjectIdx) {}
};
