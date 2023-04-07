#include "ModelRenderer.h"

#include "AssetImporter.h"
#include "VertexPositionNormalTangentTexture.h"
#include <d3dcompiler.h>
#include "Instance.h"

#include <directxtk/GeometricPrimitive.h>

using Vertex = VertexPositionNormalTangentTexture;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

constexpr size_t InstanceCapacity = 1 << 13;

ModelRenderer::ModelRenderer(ID3D11Device* device, std::filesystem::path model, 
    std::shared_ptr<Constants> constants, std::shared_ptr<std::vector<Instance>> instances) :
    Renderer(device), m_Constants(std::move(constants)), m_Instances(std::move(instances)), m_Asset(std::move(model))
{
}

void ModelRenderer::Initialize(ID3D11DeviceContext* context)
{
    m_Vc0 = std::make_unique<ConstantBuffer<Constants>>(m_Device);

    auto [mesh, maxLen, bondingRadius] = 
		BuildVertices(L"./Asset/Mesh");
	
    m_Vt0 = std::make_unique<StructuredBuffer<Vertex>>(m_Device, mesh.data(), mesh.size());
	m_Constants->VertexPerMesh = maxLen;
	m_Vt1 = std::make_unique<StructuredBuffer<Instance>>(m_Device, InstanceCapacity);
    m_Pt1 = std::make_unique<Texture2DArray>(m_Device, context, L"./Asset/Texture");
	m_Pt1->CreateViews(m_Device);

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
		ID3D11ShaderResourceView* srv[] = { m_Vt0->GetSrv(), m_Vt1->GetSrv() };
		context->VSSetShaderResources(0, _countof(srv), srv);
	}

	{
		const auto view = m_Pt1->GetSrv();
		context->PSSetShaderResources(0, 1, &view);
		const auto sampler = s_CommonStates->AnisotropicWrap();
		context->PSSetSamplers(0, 1, &sampler);
	}
	const auto opaque = s_CommonStates->Opaque();
	context->OMSetBlendState(opaque, nullptr, 0xffffffff);
	const auto depthTest = s_CommonStates->DepthDefault();
	context->OMSetDepthStencilState(depthTest, 0);

	context->DrawInstanced(m_Constants->VertexPerMesh, m_Instances->size(), 0, 0);

}

void ModelRenderer::UpdateBuffer(ID3D11DeviceContext* context)
{
	m_Vc0->SetData(context, *m_Constants);
	m_Vt1->SetData(context, m_Instances->data(), m_Instances->size());
}

ModelRenderer::MeshData ModelRenderer::BuildVertices(
	std::filesystem::path folder)
{
	std::vector<std::vector<VertexPositionNormalTangentTexture>> vbs;
	std::vector<float> rads;
	for (const auto& entry : std::filesystem::directory_iterator(folder))
	{
		if (entry.is_regular_file())
		{
			auto [mesh, tex, radius] = AssetImporter::LoadTriangleList(entry.path());
			vbs.push_back(mesh);
			rads.push_back(radius);
		}
	}

	const auto maxLen = std::max_element(vbs.begin(), vbs.end(),
		[](const auto& lhs, const auto& rhs) { return lhs.size() < rhs.size(); })->size();

	for (auto & vb : vbs)
	{
		vb.reserve(maxLen);
		for (auto i = vb.size(); i < maxLen; ++i)
		{
			vb.push_back(vb.back());
		}
	}

	std::vector<VertexPositionNormalTangentTexture> result;
	result.reserve(vbs.size() * maxLen);
	for (const auto & vb : vbs)
	{
		std::copy(vb.begin(), vb.end(), std::back_inserter(result));
	}

	return { result, maxLen, rads };
}
