#include "MergedModelRenderer.h"
#include <d3dcompiler.h>

#include "GlobalContext.h"
#include "MergedSubmeshInstance.h"
#include "SubmeshInstance.h"

using namespace DirectX;

MergedModelRenderer::MergedModelRenderer(
	ID3D11Device* device, const std::shared_ptr<GpuConstants>& constants,
	const std::shared_ptr<std::vector<SubmeshInstance>>& subIns,
	const std::shared_ptr<std::vector<ObjectInstance>>& objIns, const std::shared_ptr<const AssetLibrary>& assetLibrary,
	const std::shared_ptr<std::vector<MergedSubmeshInstance>>& mergedSubmeshInstances) :
	ModelRenderer(device, constants, subIns, objIns, assetLibrary), m_MergedSubmeshInstances(mergedSubmeshInstances)
{}

void MergedModelRenderer::Initialize(ID3D11DeviceContext* context)
{
	ModelRenderer::Initialize(context);

	m_Vt00 = std::make_unique<StructuredBuffer<MergedSubmeshInstance>>(m_Device, INSTANCE_MAX);

	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	ThrowIfFailed(D3DReadFileToBlob(L"./shader/SimpleMergedVS.cso", &blob));
	const auto hr = m_Device->CreateVertexShader(blob->GetBufferPointer(),
		blob->GetBufferSize(), nullptr, &m_Vs1);
	ThrowIfFailed(hr);
}

void MergedModelRenderer::Render(ID3D11DeviceContext* context)
{
	context->IASetInputLayout(nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	context->VSSetShader(m_Vs1.Get(), nullptr, 0);
	context->PSSetShader(m_Ps.Get(), nullptr, 0);

	{
		const auto buffer = m_Vc0->GetBuffer();
		context->VSSetConstantBuffers(0, 1, &buffer);
		ID3D11ShaderResourceView* srv[] = { m_Vt00->GetSrv(), m_Vt2->GetSrv() };
		context->VSSetShaderResources(0, _countof(srv), srv);
	}

	{
		ID3D11ShaderResourceView* srv[] = { m_Pt0->GetSrv(), m_Pt1->GetSrv() };
		context->PSSetShaderResources(0, _countof(srv), srv);
		const auto sampler = s_CommonStates->AnisotropicWrap();
		context->PSSetSamplers(0, 1, &sampler);
	}
	const auto opaque = s_CommonStates->Opaque();
	context->OMSetBlendState(opaque, nullptr, 0xffffffff);
	const auto depthTest = s_CommonStates->DepthDefault();
	context->OMSetDepthStencilState(depthTest, 0);
	context->RSSetState(s_CommonStates->CullCounterClockwise());
	context->DrawInstanced(m_Constants->VertexPerMesh, m_MergedSubmeshInstances->size(), 0, 0);
}

void MergedModelRenderer::UpdateBuffer(ID3D11DeviceContext* context)
{
	m_Vc0->SetData(context, *m_Constants);
	m_Vt00->SetData(context, m_MergedSubmeshInstances->data(), m_MergedSubmeshInstances->size());
}
