#pragma once

// Remember to change in shader as well.
struct Material
{
	uint32_t TexIdx = 0; // Global texture index

	Material() = default;

	Material(uint32_t texIdx)
		: TexIdx(texIdx) {}
};
