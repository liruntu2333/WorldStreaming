#include "ModelRenderer.h"

#include "AssetImporter.h"
#include "VertexPositionNormalTangentTexture.h"
#include <d3dcompiler.h>
#include <numeric>
#include "SubmeshInstance.h"
#include "AssetLibrary.h"
#include <directxtk/GeometricPrimitive.h>
#include "Material.h"
#include "ObjectInstance.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

constexpr size_t ObjectCapacity   = 1 << 10;
constexpr size_t InstanceCapacity = UINT32_MAX >> 4;

ModelRenderer::ModelRenderer(
	ID3D11Device* device, const std::shared_ptr<GpuConstants>& constants,
	const std::shared_ptr<std::vector<SubmeshInstance>>& subIns,
	const std::shared_ptr<std::vector<ObjectInstance>>& objIns,
	const std::shared_ptr<const AssetLibrary>& assetLibrary) :
	Renderer(device),
	m_Constants(constants),
	m_SubmeshInstances(subIns),
	m_ObjectInstances(objIns),
	m_AssetLibrary(assetLibrary) {}

void ModelRenderer::Initialize(ID3D11DeviceContext* context)
{
	m_Vc0 = std::make_unique<ConstantBuffer<GpuConstants>>(m_Device);

	const auto& subMeshVb = m_AssetLibrary->GetMergedTriangleList();
	auto matBuff          = m_AssetLibrary->GetMaterialBuffer();
	m_Vt0                 = std::make_unique<StructuredBuffer<SubmeshInstance>>(m_Device, InstanceCapacity);
	m_Vt1                 = std::make_unique<StructuredBuffer<ObjectInstance>>(m_Device, ObjectCapacity);
	m_Vt2                 = std::make_unique<StructuredBuffer<Vertex>>(m_Device, subMeshVb.data(), subMeshVb.size());
	m_Pt0                 = std::make_unique<Texture2DArray>(m_Device, context, L"./Asset/Texture");
	m_Pt0->CreateViews(m_Device);
	m_Pt1 = std::make_unique<StructuredBuffer<Material>>(m_Device, matBuff.data(), matBuff.size());

	ComPtr<ID3DBlob> blob;
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

void ModelRenderer::Render(ID3D11DeviceContext* context)
{
	UpdateBuffer(context);

	context->IASetInputLayout(nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	context->VSSetShader(m_Vs.Get(), nullptr, 0);
	context->PSSetShader(m_Ps.Get(), nullptr, 0);

	{
		const auto buffer = m_Vc0->GetBuffer();
		context->VSSetConstantBuffers(0, 1, &buffer);
		ID3D11ShaderResourceView* srv[] = { m_Vt0->GetSrv(), m_Vt1->GetSrv(), m_Vt2->GetSrv() };
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
	context->DrawInstanced(m_Constants->VertexPerMesh, m_SubmeshInstances->size(), 0, 0);
}

void ModelRenderer::UpdateBuffer(ID3D11DeviceContext* context)
{
	m_Vc0->SetData(context, *m_Constants);
	m_Vt0->SetData(context, m_SubmeshInstances->data(), m_SubmeshInstances->size());
	m_Vt1->SetData(context, m_ObjectInstances->data(), m_ObjectInstances->size());
}
