#pragma once
#include "AssetLibrary.h"
#include "ModelRenderer1.h"

class ModelRenderer2 : public ModelRenderer1
{
public:
	ModelRenderer2(
		ID3D11Device* device, const std::shared_ptr<GpuConstants>& constants,
		const std::shared_ptr<std::vector<SubmeshInstance>>& subIns,
		const std::shared_ptr<std::vector<ObjectInstance>>& objIns,
		const std::shared_ptr<const AssetLibrary>& assetLibrary,
		const std::shared_ptr<std::vector<MergedSubmeshInstance>>& mergedSubmeshInstances,
		const std::shared_ptr<std::vector<DividedSubmeshInstance>>& divideSmIns);
	~ModelRenderer2() override = default;

	void Initialize(ID3D11DeviceContext* context) override;
	void Render(ID3D11DeviceContext* context) override;
	void UpdateBuffer(ID3D11DeviceContext* context) override;

protected:

	std::unordered_map<uint32_t, std::unique_ptr<DirectX::StructuredBuffer<Vertex>>> m_Vt2s = {};
	std::shared_ptr<std::vector<DividedSubmeshInstance>> m_DividedSubmeshInstances = nullptr;
};
