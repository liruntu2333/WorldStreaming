#pragma once
#include <filesystem>
#include <map>

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

	[[nodiscard]] std::vector<Material> GetMaterialBuffer() const;
	[[nodiscard]] std::vector<SubmeshInstance> GetSubmeshInstanceBuffer(const std::vector<MeshId>& objects) const;
	[[nodiscard]] const std::vector<Vertex>& GetMergedTriangleList() const { return m_MergedTriangleList; }

private:

	[[nodiscard]] auto GetMeshId() { return m_MeshId++; }
	[[nodiscard]] auto GetSubsetId() { return m_SubsetId++; }
	[[nodiscard]] auto GetMaterialId() { return m_MaterialId++; }
	[[nodiscard]] auto GetTextureId() { return m_TextureId++; }

	void LoadMeshes(const std::filesystem::path& dir);
	void MergeTriangleLists();

	std::filesystem::path m_AssetDir = L"./Asset/Mesh";
	std::map<MeshId, std::pair<SubsetId, uint32_t>> m_MeshSubsetMap {};
	std::map<SubsetId, std::vector<Vertex>> m_SubsetTriangle {};
	std::map<SubsetId, MaterialId> m_SubsetMaterialMap {};
	std::map<MaterialId, Material> m_Materials {};

	std::vector<Vertex> m_MergedTriangleList {};
	std::vector<std::filesystem::path> m_MergedTextures {};

	uint32_t m_MeshId     = 0;
	uint32_t m_SubsetId   = 0;
	uint32_t m_MaterialId = 0;
	uint32_t m_TextureId  = 0;

	std::shared_ptr<GpuConstants> m_Constants = nullptr;
};
