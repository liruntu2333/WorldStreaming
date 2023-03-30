#pragma once

#include <filesystem>
#include <directxtk/BufferHelpers.h>
#include "StructuredBuffer.h"

#include "PassConstants.h"
#include "Renderer.h"
#include "Texture2D.h"

struct VertexPositionNormalTangentTexture;

class ModelRenderer : public Renderer
{
public:
    ModelRenderer(ID3D11Device* device, std::filesystem::path model, std::shared_ptr<PassConstants> constants);
    ~ModelRenderer() override = default;

    void Initialize(ID3D11DeviceContext* context) override;
    void Render(ID3D11DeviceContext* context) override;

private:
    void UpdateBuffer(ID3D11DeviceContext* context) override;

    std::unique_ptr<DirectX::ConstantBuffer<PassConstants>> m_Vc0 = nullptr;
    std::unique_ptr<DirectX::StructuredBuffer<VertexPositionNormalTangentTexture>> m_Vt0 = nullptr;
    std::unique_ptr<DirectX::Texture2D> m_Pt1 = nullptr;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_Vs = nullptr;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_Ps = nullptr;
    //Microsoft::WRL::ComPtr<ID3D11InputLayout> m_Layout = nullptr;
    //Microsoft::WRL::ComPtr<ID3D11Buffer> m_Ins;

    std::shared_ptr<PassConstants> m_Constants = nullptr;
    std::filesystem::path m_Asset{};
};

