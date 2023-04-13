#include "AssetLibrary.h"
#include "AssetImporter.h"
#include "GlobalContext.h"
#include "GpuConstants.h"

using namespace DirectX;
using namespace SimpleMath;

void AssetLibrary::Initialize()
{
	LoadMeshes(m_AssetDir);

	MergeTriangleLists();
}

std::vector<Material> AssetLibrary::GetMaterialBuffer() const
{
	std::vector<Material> materials;
	materials.reserve(m_Materials.size());
	for (const auto& [id, mat] : m_Materials)
		materials.emplace_back(mat);
	return materials;
}

std::vector<SubmeshInstance> AssetLibrary::GetSubmeshInstanceBuffer(const std::vector<MeshId>& objects) const
{
	std::vector<SubmeshInstance> instances;
	for (uint32_t i = 0; i < objects.size(); ++i)
	{
		const auto mesh             = objects[i];
		const auto [ssStart, ssCnt] = m_MeshSubsetMap.at(mesh);
		for (int j = 0; j < ssCnt; ++j)
		{
			const SubsetId subset = ssStart + j;
			instances.emplace_back(subset, m_SubsetMaterialMap.at(subset), i);
		}
	}
	return instances;
}

void AssetLibrary::NormalizeVertices(AssetImporter::ImporterModelData& model) {
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
	const auto invRad    = 1.0f / sphere.Radius;
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
		const uint32_t ssCnt     = model.Subsets.size();
		const SubsetId ssIdStart = m_SubsetId;
		for (const auto& mesh : model.Subsets)
		{
			const auto& vb = mesh.Vertices;
			const auto& ib = mesh.Indices;
			std::vector<Vertex> triangles;
			triangles.reserve(ib.size() * 3);
			for (const uint32_t i : ib)
				triangles.emplace_back(vb[i]);
			m_SubsetTriangle.emplace(GetSubsetId(), triangles);
		}

		// TODO: load materials and build texture list
		// generate material ids
		[[maybe_unused]] const uint32_t matCnt = model.Materials.size();
		const MaterialId matIdStart            = m_MaterialId;
		for (const auto& material : model.Materials)
		{
			Material mat;
			mat.TexIdx = std::rand() % 32;
			m_Materials.emplace(GetMaterialId(), mat);
		}

		// build subset material map
		for (uint32_t i = 0; i < ssCnt; ++i)
		{
			const auto subsetId   = i + ssIdStart;
			const auto materialId = model.Subsets[i].MaterialIndex + matIdStart;
			m_SubsetMaterialMap.emplace(subsetId, materialId);
		}

		// build mesh-subset map
		m_MeshSubsetMap.emplace(GetMeshId(), std::pair(ssIdStart, ssCnt));
	}

	g_Context.MeshCount = m_MeshId;
}

void AssetLibrary::MergeTriangleLists()
{
	m_MergedTriangleList.clear();
	const uint32_t maxSubsetVertCnt = std::max_element(m_SubsetTriangle.begin(), m_SubsetTriangle.end(),
		[](const auto& lhs, const auto& rhs) { return lhs.second.size() < rhs.second.size(); })->second.size();
	m_MergedTriangleList.reserve(m_SubsetTriangle.size() * maxSubsetVertCnt * 3);
	for (const auto& [id, subset] : m_SubsetTriangle)
	{
		std::copy(subset.begin(), subset.end(), std::back_inserter(m_MergedTriangleList));
		for (uint32_t i = subset.size(); i < maxSubsetVertCnt; ++i)
			m_MergedTriangleList.emplace_back();
	}

	m_Constants->VertexPerMesh = maxSubsetVertCnt;
}
