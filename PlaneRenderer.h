#pragma once

#include "PassConstants.h"
#include "Renderer.h"
#include <directxtk/BufferHelpers.h>
#include <wrl/client.h>

class PlaneRenderer : public Renderer
{
public:
    PlaneRenderer(ID3D11Device* device, std::shared_ptr<PassConstants> constants);
    ~PlaneRenderer() override = default;

    PlaneRenderer(const PlaneRenderer&) = delete;
    PlaneRenderer& operator=(const PlaneRenderer&) = delete;
    PlaneRenderer(PlaneRenderer&&) = delete;
    PlaneRenderer& operator=(PlaneRenderer&&) = delete;

    void Initialize(ID3D11DeviceContext* context) override;
    void Render(ID3D11DeviceContext* context) override;
    void UpdateBuffer(ID3D11DeviceContext* context) override;

protected:
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_Vs = nullptr;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_Ps = nullptr;
    DirectX::ConstantBuffer<PassConstants> m_Cb0;

    // update outside
    std::shared_ptr<PassConstants> m_Constants = nullptr;
};

