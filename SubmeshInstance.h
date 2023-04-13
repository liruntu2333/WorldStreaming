#pragma once
#include <cstdint>

// Remember to change in shader as well.
struct SubmeshInstance
{
	uint32_t SubsetId{};
	uint32_t MatIdx{};
	uint32_t ObjectIdx{};

	SubmeshInstance() = default;
	SubmeshInstance(uint32_t meshId, uint32_t matIdx, uint32_t objectIdx)
		: SubsetId(meshId), MatIdx(matIdx), ObjectIdx(objectIdx) {}
};
