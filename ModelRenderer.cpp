#include "ModelRenderer.h"

#include "AssetImporter.h"
#include "VertexPositionNormalTangentTexture.h"
#include <d3dcompiler.h>
#include <numeric>

#include "Instance.h"

#include <directxtk/GeometricPrimitive.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

constexpr size_t InstanceCapacity = 1 << 13;

ModelRenderer::ModelRenderer(ID3D11Device* device, std::filesystem::path model, 
                             std::shared_ptr<Constants> constants, std::shared_ptr<std::vector<Instance>> instances) :
    Renderer(device), m_Constants(std::move(constants)), m_Instances(std::move(instances)), m_AssetDir(std::move(model))
{
}

void ModelRenderer::Initialize(ID3D11DeviceContext* context)
{
    m_Vc0 = std::make_unique<ConstantBuffer<Constants>>(m_Device);

    auto [meshes, textures] = LoadAssets(m_AssetDir);
	m_MeshLib = std::move(meshes);
	m_TexLib = std::move(textures);

	m_Vt0 = MergeVert(context, m_MeshStats);
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

std::pair<ModelRenderer::MeshLib, ModelRenderer::TexLib> ModelRenderer::LoadAssets(
	const std::filesystem::path& dir)
{
	for (const auto& entry : std::filesystem::directory_iterator(dir))
	{
		if (entry.is_regular_file())
		{
			std::vector<Vertex> vb;
			AssetImporter::ModelData model = AssetImporter::LoadAsset(entry.path());

			for (const auto & mesh : model.Meshes)
			{
				std::copy(mesh.Vertices.begin(), mesh.Vertices.end(), std::back_inserter(vb));
			}
		}
	}
}

std::unique_ptr<StructuredBuffer<ModelRenderer::Vertex>> ModelRenderer::MergeVert(ID3D11DeviceContext* context, MeshStats& stats)
{
	stats.clear();
	std::vector<Vertex> mergedVert;
	for (const auto & mesh : m_MeshLib)
	{
		stats.push_back(mesh.size());
	}

	const auto meshCnt = scene->mNumMeshes;
	for (uint32_t i = 0; i < meshCnt; ++i)
	{
		const aiMesh* mesh = scene->mMeshes[i];

		const auto vertCnt = mesh->mNumVertices;
		DirectX::BoundingSphere sphere;
		DirectX::BoundingSphere::CreateFromPoints(sphere, vertCnt,
												  reinterpret_cast<const DirectX::XMFLOAT3*>(mesh->mVertices), sizeof(aiVector3D));
		const Vector3 offset = sphere.Center;
		const float radius = sphere.Radius;
		const auto faceCnt = mesh->mNumFaces;

		triangleList.reserve(triangleList.size() + faceCnt * 3);

		for (uint32_t j = 0; j < faceCnt; ++j)
		{
			const aiFace& face = mesh->mFaces[j];
			for (uint32_t k = 0; k < 3; ++k)
			{
				const auto idx = face.mIndices[k];

				Vector3 pos(mesh->mVertices[idx].x, mesh->mVertices[idx].y, mesh->mVertices[idx].z);
				pos = (pos - offset) / radius;
				Vector3 norm(mesh->mNormals[idx].x, mesh->mNormals[idx].y, mesh->mNormals[idx].z);
				Vector3 tan(mesh->mTangents[idx].x, mesh->mTangents[idx].y, mesh->mTangents[idx].z);
				Vector2 texCoord(mesh->mTextureCoords[0][idx].x, mesh->mTextureCoords[0][idx].y);

				triangleList.emplace_back(pos, norm, tan, texCoord);
			}
		}
	}

	const uint32_t vertMax = *std::max_element(stats.begin(), stats.end());
	for (const auto& mesh : m_MeshLib)
	{
		mergedVert.insert(mergedVert.end(), mesh.begin(), mesh.end());
		Vertex back = mergedVert.back();
		for (uint32_t i = mesh.size(); i < vertMax; ++i)
			mergedVert.push_back(back);
	}

	return std::make_unique<StructuredBuffer<Vertex>>(m_Device, mergedVert.data(), mergedVert.size());
}
