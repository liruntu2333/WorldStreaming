#pragma once
#include <directxtk/SimpleMath.h>

struct Instance
{
    DirectX::SimpleMath::Matrix World;
    uint32_t GeoIdx{};
    uint32_t MatIdx{};
    uint32_t Color{};
    uint32_t Param{};

    Instance() = default;
    Instance(DirectX::SimpleMath::Matrix world, uint32_t geoIdx, uint32_t matIdx, uint32_t color, uint32_t param)
        : World(world), GeoIdx(geoIdx), MatIdx(matIdx), Color(color), Param(param) {}
};
