#pragma once

#include <filesystem>
#include <directxtk/BufferHelpers.h>
#include "StructuredBuffer.h"

#include "GpuConstants.h"
#include "Renderer.h"
#include "Texture2D.h"

struct ObjectInstance;
struct Material;
struct VertexPositionNormalTangentTexture;
struct SubmeshInstance;
class AssetLibrary;

class ModelRenderer : public Renderer
{
public:
	ModelRenderer(
		ID3D11Device* device, const std::shared_ptr<GpuConstants>& constants,
		const std::shared_ptr<std::vector<SubmeshInstance>>& subIns,
		const std::shared_ptr<std::vector<ObjectInstance>>& objIns,
		const std::shared_ptr<const AssetLibrary>& assetLibrary);
	~ModelRenderer() override = default;

	void Initialize(ID3D11DeviceContext* context) override;
	void Render(ID3D11DeviceContext* context) override;

private:
	void UpdateBuffer(ID3D11DeviceContext* context) override;

	using Vertex = VertexPositionNormalTangentTexture;
	std::unique_ptr<DirectX::ConstantBuffer<GpuConstants>> m_Vc0      = nullptr;
	std::unique_ptr<DirectX::StructuredBuffer<SubmeshInstance>> m_Vt0 = nullptr;
	std::unique_ptr<DirectX::StructuredBuffer<ObjectInstance>> m_Vt1  = nullptr;
	std::unique_ptr<DirectX::StructuredBuffer<Vertex>> m_Vt2          = nullptr;
	std::unique_ptr<DirectX::Texture2D> m_Pt0                         = nullptr;
	std::unique_ptr<DirectX::StructuredBuffer<Material>> m_Pt1        = nullptr;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_Vs = nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_Ps  = nullptr;
	//Microsoft::WRL::ComPtr<ID3D11InputLayout> m_Layout = nullptr;
	//Microsoft::WRL::ComPtr<ID3D11Buffer> m_Ins;

	std::shared_ptr<GpuConstants> m_Constants                        = nullptr;
	std::shared_ptr<std::vector<SubmeshInstance>> m_SubmeshInstances = nullptr;
	std::shared_ptr<std::vector<ObjectInstance>> m_ObjectInstances   = nullptr;
	std::shared_ptr<const AssetLibrary> m_AssetLibrary               = nullptr;
};
