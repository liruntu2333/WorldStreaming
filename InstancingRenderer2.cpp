#include "InstancingRenderer2.h"

#include <d3dcompiler.h>
#include "Texture2D.h"
#include "GpuConstants.h"
#include <DirectXPackedVector.h>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>

#define LOAD_OFFLINE 1

using namespace DirectX;
using namespace SimpleMath;

constexpr UINT VERTEX_TEXTURE_WIDTH = 512;

namespace
{
	template <typename T>
	void OutputBin(const std::vector<T>& vts, uint32_t faceStride)
	{
		const std::string name = "Asset/MeshBin/" + std::to_string(faceStride) + ".bin";
		std::ofstream ofs(name, std::ios::binary | std::ios::out);
		if (!ofs)
			throw std::runtime_error("failed to open " + name);
		for (const auto& v : vts)
			ofs.write(reinterpret_cast<const char*>(&v), sizeof(T));
		ofs.close();
	}
}

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

#if LOAD_OFFLINE
	// load vertex data from file
	for (const auto& entry : std::filesystem::directory_iterator("Asset/MeshBin"))
	{
		uint32_t subsetFace = std::stoi(entry.path().filename().string());
		const uint32_t subsetVert = subsetFace * 3;
		std::vector<XMUINT4> vts;
		std::ifstream ifs(entry.path(), std::ios::binary | std::ios::in);
		if (!ifs)
			throw std::runtime_error("failed to open " + entry.path().string());
		const auto fileSize = file_size(entry.path());
		vts.resize(fileSize / sizeof(XMUINT4));
		ifs.read(reinterpret_cast<char*>(vts.data()), fileSize);
		ifs.close();

		const UINT height = (vts.size() + VERTEX_TEXTURE_WIDTH - 1) / VERTEX_TEXTURE_WIDTH;
		vts.resize(height * VERTEX_TEXTURE_WIDTH, XMUINT4());

		CD3D11_TEXTURE2D_DESC vtxTex(DXGI_FORMAT_R32G32B32A32_UINT, VERTEX_TEXTURE_WIDTH, height, 1,
			1, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT);

		m_Vt2C16B[subsetFace] = std::make_unique<StructuredBuffer<XMUINT4>>(m_Device, vts.data(),
			vts.size());

		m_Vt2C16BT[subsetFace] = std::make_unique<Texture2D>(m_Device, vtxTex, vts.data(),
			vtxTex.Width * sizeof(XMUINT4));
		m_Vt2C16BT[subsetFace]->CreateViews(m_Device);
	}

#else
	const auto& divided = m_AssetLibrary->GetMergedTriangleListDivide();

	for (const auto& dir : std::filesystem::directory_iterator(L"Asset/MeshBin"))
		std::filesystem::remove(dir.path());
	for (const auto& [faceStride, vts] : divided)
	{
		std::vector<XMUINT4> encoded;
		encoded.reserve(vts.size());
		for (const auto& v : vts)
		{
			// required to be normalized -1, 1
			auto encodePos = [](Vector3 p) -> uint64_t
			{
				p = p * 32768.0f + Vector3(32768.0f);
				p.Clamp(Vector3(0.0f), Vector3(65535.0f), p);
				const uint64_t x = p.x;
				const uint64_t y = p.y;
				const uint64_t z = p.z;
				return x << 48 | y << 32 | z << 16;
			};

			auto encodeOctahedron = [](Vector3 n) -> uint16_t
			{
				const float absN = std::abs(n.x) + std::abs(n.y) + std::abs(n.z);
				n.x /= absN;
				n.y /= absN;
				if (n.z <= 0.0f)
				{
					const float newX = (1.0f - std::abs(n.y)) * std::copysignf(1.0f, n.x);
					const float newY = (1.0f - std::abs(n.x)) * std::copysignf(1.0f, n.y);
					n.x = newX;
					n.y = newY;
				}

				Vector2 mappedN = Vector2(n.x, n.y) * 128.0f + Vector2(128.0f);
				mappedN.Clamp(Vector2(0.0f), Vector2(255.0f), mappedN);
				const uint16_t mappedNx = mappedN.x;
				const uint16_t mappedNy = mappedN.y;
				return mappedNx << 8 | mappedNy;
			};

			// required to be in -8, 8
			auto encodeUv = [](Vector2 uv) -> uint32_t
			{
				uv /= 8.0f;
				uv = uv * 32768.0f + Vector2(32768.0f);
				uv.Clamp(Vector2(0.0f), Vector2(65535.0f), uv);
				const uint32_t tu = uv.x;
				const uint32_t tv = uv.y;
				return tu << 16 | tv;
			};

			const auto ep = encodePos(v.Pos);
			const auto en = encodeOctahedron(v.Nor);
			const auto et = encodeOctahedron(v.Tan);
			const auto euv = encodeUv(v.Tc);

			encoded.emplace_back(
				static_cast<uint32_t>(ep >> 32 & 0xFFFFFFFF),
				static_cast<uint32_t>(ep & 0xFFFFFFFF),
				static_cast<uint32_t>(en) << 16 | static_cast<uint32_t>(et),
				euv);

			//auto decodeOctahedron = [](uint16_t n) -> Vector3
			//{
			//	const Vector2 oct = Vector2(n >> 8 & 0xff, n & 0xff) / 128.0f - Vector2(1.0f);
			//	auto nor = Vector3(oct.x, oct.y, 1.0f - std::abs(oct.x) - std::abs(oct.y));
			//	if (nor.z < 0)
			//	{
			//		const float newX = (1.0f - std::abs(nor.y)) * std::copysignf(1.0f, nor.x);
			//		const float newY = (1.0f - std::abs(nor.x)) * std::copysignf(1.0f, nor.y);
			//		nor.x = newX;
			//		nor.y = newY;
			//	}
			//	nor.Normalize();
			//	return nor;
			//};

			//const auto ent = encoded.back().z;
			//Vector3 decNor = decodeOctahedron(ent >> 16 & 0xFFFF);
			//Vector3 decTan = decodeOctahedron(ent & 0xFFFF);
		}

		const UINT height = (encoded.size() + VERTEX_TEXTURE_WIDTH - 1) / VERTEX_TEXTURE_WIDTH;
		encoded.resize(height * VERTEX_TEXTURE_WIDTH, XMUINT4(0, 0, 0, 0));

		CD3D11_TEXTURE2D_DESC vtxTex(DXGI_FORMAT_R32G32B32A32_UINT, VERTEX_TEXTURE_WIDTH, height, 1,
			1, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT);
		m_Vt2C16B[faceStride] = std::make_unique<StructuredBuffer<XMUINT4>>(m_Device, encoded.data(),
			encoded.size());
		m_Vt2C16BT[faceStride] = std::make_unique<Texture2D>(m_Device, vtxTex, encoded.data(),
			vtxTex.Width * sizeof(XMUINT4));
		m_Vt2C16BT[faceStride]->CreateViews(m_Device);

		//OutputBin(encoded, faceStride);
	}

#endif
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
		ID3D11ShaderResourceView* srv[] = { m_Pt0->GetSrv(), /*m_Pt1->GetSrv()*/ };
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

		ID3D11ShaderResourceView* srv = m_Vt2C16BT[faceStride]->GetSrv();
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
