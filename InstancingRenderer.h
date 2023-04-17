#pragma once
#include <directxtk/BufferHelpers.h>

#include "AssetLibrary.h"
#include "GlobalContext.h"
#include "Renderer.h"
#include "StructuredBuffer.h"

namespace DirectX {
	class Texture2D;
}

struct ObjectInstance;
constexpr uint32_t INSTANCE_MAX = OBJECT_MAX << 5;

class InstancingRenderer : public Renderer
{
public:
	InstancingRenderer(
		ID3D11Device* device, const std::shared_ptr<GpuConstants>& constants,
		const std::shared_ptr<std::vector<SubmeshInstance>>& subIns,
		const std::shared_ptr<std::vector<ObjectInstance>>& objIns,
		const std::shared_ptr<const AssetLibrary>& assetLibrary,
		const std::shared_ptr<std::vector<DividedSubmeshInstance>>& divideSmIns);
	~InstancingRenderer() override = default;

	void Initialize(ID3D11DeviceContext* context) override;
	void Render(ID3D11DeviceContext* context) override;
	void UpdateBuffer(ID3D11DeviceContext* context) override;

protected:
	using Vertex = VertexPositionNormalTangentTexture;
	std::unique_ptr<DirectX::ConstantBuffer<GpuConstants>> m_Vc0 = nullptr;

	std::unique_ptr<DirectX::StructuredBuffer<SubmeshInstance>> m_Vt0 = nullptr;
	std::unique_ptr<DirectX::StructuredBuffer<ObjectInstance>> m_Vt1 = nullptr;
	std::unordered_map<uint32_t, std::unique_ptr<DirectX::StructuredBuffer<Vertex>>> m_Vt2 = {};

	std::unique_ptr<DirectX::Texture2D> m_Pt0 = nullptr;
	std::unique_ptr<DirectX::StructuredBuffer<Material>> m_Pt1 = nullptr;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_Vs = nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_Ps = nullptr;
	//Microsoft::WRL::ComPtr<ID3D11InputLayout> m_Layout = nullptr;
	//Microsoft::WRL::ComPtr<ID3D11Buffer> m_Ins;

	std::shared_ptr<GpuConstants> m_Constants = nullptr;
	std::shared_ptr<std::vector<SubmeshInstance>> m_SubmeshInstances = nullptr;
	std::shared_ptr<std::vector<DividedSubmeshInstance>> m_DividedSubmeshInstances = nullptr;
	std::shared_ptr<std::vector<ObjectInstance>> m_ObjectInstances = nullptr;
	std::shared_ptr<const AssetLibrary> m_AssetLibrary = nullptr;
};
