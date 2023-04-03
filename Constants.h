#pragma once

#include <directxtk/SimpleMath.h>

using namespace DirectX::SimpleMath;
struct Constants
{
    Matrix ViewProj;
    Vector3 EyePosition;
    float DeltaTime{};
    Matrix View;
    Matrix Proj;
    uint32_t VertexPerMesh = 0;
    uint32_t pad0;
    uint32_t pad1;
    uint32_t pad2;

    Constants() = default;
};
