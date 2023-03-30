#pragma once

#include <directxtk/SimpleMath.h>

using namespace DirectX::SimpleMath;
struct PassConstants
{
    Matrix ViewProj;
    Vector3 EyePosition;
    float DeltaTime{};
    Matrix View;
    Matrix Proj;

    PassConstants() = default;
};
