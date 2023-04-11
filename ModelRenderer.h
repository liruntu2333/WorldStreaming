#pragma once

#include <filesystem>
#include <directxtk/BufferHelpers.h>
#include "StructuredBuffer.h"

#include "Constants.h"
#include "Renderer.h"
#include "Texture2D.h"

struct VertexPositionNormalTangentTexture;
struct Instance;

class ModelRenderer : public Renderer
{
public:
    ModelRenderer(ID3D11Device* device, std::filesystem::path model, std::shared_ptr<Constants> constants,
                  std::shared_ptr<std::vector<Instance>> instances);
    ~ModelRenderer() override = default;

    void Initialize(ID3D11DeviceContext* context) override;
    void Render(ID3D11DeviceContext* context) override;

private:
    void UpdateBuffer(ID3D11DeviceContext* context) override;

    using Vertex = VertexPositionNormalTangentTexture;
    using MeshLib = std::vector<std::vector<Vertex>>;
    using TexLib = std::vector<std::filesystem::path>;
    using MeshStats = std::vector<uint32_t>;
    static std::pair<MeshLib, TexLib> LoadAssets(const std::filesystem::path& dir);
    std::unique_ptr<DirectX::StructuredBuffer<Vertex>> MergeVert(ID3D11DeviceContext* context, MeshStats& stats);

    std::unique_ptr<DirectX::ConstantBuffer<Constants>> m_Vc0 = nullptr;
    std::unique_ptr<DirectX::StructuredBuffer<Vertex>> m_Vt0 = nullptr;
    std::unique_ptr<DirectX::StructuredBuffer<Instance>> m_Vt1 = nullptr;
    std::unique_ptr<DirectX::Texture2D> m_Pt1 = nullptr;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_Vs = nullptr;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_Ps = nullptr;
    //Microsoft::WRL::ComPtr<ID3D11InputLayout> m_Layout = nullptr;
    //Microsoft::WRL::ComPtr<ID3D11Buffer> m_Ins;

    std::shared_ptr<Constants> m_Constants = nullptr;
    std::shared_ptr<std::vector<Instance>> m_Instances = nullptr;
    std::filesystem::path m_AssetDir{};
    MeshLib m_MeshLib{};
    MeshStats m_MeshStats{};
    TexLib m_TexLib{};
};
