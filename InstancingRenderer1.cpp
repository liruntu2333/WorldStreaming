#include "InstancingRenderer1.h"
#include <d3dcompiler.h>
#include "Texture2D.h"
#include "GpuConstants.h"

using namespace DirectX;

InstancingRenderer1::InstancingRenderer1(
	ID3D11Device* device, const std::shared_ptr<GpuConstants>& constants,
	const std::shared_ptr<std::vector<SubmeshInstance>>& subIns,
	const std::shared_ptr<std::vector<ObjectInstance>>& objIns, const std::shared_ptr<const AssetLibrary>& assetLibrary,
	const std::shared_ptr<std::vector<DividedSubmeshInstance>>& divideSmIns) :
	InstancingRenderer(device, constants, subIns, objIns, assetLibrary, divideSmIns) {}

void InstancingRenderer1::Initialize(ID3D11DeviceContext* context)
{
	InstancingRenderer::Initialize(context);

	m_Vb = std::make_unique<VertexBuffer<SubmeshInstance>>(m_Device, INSTANCE_MAX);

	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	ThrowIfFailed(D3DReadFileToBlob(L"./shader/SimpleIaVS.cso", &blob));
	ThrowIfFailed(m_Device->CreateVertexShader(blob->GetBufferPointer(),
		blob->GetBufferSize(), nullptr, &m_VsIa));

	D3D11_INPUT_ELEMENT_DESC elements[] = {
		{ "SubsetID", 0, DXGI_FORMAT_R32_UINT, 0, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "MaterialID", 0, DXGI_FORMAT_R32_UINT, 0, 4, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "ObjectID", 0, DXGI_FORMAT_R32_UINT, 0, 8, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};

	ThrowIfFailed(m_Device->CreateInputLayout(elements, _countof(elements), blob->GetBufferPointer(),
		blob->GetBufferSize(),
		&m_Input));
}

void InstancingRenderer1::Render(ID3D11DeviceContext* context)
{
	context->IASetInputLayout(m_Input.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	context->VSSetShader(m_VsIa.Get(), nullptr, 0);
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
		m_Vb->SetData(context, insBuff.data(), insBuff.size());

		const auto vb = static_cast<ID3D11Buffer*>(*m_Vb);
		constexpr UINT stride = sizeof(SubmeshInstance), offset = 0;
		context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

		ID3D11ShaderResourceView* srv = m_Vt2[faceStride]->GetSrv();
		context->VSSetShaderResources(0, 1, &srv);

		context->DrawInstanced(m_Constants->VertexPerMesh, insBuff.size(), 0, 0);
	}
}

void InstancingRenderer1::UpdateBuffer(ID3D11DeviceContext* context)
{
	InstancingRenderer::UpdateBuffer(context);
}
