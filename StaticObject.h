#pragma once
#include <directxtk/SimpleMath.h>

struct StaticObject
{
    DirectX::SimpleMath::Vector3 Position;
    float Scale{};
    DirectX::SimpleMath::Quaternion Rotation;
    uint32_t GeometryIndex{};
    uint32_t MaterialIndex{};
    uint32_t Color{};
    uint32_t Param{};

    StaticObject() = default;
    StaticObject(DirectX::SimpleMath::Vector3 position, DirectX::SimpleMath::Quaternion rotation, float scale, uint32_t geometryIndex, uint32_t materialIndex, uint32_t color)
        : Position(position), Scale(scale), Rotation(rotation), GeometryIndex(geometryIndex), MaterialIndex(materialIndex), Color(color) {}
};
