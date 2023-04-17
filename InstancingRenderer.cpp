#include "InstancingRenderer.h"

#include <d3dcompiler.h>

#include "ObjectInstance.h"
#include "Texture2D.h"
#include "GpuConstants.h"

using namespace DirectX;

InstancingRenderer::InstancingRenderer(
	ID3D11Device* device, const std::shared_ptr<GpuConstants>& constants,
	const std::shared_ptr<std::vector<SubmeshInstance>>& subIns,
	const std::shared_ptr<std::vector<ObjectInstance>>& objIns, const std::shared_ptr<const AssetLibrary>& assetLibrary,
	const std::shared_ptr<std::vector<DividedSubmeshInstance>>& divideSmIns) :
	Renderer(device), m_Constants(constants), m_SubmeshInstances(subIns), m_DividedSubmeshInstances(divideSmIns),
	m_ObjectInstances(objIns), m_AssetLibrary(assetLibrary) {}

void InstancingRenderer::Initialize(ID3D11DeviceContext* context)
{
	m_Vc0 = std::make_unique<ConstantBuffer<GpuConstants>>(m_Device);

	auto matBuff = m_AssetLibrary->GetMaterialList();
	m_Vt0 = std::make_unique<StructuredBuffer<SubmeshInstance>>(m_Device, INSTANCE_MAX);
	m_Vt1 = std::make_unique<StructuredBuffer<ObjectInstance>>(m_Device, OBJECT_MAX);

	const auto& dividedTriList = m_AssetLibrary->GetMergedTriangleListDivide();
	for (const auto& [faceStride, triList] : dividedTriList)
		m_Vt2[faceStride] = std::make_unique<StructuredBuffer<Vertex>>(m_Device, triList.data(), triList.size());

	m_Pt0 = std::make_unique<Texture2DArray>(m_Device, context, L"./Asset/Texture");
	m_Pt0->CreateViews(m_Device);
	m_Pt1 = std::make_unique<StructuredBuffer<Material>>(m_Device, matBuff.data(), matBuff.size());

	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	ThrowIfFailed(D3DReadFileToBlob(L"./shader/SimpleVS.cso", &blob));
	auto hr = m_Device->CreateVertexShader(blob->GetBufferPointer(),
		blob->GetBufferSize(), nullptr, &m_Vs);
	ThrowIfFailed(hr);

	blob->Release();
	ThrowIfFailed(D3DReadFileToBlob(L"./shader/SimplePS.cso", &blob));
	hr = m_Device->CreatePixelShader(blob->GetBufferPointer(),
		blob->GetBufferSize(), nullptr, &m_Ps);
	ThrowIfFailed(hr);
}

void InstancingRenderer::Render(ID3D11DeviceContext* context)
{
	context->IASetInputLayout(nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	context->VSSetShader(m_Vs.Get(), nullptr, 0);
	context->PSSetShader(m_Ps.Get(), nullptr, 0);

	{
		const auto buffer = m_Vc0->GetBuffer();
		context->VSSetConstantBuffers(0, 1, &buffer);
		ID3D11ShaderResourceView* srv[] = { m_Vt1->GetSrv() };
		context->VSSetShaderResources(1, _countof(srv), srv);
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

	for (const auto& [faceStride, insBuff] : *m_DividedSubmeshInstances)
	{
		m_Constants->VertexPerMesh = faceStride * 3;
		m_Vc0->SetData(context, *m_Constants);
		m_Vt0->SetData(context, insBuff.data(), insBuff.size());

		ID3D11ShaderResourceView* srv = m_Vt2[faceStride]->GetSrv();
		context->VSSetShaderResources(2, 1, &srv);
		srv = m_Vt0->GetSrv();
		context->VSSetShaderResources(0, 1, &srv);

		context->DrawInstanced(m_Constants->VertexPerMesh, insBuff.size(), 0, 0);
	}
}

void InstancingRenderer::UpdateBuffer(ID3D11DeviceContext* context)
{
	m_Vt1->SetData(context, m_ObjectInstances->data(), m_ObjectInstances->size());
}
