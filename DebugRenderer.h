#pragma once
#include "Renderer.h"
#include <directxtk/SimpleMath.h>

struct BvhLinearNode;

namespace DirectX
{
    namespace DX11
    {
        class GeometricPrimitive;
    }
}

struct GpuConstants;

class DebugRenderer : public Renderer
{
public:
    DebugRenderer(ID3D11Device* device, std::shared_ptr<GpuConstants> constants,
        const std::vector<BvhLinearNode>& tree);

    ~DebugRenderer() override = default;

    DebugRenderer(const DebugRenderer&) = delete;
    DebugRenderer(DebugRenderer&&) = delete;
    DebugRenderer& operator=(const DebugRenderer&) = delete;
    DebugRenderer& operator=(DebugRenderer&&) = delete;

    void Initialize(ID3D11DeviceContext* context) override;
    void Render(ID3D11DeviceContext* context) override;

private:
    void UpdateBuffer(ID3D11DeviceContext* context) override;

    std::unique_ptr<DirectX::GeometricPrimitive> m_SphereGeo = nullptr;
    std::shared_ptr<GpuConstants> m_Constants;
    const std::vector<BvhLinearNode>& m_BsTree;
};

