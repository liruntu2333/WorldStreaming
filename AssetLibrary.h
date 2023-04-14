#pragma once
#include <filesystem>
#include <map>

#include "AssetImporter.h"
#include "Material.h"
#include "SubmeshInstance.h"
#include "VertexPositionNormalTangentTexture.h"

struct GpuConstants;

class AssetLibrary
{
public:
	using Vertex = VertexPositionNormalTangentTexture;
	using MeshId = uint32_t;
	using SubsetId = uint32_t;
	using MaterialId = uint32_t;
	using TextureId = uint32_t;

	AssetLibrary()  = default;
	~AssetLibrary() = default;

	AssetLibrary(const std::filesystem::path& assetDir, const std::shared_ptr<GpuConstants>& constants) :
		m_AssetDir(assetDir), m_Constants(constants) {}

	void Initialize();

	[[nodiscard]] std::vector<Material> GetMaterialList() const;
	[[nodiscard]] std::vector<SubmeshInstance> GetSubmeshQueryList(const std::vector<MeshId>& objects) const;
	[[nodiscard]] const std::vector<Vertex>& GetMergedTriangleList() const { return m_MergedTriangleList; }

	auto GetMeshCount() const { return m_MeshSubmeshMap.size(); }

private:
	[[nodiscard]] auto GetMeshId()
	{
		assert(m_MeshId != INT32_MAX);
		return m_MeshId++;
	}
	[[nodiscard]] auto GetSubmeshId()
	{
		assert(m_SubmeshId != INT32_MAX);
		return m_SubmeshId++;
	}
	[[nodiscard]] auto GetMaterialId()
	{
		assert(m_MaterialId != INT32_MAX);
		return m_MaterialId++;
	}
	[[nodiscard]] auto GetTextureId()
	{
		assert(m_TextureId != INT32_MAX);
		return m_TextureId++;
	}

	void LoadMeshes(const std::filesystem::path& dir);
	void MergeTriangleLists();
	static void NormalizeVertices(AssetImporter::ImporterModelData& model);

	std::filesystem::path m_AssetDir = L"./Asset/Mesh";
	std::unordered_map<MeshId, std::pair<SubsetId, uint32_t>> m_MeshSubmeshMap {};
	std::unordered_map<SubsetId, std::vector<Vertex>> m_SubmeshTriangle {};
	std::unordered_map<SubsetId, MaterialId> m_SubmeshMaterialMap {};
	std::unordered_map<MaterialId, Material> m_Materials {};

	std::vector<Vertex> m_MergedTriangleList {};
	std::vector<std::filesystem::path> m_MergedTextures {};

	uint32_t m_MeshId     = 0;
	uint32_t m_SubmeshId   = 0;
	uint32_t m_MaterialId = 0;
	uint32_t m_TextureId  = 0;

	std::shared_ptr<GpuConstants> m_Constants = nullptr;
};
