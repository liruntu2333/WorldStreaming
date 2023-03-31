#pragma once
#include <directxtk/SimpleMath.h>

struct StaticObject
{
    DirectX::SimpleMath::Vector3 Position;
    DirectX::SimpleMath::Quaternion Rotation;
    float Scale{};
    uint32_t GeometryIndex{};
    uint32_t MaterialIndex{};
    uint32_t Color{};

    StaticObject() = default;
    StaticObject(DirectX::SimpleMath::Vector3 position, DirectX::SimpleMath::Quaternion rotation, float scale, uint32_t geometryIndex, uint32_t materialIndex, uint32_t color)
        : Position(position), Rotation(rotation), Scale(scale), GeometryIndex(geometryIndex), MaterialIndex(materialIndex), Color(color) {}
};
