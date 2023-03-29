#pragma once

#include <directxtk/SimpleMath.h>

using namespace DirectX::SimpleMath;
struct PassConstants
{
    Matrix ViewProj;
    Vector3 EyePosition;
    float DeltaTime{};

    PassConstants() = default;
    PassConstants(const Matrix& vp, const Vector3& ep, const float dt) :
        ViewProj(vp), EyePosition(ep), DeltaTime(dt) {}
};
