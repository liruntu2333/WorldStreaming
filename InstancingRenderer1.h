#pragma once
#include "InstancingRenderer.h"
#include "SubmeshInstance.h"
#include "VertexBuffer.h"

class InstancingRenderer1 : public InstancingRenderer
{
public:
	InstancingRenderer1(
		ID3D11Device* device, const std::shared_ptr<GpuConstants>& constants,
		const std::shared_ptr<std::vector<SubmeshInstance>>& subIns,
		const std::shared_ptr<std::vector<ObjectInstance>>& objIns,
		const std::shared_ptr<const AssetLibrary>& assetLibrary,
		const std::shared_ptr<std::vector<DividedSubmeshInstance>>& divideSmIns);
	~InstancingRenderer1() override = default;

	void Initialize(ID3D11DeviceContext* context) override;
	void Render(ID3D11DeviceContext* context) override;
	void UpdateBuffer(ID3D11DeviceContext* context) override;

protected:

	std::unique_ptr<DirectX::VertexBuffer<SubmeshInstance>> m_Vb = nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_Input = nullptr;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_VsIa = nullptr;
};

