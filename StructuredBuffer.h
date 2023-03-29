#pragma once
#include <cassert>
#include <cstring>
#include <d3d11.h>

#include "D3DHelper.h"

namespace DirectX
{
	template <typename T>
	class StructuredBuffer
	{
	public:
		explicit StructuredBuffer(ID3D11Device* device, UINT elementCount);

		~StructuredBuffer() = default;

		StructuredBuffer(const StructuredBuffer&) = delete;
		StructuredBuffer(StructuredBuffer&&) = default;
		StructuredBuffer& operator=(const StructuredBuffer&) = delete;
		StructuredBuffer& operator=(StructuredBuffer&&) = default;

		explicit operator ID3D11Buffer*() const { return m_Buffer.Get(); }
		auto GetSrv() const { return m_Srv.Get(); }

		void SetData(ID3D11DeviceContext* context, const T* buffer, UINT size);

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_Buffer = nullptr;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_Srv = nullptr;
		//Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_Uav = nullptr;
		//ID3D11DeviceContext* m_Context;
		const UINT m_Capacity;
	};

	template <typename T>
	StructuredBuffer<T>::StructuredBuffer(ID3D11Device* device, UINT elementCount) :
		m_Capacity(elementCount)
	{
		CD3D11_BUFFER_DESC buff(m_Capacity * sizeof(T), D3D11_BIND_SHADER_RESOURCE,
		                        D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE,
		                        D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, sizeof(T));
		auto hr = device->CreateBuffer(&buff, nullptr, &m_Buffer);
		ThrowIfFailed(hr);

		CD3D11_SHADER_RESOURCE_VIEW_DESC srv(m_Buffer.Get(), DXGI_FORMAT_UNKNOWN,
		                                     0, m_Capacity, 0);
		hr = device->CreateShaderResourceView(m_Buffer.Get(), &srv, &m_Srv);
		ThrowIfFailed(hr);
	}

	template <typename T>
	void StructuredBuffer<T>::SetData(ID3D11DeviceContext* context, const T* buffer, UINT size)
	{
		assert(m_Buffer && size <= m_Capacity);
		D3D11_MAPPED_SUBRESOURCE mapped;
		context->Map(m_Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		std::memcpy(mapped.pData, buffer, size * sizeof(T));
		context->Unmap(m_Buffer.Get(), 0);
	}
}
