#pragma once

#include <d3d11.h>
#include <wrl/client.h>

namespace DirectX
{
	class Texture2D
	{
	public:
		Texture2D(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& desc);
		Texture2D(ID3D11Device* device, const wchar_t* path);
		virtual ~Texture2D() = default;

		Texture2D(Texture2D&&) = default;
		Texture2D& operator=(Texture2D&&) = default;
		Texture2D(const Texture2D&) = delete;
		Texture2D& operator=(const Texture2D&) = delete;

		[[nodiscard]] auto GetTexture() const { return m_Texture.Get(); }
		explicit operator ID3D11Texture2D*() const { return m_Texture.Get(); }
		//explicit operator ID3D11Resource*() const { return m_Texture.Get(); }
		[[nodiscard]] auto GetSrv() const { return m_Srv.Get(); }
		[[nodiscard]] auto GetRtv() const { return m_Rtv.Get(); }
		[[nodiscard]] auto GetUav() const { return m_Uav.Get(); }
		[[nodiscard]] auto GetDsv() const { return m_Dsv.Get(); }
		[[nodiscard]] const auto& GetDesc() const { return m_Desc; }

		virtual void CreateViews(ID3D11Device* device);
		void Load(ID3D11Device* device, const wchar_t* path);

	protected:
		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_Texture = nullptr;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_Srv = nullptr;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_Rtv = nullptr;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_Uav = nullptr;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_Dsv = nullptr;

		D3D11_TEXTURE2D_DESC m_Desc{};
	};
}


