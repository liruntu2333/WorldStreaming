#pragma once
#include <directxtk/SimpleMath.h>

struct ObjectInstance
{
	DirectX::SimpleMath::Matrix World;
	uint32_t Color {};
	uint32_t Param[3] {};

	ObjectInstance() = default;

	ObjectInstance(
		const DirectX::SimpleMath::Matrix& world, uint32_t color)
		: World(world), Color(color) {}
};
