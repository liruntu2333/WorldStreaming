#include "Texture2D.h"

#include <cassert>

#include "D3DHelper.h"
#include <directxtk/WICTextureLoader.h>

using namespace DirectX;

Texture2D::Texture2D(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& desc) : m_Desc(desc)
{
	const auto hr = device->CreateTexture2D(&m_Desc, nullptr, &m_Texture);
	ThrowIfFailed(hr);
}

Texture2D::Texture2D(ID3D11Device* device, const wchar_t* path)
{
	const auto hr = CreateWICTextureFromFile(device, path, 
		reinterpret_cast<ID3D11Resource**>(m_Texture.GetAddressOf()), m_Srv.GetAddressOf());

	ThrowIfFailed(hr);

	m_Texture->GetDesc(&m_Desc);
}

void Texture2D::CreateViews(ID3D11Device* device)
{
	D3D11_TEXTURE2D_DESC desc;
	m_Texture->GetDesc(&desc);
	auto bindFlag = desc.BindFlags;
	bool isMultiSample = desc.SampleDesc.Count > 1;

	if (m_Srv == nullptr && bindFlag & D3D11_BIND_SHADER_RESOURCE)
	{
		auto srv = CD3D11_SHADER_RESOURCE_VIEW_DESC(m_Texture.Get(),
			isMultiSample
			? D3D11_SRV_DIMENSION_TEXTURE2DMS
			: D3D11_SRV_DIMENSION_TEXTURE2D);
		auto hr = device->CreateShaderResourceView(m_Texture.Get(), &srv, &m_Srv);
		ThrowIfFailed(hr);
	}

	if (m_Rtv == nullptr && bindFlag & D3D11_BIND_RENDER_TARGET)
	{
		auto rtv = CD3D11_RENDER_TARGET_VIEW_DESC(m_Texture.Get(),
			isMultiSample
			? D3D11_RTV_DIMENSION_TEXTURE2DMS
			: D3D11_RTV_DIMENSION_TEXTURE2D);
		auto hr = device->CreateRenderTargetView(m_Texture.Get(), &rtv, &m_Rtv);
		ThrowIfFailed(hr);
	}

	if (m_Uav == nullptr && bindFlag & D3D11_BIND_UNORDERED_ACCESS)
	{
		assert(!isMultiSample); // MSAA resource shouldn't be multi-sampled.
		auto uav = CD3D11_UNORDERED_ACCESS_VIEW_DESC(m_Texture.Get(),
			D3D11_UAV_DIMENSION_TEXTURE2D);
		auto hr = device->CreateUnorderedAccessView(m_Texture.Get(), &uav, &m_Uav);
		ThrowIfFailed(hr);
	}

	if (m_Dsv == nullptr && bindFlag & D3D11_BIND_DEPTH_STENCIL)
	{
		auto dsv = CD3D11_DEPTH_STENCIL_VIEW_DESC(m_Texture.Get(),
			isMultiSample
			? D3D11_DSV_DIMENSION_TEXTURE2DMS
			: D3D11_DSV_DIMENSION_TEXTURE2D);
		auto hr = device->CreateDepthStencilView(m_Texture.Get(), &dsv, &m_Dsv);
		ThrowIfFailed(hr);
	}
}

void Texture2D::Load(ID3D11Device* device, const wchar_t* path)
{
	const auto hr = CreateWICTextureFromFile(device, path, 
		reinterpret_cast<ID3D11Resource**>(m_Texture.ReleaseAndGetAddressOf()), 
		m_Srv.ReleaseAndGetAddressOf());

	ThrowIfFailed(hr);

	m_Texture->GetDesc(&m_Desc);
}
