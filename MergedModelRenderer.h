#pragma once
#include "ModelRenderer.h"

struct MergedSubmeshInstance;

class MergedModelRenderer : public ModelRenderer
{
public:
	MergedModelRenderer(
		ID3D11Device* device, const std::shared_ptr<GpuConstants>& constants,
		const std::shared_ptr<std::vector<SubmeshInstance>>& subIns,
		const std::shared_ptr<std::vector<ObjectInstance>>& objIns,
		const std::shared_ptr<const AssetLibrary>& assetLibrary,
		const std::shared_ptr<std::vector<MergedSubmeshInstance>>& mergedSubmeshInstances);
	~MergedModelRenderer() override = default;

	void Initialize(ID3D11DeviceContext* context) override;
	void Render(ID3D11DeviceContext* context) override;
	void UpdateBuffer(ID3D11DeviceContext* context) override;

protected:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_Vs1 = nullptr;
	std::unique_ptr<DirectX::StructuredBuffer<MergedSubmeshInstance>> m_Vt00 = nullptr;
	std::shared_ptr<std::vector<MergedSubmeshInstance>> m_MergedSubmeshInstances = nullptr;
};
