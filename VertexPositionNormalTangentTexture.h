#pragma once
#include <directxtk/SimpleMath.h>

struct VertexPositionNormalTangentTexture
{
    DirectX::SimpleMath::Vector3 Pos;
    DirectX::SimpleMath::Vector3 Nor;
    DirectX::SimpleMath::Vector3 Tan;
    DirectX::SimpleMath::Vector2 Tc;

    VertexPositionNormalTangentTexture() = default;
    VertexPositionNormalTangentTexture(
        const DirectX::SimpleMath::Vector3& pos, 
        const DirectX::SimpleMath::Vector3& nor, 
        const DirectX::SimpleMath::Vector3& tan, 
        const DirectX::SimpleMath::Vector2& tc)
        : Pos(pos), Nor(nor), Tan(tan), Tc(tc) {}
};

