#include "PlaneRenderer.h"
#include "D3DHelper.h"
#include "d3dcompiler.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

PlaneRenderer::PlaneRenderer(ID3D11Device* device, std::shared_ptr<PassConstants> constants): Renderer(device),
	m_Constants(constants), m_Cb0(device)
{
}

void PlaneRenderer::Initialize(ID3D11DeviceContext* context)
{
	ComPtr<ID3DBlob> blob;
	ThrowIfFailed(D3DReadFileToBlob(L"./shader/DummyVS.cso", &blob));
	auto hr = m_Device->CreateVertexShader(blob->GetBufferPointer(),
		blob->GetBufferSize(), nullptr, &m_Vs);
	ThrowIfFailed(hr);

	blob->Release();
	ThrowIfFailed(D3DReadFileToBlob(L"./shader/DummyPS.cso", &blob));
	hr = m_Device->CreatePixelShader(blob->GetBufferPointer(),
		blob->GetBufferSize(), nullptr, &m_Ps);
	ThrowIfFailed(hr);
}

void PlaneRenderer::Render(ID3D11DeviceContext* context)
{
	UpdateBuffer(context);

	context->IASetInputLayout(nullptr);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	const auto cb0 = m_Cb0.GetBuffer();
	context->VSSetConstantBuffers(0, 1, &cb0);
	const auto opaque = s_CommonStates->Opaque();
	context->OMSetBlendState(opaque, nullptr, 0xffffffff);
	// right handed
	const auto depthTest = s_CommonStates->DepthDefault();
	context->OMSetDepthStencilState(depthTest, 0);
	context->RSSetState(s_CommonStates->CullCounterClockwise());
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->VSSetShader(m_Vs.Get(), nullptr, 0);
	context->PSSetShader(m_Ps.Get(), nullptr, 0);
	context->Draw(6, 0);
}

void PlaneRenderer::UpdateBuffer(ID3D11DeviceContext* context)
{
	m_Cb0.SetData(context, *m_Constants);
}
