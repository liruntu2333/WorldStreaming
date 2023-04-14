#include "AssetLibrary.h"
#include "AssetImporter.h"
#include "GpuConstants.h"

using namespace DirectX;
using namespace SimpleMath;

void AssetLibrary::Initialize()
{
	LoadMeshes(m_AssetDir);

	MergeTriangleLists();
}

std::vector<Material> AssetLibrary::GetMaterialList() const
{
	std::vector<Material> materials;
	materials.reserve(m_Materials.size());
	for (const auto& [id, mat] : m_Materials)
		materials.emplace_back(mat);
	return materials;
}

std::vector<SubmeshInstance> AssetLibrary::GetSubmeshQueryList(const std::vector<MeshId>& objects) const
{
	std::vector<SubmeshInstance> instances;
	for (uint32_t i = 0; i < objects.size(); ++i)
	{
		const auto mesh = objects[i];
		const auto [ssStart, ssCnt] = m_MeshSubmeshMap.at(mesh);
		for (int j = 0; j < ssCnt; ++j)
		{
			const SubsetId subset = ssStart + j;
			instances.emplace_back(subset, m_SubmeshMaterialMap.at(subset), i);
		}
	}
	return instances;
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
	for (const auto& entry : std::filesystem::directory_iterator(dir))
	{
		if (!entry.is_regular_file() || entry.path().extension() != ".fbx") continue;


		AssetImporter::ImporterModelData model = AssetImporter::LoadAsset(entry.path());

		// TODO: pass normalization transform matrix to culling system instead of normalizing vertices data
		// Calculate bounding sphere and normalize vertices
		NormalizeVertices(model);

		// Convert to triangle list vbs
		const uint32_t ssCnt = model.Subsets.size();
		const SubsetId ssIdStart = m_SubmeshId;
		for (const auto& mesh : model.Subsets)
		{
			const auto& vb = mesh.Vertices;
			const auto& ib = mesh.Indices;
			std::vector<Vertex> triangles;
			triangles.reserve(ib.size() * 3);
			for (const uint32_t i : ib)
				triangles.emplace_back(vb[i]);
			m_SubmeshTriangle[GetSubmeshId()] = triangles;
		}

		// TODO: load materials and build texture list
		// generate material ids
		[[maybe_unused]] const uint32_t matCnt = model.Materials.size();
		const MaterialId matIdStart = m_MaterialId;
		for (const auto& material : model.Materials)
		{
			Material mat;
			mat.TexIdx = std::rand() % 32;
			m_Materials[GetMaterialId()] = mat;
		}

		// build subset material map
		for (uint32_t i = 0; i < ssCnt; ++i)
		{
			const auto subsetId = i + ssIdStart;
			const auto materialId = model.Subsets[i].MaterialIndex + matIdStart;
			m_SubmeshMaterialMap[subsetId] = materialId;
		}

		// build mesh-subset map
		m_MeshSubmeshMap[GetMeshId()] = { ssIdStart, ssCnt };
	}
}

void AssetLibrary::MergeTriangleLists()
{
	m_MergedTriangleList.clear();
	const uint32_t maxSubsetVertCnt = std::max_element(m_SubmeshTriangle.begin(), m_SubmeshTriangle.end(),
		[](const auto& lhs, const auto& rhs) { return lhs.second.size() < rhs.second.size(); })->second.size();
	m_MergedTriangleList.reserve(m_SubmeshTriangle.size() * maxSubsetVertCnt * 3);
	for (const auto& [id, subset] : m_SubmeshTriangle)
	{
		std::copy(subset.begin(), subset.end(), std::back_inserter(m_MergedTriangleList));
		for (uint32_t i = subset.size(); i < maxSubsetVertCnt; ++i)
			m_MergedTriangleList.emplace_back();
	}

	m_Constants->VertexPerMesh = maxSubsetVertCnt;
}
