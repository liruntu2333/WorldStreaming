#pragma once
#include <directxtk/SimpleMath.h>

// Remember to change in shader as well.
struct Material
{
	uint32_t TexIdx {}; // Global texture index

	Material() = default;
};

