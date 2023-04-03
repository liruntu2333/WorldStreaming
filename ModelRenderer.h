#pragma once

#include <filesystem>
#include <directxtk/BufferHelpers.h>
#include "StructuredBuffer.h"

#include "Constants.h"
#include "Renderer.h"
#include "Texture2D.h"

struct VertexPositionNormalTangentTexture;
struct InstanceData;

class ModelRenderer : public Renderer
{
public:
    ModelRenderer(ID3D11Device* device, std::filesystem::path model, std::shared_ptr<Constants> constants, std::shared_ptr<std::vector<InstanceData>> instances);
    ~ModelRenderer() override = default;

    std::vector<float> Initialize(ID3D11DeviceContext* context) override;
    void Render(ID3D11DeviceContext* context) override;

private:
    void UpdateBuffer(ID3D11DeviceContext* context) override;
    using MeshData = std::tuple<std::vector<VertexPositionNormalTangentTexture>, size_t, std::vector<float>>;
    static MeshData BuildVertices(
        std::filesystem::path folder);

    std::unique_ptr<DirectX::ConstantBuffer<Constants>> m_Vc0 = nullptr;
    std::unique_ptr<DirectX::StructuredBuffer<VertexPositionNormalTangentTexture>> m_Vt0 = nullptr;
    std::unique_ptr<DirectX::StructuredBuffer<InstanceData>> m_Vt1 = nullptr;
    std::unique_ptr<DirectX::Texture2D> m_Pt1 = nullptr;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_Vs = nullptr;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_Ps = nullptr;
    //Microsoft::WRL::ComPtr<ID3D11InputLayout> m_Layout = nullptr;
    //Microsoft::WRL::ComPtr<ID3D11Buffer> m_Ins;

    std::shared_ptr<Constants> m_Constants = nullptr;
    std::shared_ptr<std::vector<InstanceData>> m_Instances = nullptr;
    std::filesystem::path m_Asset{};
};

