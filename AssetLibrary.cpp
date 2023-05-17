#include "AssetLibrary.h"

#include <fstream>
#include <iostream>
#include <unordered_set>

#include "AssetImporter.h"
#include "GpuConstants.h"

#include <nlohmann/json.hpp>

using namespace DirectX;
using namespace SimpleMath;

constexpr uint32_t MIN_TRIANGLE_COUNT = 16;

namespace
{
	uint32_t HighestPowerOf2(uint32_t n)
	{
		n |= n >> 1;
		n |= n >> 2;
		n |= n >> 4;
		n |= n >> 8;
		n |= n >> 16;
		return n - (n >> 1);
	}
}

void AssetLibrary::Initialize()
{
	LoadMeshes(m_AssetDir);

	MergeTriangleLists();

	MergeTriangleListsDivide();

	OutputJson();
}

std::vector<Material> AssetLibrary::GetMaterialList() const
{
	std::vector<Material> materials;
	materials.reserve(m_MaterialTbl.size());
	for (const auto& [id, mat] : m_MaterialTbl)
		materials.emplace_back(mat);
	return materials;
}

std::vector<SubmeshInstance> AssetLibrary::QuerySubmesh(const std::vector<MeshId>& objects) const
{
	std::vector<SubmeshInstance> instances;
	for (uint32_t i = 0; i < objects.size(); ++i)
	{
		const auto mesh = objects[i];
		const auto [ssStart, ssCnt] = m_SubmeshTbl.at(mesh);
		for (int j = 0; j < ssCnt; ++j)
		{
			const SubmeshId subId = ssStart + j;
			instances.emplace_back(subId, m_MaterialIdTbl.at(subId), i);
		}
	}
	return instances;
}

std::vector<DividedSubmeshInstance> AssetLibrary::QuerySubmeshDivide(const std::vector<MeshId>& objects) const
{
	std::map<uint32_t, std::vector<SubmeshInstance>> layers;
	for (uint32_t i = 0; i < objects.size(); ++i)
	{
		const auto mesh = objects[i];
		const auto [ssStart, ssCnt] = m_SubmeshTbl.at(mesh);
		for (int j = 0; j < ssCnt; ++j)
		{
			const SubmeshId subId = ssStart + j;
			for (const auto& [fCnt, offset] : m_BatchInfo.at(subId))
				layers[fCnt].emplace_back(offset, m_MaterialIdTbl.at(subId), i);
		}
	}
	std::vector<DividedSubmeshInstance> drawParams;
	for (auto& [fCnt, ins] : layers)
	{
		drawParams.emplace_back(fCnt, std::move(ins));
	}
	return drawParams;
}

void AssetLibrary::NormalizeVertices(AssetImporter::ImporterModelData& model)
{
	std::vector<Vector3> vp;
	BoundingSphere sphere;
	for (const auto& mesh : model.Subsets)
	{
		const auto& vb = mesh.Vertices;
		vp.reserve(vp.size() + vb.size());
		for (const auto& v : vb)
			vp.emplace_back(v.Pos);
	}
	BoundingSphere::CreateFromPoints(sphere, vp.size(), vp.data(), sizeof(Vector3));
	const auto invRad = 1.0f / sphere.Radius;
	const Vector3 center = sphere.Center;
	for (auto& mesh : model.Subsets)
		for (auto& v : mesh.Vertices)
			v.Pos = (v.Pos - center) * invRad;
}

void AssetLibrary::LoadMeshes(const std::filesystem::path& dir)
{
	std::unordered_map<std::wstring, const uint32_t> texIdMap;
	texIdMap.emplace(L"./Asset/Texture/default.dds", m_TextureId);

	std::unordered_set<std::wstring> texExist;
	for (const auto& entry : std::filesystem::directory_iterator(L"./Asset/Texture"))
		texExist.emplace(entry.path().filename().wstring());

	m_TextureTbl[GetTextureId()] = L"./Asset/Texture/default.dds";
	m_MaterialTbl[GetMaterialId()] = Material();
	for (const auto& entry : std::filesystem::directory_iterator(dir))
	{
		if (!entry.is_regular_file() || entry.path().extension() != ".fbx") continue;
		AssetImporter::ImporterModelData model = AssetImporter::LoadAsset(entry.path());

		// TODO: pass normalization transform matrix to culling system instead of normalizing vertices data
		// Calculate bounding sphere and normalize vertices
		NormalizeVertices(model);

		// Convert to triangle list vbs
		const uint32_t ssCnt = model.Subsets.size();
		const SubmeshId ssIdStart = m_SubmeshId;
		for (const auto& mesh : model.Subsets)
		{
			const auto& vb = mesh.Vertices;
			const auto& ib = mesh.Indices;
			std::vector<Vertex> triangles;
			triangles.reserve(ib.size() * 3);
			for (const uint32_t i : ib)
				triangles.emplace_back(vb[i]);
			m_TriangleTbl[GetSubmeshId()] = triangles;
		}

		// TODO: load materials and build texture list
		// generate material ids
		[[maybe_unused]] const uint32_t matCnt = model.Materials.size();
		const MaterialId matIdStart = m_MaterialId;
		for (const auto& material : model.Materials)
		{
			//Material mat;
			//mat.TexIdx = std::rand() % 32;
			//m_MaterialTbl[GetMaterialId()] = mat;

			if (material.TexturePath.empty() ||
				texExist.find(material.TexturePath.wstring()) == texExist.end())
			{
				m_MaterialTbl[GetMaterialId()] = Material();
				continue;
			}

			if (auto find = texIdMap.find(material.TexturePath.wstring()); find != texIdMap.end())
				m_MaterialTbl[GetMaterialId()] = Material(find->second);
			else
			{
				const auto texId = GetTextureId();
				m_TextureTbl[texId] = L"./Asset/Texture/" + material.TexturePath.wstring();
				texIdMap.emplace(material.TexturePath.wstring(), texId);
				m_MaterialTbl.emplace(GetMaterialId(), Material(texId));
			}
		}

		// build subset material map
		for (uint32_t i = 0; i < ssCnt; ++i)
		{
			const auto subsetId = i + ssIdStart;
			const auto materialId = model.Subsets[i].MaterialIndex + matIdStart;
			m_MaterialIdTbl[subsetId] = materialId;
		}

		// build mesh-subset map
		m_SubmeshTbl[GetMeshId()] = { ssIdStart, ssCnt };
	}
}

void AssetLibrary::MergeTriangleLists()
{
	m_MergedTriangleList.clear();

	const uint32_t maxSubsetVertCnt = std::max_element(m_TriangleTbl.begin(), m_TriangleTbl.end(),
		[](const auto& lhs, const auto& rhs) { return lhs.second.size() < rhs.second.size(); })->second.size();
	m_MergedTriangleList.reserve(m_TriangleTbl.size() * maxSubsetVertCnt * 3);
	std::map batches(m_TriangleTbl.begin(), m_TriangleTbl.end());
	for (const auto& [id, subset] : batches)
	{
		std::copy(subset.begin(), subset.end(), std::back_inserter(m_MergedTriangleList));
		for (uint32_t i = subset.size(); i < maxSubsetVertCnt; ++i)
			m_MergedTriangleList.emplace_back();
	}

	m_Constants->VertexPerMesh = maxSubsetVertCnt;
}

void AssetLibrary::MergeTriangleListsDivide()
{
	m_MergedTriangleBatches.clear();

	for (const auto& [id, vts] : m_TriangleTbl)
	{
		using It = std::vector<Vertex>::const_iterator;
		std::function<void(It, It)> divide = [&](const It& begin, const It& end)
		{
			const uint32_t vtxCnt = std::distance(begin, end);
			const auto faceCnt = vtxCnt / 3;
			uint32_t batchStride = HighestPowerOf2(faceCnt);
			if (faceCnt == 0) return;
			if (faceCnt <= MIN_TRIANGLE_COUNT)
			{
				batchStride = MIN_TRIANGLE_COUNT;
				std::copy(begin, end, std::back_inserter(m_MergedTriangleBatches[batchStride]));
				for (int i = vtxCnt; i < MIN_TRIANGLE_COUNT * 3; ++i)
					m_MergedTriangleBatches[batchStride].emplace_back();
			}
			else
				std::copy_n(begin, batchStride * 3, std::back_inserter(m_MergedTriangleBatches[batchStride]));

			const uint32_t offsetInBatch = m_MergedTriangleBatches[batchStride].size() / (batchStride * 3) - 1;
			m_BatchInfo[id].emplace_back(batchStride, offsetInBatch);

			if (faceCnt > MIN_TRIANGLE_COUNT) divide(begin + batchStride * 3, end);
		};
		divide(vts.begin(), vts.end());
	}
}

void AssetLibrary::OutputJson() const
{
	const nlohmann::json jTexList(m_TextureTbl);
	std::ofstream o("Asset/Json/TextureList.json");
	o << std::setw(4) << jTexList;
	o.close();

	const nlohmann::json jSubmeshTbl(m_SubmeshTbl);
	o.open("Asset/Json/SubmeshTable.json");
	o << std::setw(4) << jSubmeshTbl;
	o.close();

	std::map<SubmeshId, TextureId> texIdMap;
	for (const auto& [submesh, material] : m_MaterialIdTbl)
	{
		const auto texId = m_MaterialTbl.at(material).TexIdx;
		texIdMap.emplace(submesh, texId);
	}

	const nlohmann::json jTexIdTbl(texIdMap);
	o.open("Asset/Json/TextureTable.json");
	o << std::setw(4) << jTexIdTbl;
	o.close();

	const nlohmann::json jBatchInfo(m_BatchInfo);
	o.open("Asset/Json/BatchInfo.json");
	o << std::setw(4) << jBatchInfo;
	o.close();
}
