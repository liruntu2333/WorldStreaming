#pragma once
#include "InstancingRenderer1.h"

class InstancingRenderer2 : public InstancingRenderer1
{
public:
	InstancingRenderer2(
		ID3D11Device* device, const std::shared_ptr<GpuConstants>& constants,
		const std::shared_ptr<std::vector<SubmeshInstance>>& subIns,
		const std::shared_ptr<std::vector<ObjectInstance>>& objIns,
		const std::shared_ptr<const AssetLibrary>& assetLibrary,
		const std::shared_ptr<std::vector<DividedSubmeshInstance>>& divideSmIns);

	~InstancingRenderer2() override = default;

	void Initialize(ID3D11DeviceContext* context) override;
	void Render(ID3D11DeviceContext* context) override;
	void UpdateBuffer(ID3D11DeviceContext* context) override;

protected:
	std::unordered_map<uint32_t, std::unique_ptr<DirectX::Texture2D>> m_Vt2Compressed {};
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_VsCompressed = nullptr;
};