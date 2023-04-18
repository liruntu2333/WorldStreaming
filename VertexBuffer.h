﻿#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "D3DHelper.h"
#include <algorithm>

namespace DirectX
{
	template <typename T>
	class VertexBuffer
	{
	public:
		VertexBuffer(ID3D11Device* device, UINT elementCount);
		VertexBuffer(ID3D11Device* device, const T* buffer, UINT size); // static version

		~VertexBuffer() = default;

		VertexBuffer(const VertexBuffer&) = delete;
		VertexBuffer(VertexBuffer&&) = default;
		VertexBuffer& operator=(const VertexBuffer&) = delete;
		VertexBuffer& operator=(VertexBuffer&&) = default;

		explicit operator ID3D11Buffer*() const { return m_Buffer.Get(); }

		void SetData(ID3D11DeviceContext* context, const T* buffer, UINT size);

		const UINT m_Capacity;

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_Buffer = nullptr;
	};

	template <typename T>
	VertexBuffer<T>::VertexBuffer(ID3D11Device* device, UINT elementCount) : m_Capacity(elementCount)
	{
		const CD3D11_BUFFER_DESC desc(m_Capacity * sizeof(T), D3D11_BIND_VERTEX_BUFFER,
			D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		ThrowIfFailed(device->CreateBuffer(&desc, nullptr, &m_Buffer));
	}

	template <typename T>
	VertexBuffer<T>::VertexBuffer(ID3D11Device* device, const T* buffer, UINT size) : m_Capacity(size)
	{
		const CD3D11_BUFFER_DESC desc(m_Capacity * sizeof(T), D3D11_BIND_VERTEX_BUFFER);
		const D3D11_SUBRESOURCE_DATA data = { buffer, 0, 0 };
		ThrowIfFailed(device->CreateBuffer(&desc, &data, &m_Buffer));
	}

	template <typename T>
	void VertexBuffer<T>::SetData(ID3D11DeviceContext* context, const T* buffer, UINT size)
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		ThrowIfFailed(context->Map(m_Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 
			0, &mapped));
		std::memcpy(mapped.pData, buffer, size * sizeof(T));
		context->Unmap(m_Buffer.Get(), 0);
	}
}
