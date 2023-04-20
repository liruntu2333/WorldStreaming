#include "InstancingRenderer2.h"

#include <d3dcompiler.h>
#include "Texture2D.h"
#include "GpuConstants.h"
#include <DirectXPackedVector.h>
#include <numeric>

using namespace DirectX;
using namespace SimpleMath;

constexpr UINT VERTEX_TEXTURE_WIDTH = 512;

InstancingRenderer2::InstancingRenderer2(
	ID3D11Device* device, const std::shared_ptr<GpuConstants>& constants,
	const std::shared_ptr<std::vector<SubmeshInstance>>& subIns,
	const std::shared_ptr<std::vector<ObjectInstance>>& objIns, const std::shared_ptr<const AssetLibrary>& assetLibrary,
	const std::shared_ptr<std::vector<DividedSubmeshInstance>>& divideSmIns) :
	InstancingRenderer1(device, constants, subIns, objIns, assetLibrary, divideSmIns) {}

void InstancingRenderer2::Initialize(ID3D11DeviceContext* context)
{
	InstancingRenderer1::Initialize(context);

	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	ThrowIfFailed(D3DReadFileToBlob(L"./shader/SimpleCompressedVS.cso", &blob));
	ThrowIfFailed(m_Device->CreateVertexShader(blob->GetBufferPointer(),
		blob->GetBufferSize(), nullptr, &m_VsC16B));

	const auto& divided = m_AssetLibrary->GetMergedTriangleListDivide();
	for (const auto& [faceStride, vts] : divided)
	{
		std::vector<XMUINT4> compressed;
		compressed.reserve(vts.size());
		for (const auto& vtx : vts)
		{
			auto compressVec = [](const Vector3& v)
			{
				Vector3 vc = (v * 512.0f + Vector3(512.0f));
				vc.Clamp(Vector3(0.0f), Vector3(1023.0f), vc);
				const uint32_t x = static_cast<uint32_t>(vc.x);
				const uint32_t y = static_cast<uint32_t>(vc.y);
				const uint32_t z = static_cast<uint32_t>(vc.z);
				return (x << 20) | (y << 10) | z;
			};

			auto compressUv = [](const Vector2& v)
			{
				const PackedVector::XMHALF2 half2(v.x, v.y);
				return half2.v;
			};

			compressed.emplace_back(
				compressVec(vtx.Pos), compressVec(vtx.Nor), compressVec(vtx.Tan), compressUv(vtx.Tc));
		}

		const UINT height = (compressed.size() + VERTEX_TEXTURE_WIDTH - 1) / VERTEX_TEXTURE_WIDTH;
		compressed.resize(height * VERTEX_TEXTURE_WIDTH, XMUINT4(0, 0, 0, 0));

		CD3D11_TEXTURE2D_DESC vtxTex(DXGI_FORMAT_R32G32B32A32_UINT, VERTEX_TEXTURE_WIDTH, height, 1,
			1, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE);
		m_Vt2C16B[faceStride] = std::make_unique<StructuredBuffer<XMUINT4>>(m_Device, compressed.data(),
			compressed.size());
	}
}

void InstancingRenderer2::Render(ID3D11DeviceContext* context)
{
	context->IASetInputLayout(m_Input.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	context->VSSetShader(m_VsC16B.Get(), nullptr, 0);
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

		ID3D11ShaderResourceView* srv = m_Vt2C16B[faceStride]->GetSrv();
		context->VSSetShaderResources(0, 1, &srv);

		context->DrawInstanced(m_Constants->VertexPerMesh, insBuff.size(), 0, 0);
	}
}

void InstancingRenderer2::UpdateBuffer(ID3D11DeviceContext* context)
{
	InstancingRenderer1::UpdateBuffer(context);
}

uint32_t InstancingRenderer2::GetVertexBufferByteSize()
{
	return std::accumulate(m_Vt2C16B.begin(), m_Vt2C16B.end(), 0,
		[](uint32_t size, const auto& list) { return size + list.second->m_Capacity; }) * sizeof(XMUINT4);
}
