#pragma once
#include <filesystem>
#include <map>

#include "AssetImporter.h"
#include "Material.h"
#include "SubmeshInstance.h"
#include "VertexPositionNormalTangentTexture.h"

struct GpuConstants;

using DividedSubmeshInstance = std::pair<uint32_t, std::vector<SubmeshInstance>>;

class AssetLibrary
{
public:
	using Vertex = VertexPositionNormalTangentTexture;
	using MeshId = uint32_t;
	using SubmeshId = uint32_t;
	using MaterialId = uint32_t;
	using TextureId = uint32_t;

	using TriangleBatch = std::vector<Vertex>;

	using FacePerSubset = uint32_t;
	using OffsetInBatch = uint32_t;
	using LocationInBatch = std::pair<FacePerSubset, OffsetInBatch>;

	AssetLibrary() = default;
	~AssetLibrary() = default;

	AssetLibrary(const std::filesystem::path& assetDir, const std::shared_ptr<GpuConstants>& constants) :
		m_AssetDir(assetDir), m_Constants(constants) {}

	void Initialize();

	[[nodiscard]] std::vector<Material> GetMaterialList() const;
	[[nodiscard]] std::vector<SubmeshInstance> QuerySubmesh(const std::vector<MeshId>& objects) const;

	[[nodiscard]] std::vector<DividedSubmeshInstance> QuerySubmeshDivide(const std::vector<MeshId>& objects) const;
	[[nodiscard]] const auto& GetMergedTriangleList() const { return m_MergedTriangleList; }
	[[nodiscard]] const auto& GetMergedTriangleListDivide() const { return m_MergedTriangleBatches; }
	[[nodiscard]] const auto& GetTextureTable() const { return m_TextureTbl; }

	auto GetMeshCount() const { return m_SubmeshTbl.size(); }

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
	void MergeTriangleListsDivide();
	static void NormalizeVertices(AssetImporter::ImporterModelData& model);

	std::filesystem::path m_AssetDir = L"./Asset/Mesh";
	std::unordered_map<MeshId, std::pair<SubmeshId, uint32_t>> m_SubmeshTbl {};
	std::unordered_map<SubmeshId, TriangleBatch> m_TriangleTbl {};
	std::unordered_map<SubmeshId, MaterialId> m_MaterialIdTbl {};
	std::unordered_map<MaterialId, Material> m_MaterialTbl {};
	std::map<TextureId, std::filesystem::path> m_TextureTbl {};

	// stride = 16, 32, 64, ... triangles
	std::map<FacePerSubset, TriangleBatch> m_MergedTriangleBatches {};
	// on which batches and offset of what
	std::unordered_map<SubmeshId, std::vector<LocationInBatch>> m_BatchInfo {};

	TriangleBatch m_MergedTriangleList {};

	uint32_t m_MeshId = 0;
	uint32_t m_SubmeshId = 0;
	uint32_t m_MaterialId = 0;
	uint32_t m_TextureId = 0;

	std::shared_ptr<GpuConstants> m_Constants = nullptr;
};
