#pragma once
#include <map>

#include "AssetImporter.h"

struct Material
{
	uint32_t TexIdx = 0; // Global texture index
	Material() = default;
	Material(uint32_t texIdx)
		: TexIdx(texIdx) {}
};


class MeshSplitter
{
public:
	using Vertex = VertexPositionNormalTangentTexture;
	using EncodedVertex = DirectX::XMUINT4;
	using MeshId = uint32_t;
	using SubmeshId = uint32_t;
	using MaterialId = uint32_t;
	using TextureId = uint32_t;

	using VertexCluster = std::vector<Vertex>;

	using FacePerSubset = uint32_t;
	using OffsetInBatch = uint32_t;
	using LocationInBatch = std::pair<FacePerSubset, OffsetInBatch>;

	MeshSplitter(const std::filesystem::path& assetDir = "./") :
		m_AssetDir(assetDir) {}

	~MeshSplitter() = default;
	void Split();

protected:
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
	void SplitIntoCluster();
	void EncodeVertices();
	void OutputFile() const;
	static void NormalizeVertices(AssetImporter::ImporterModelData& model);

	std::filesystem::path m_AssetDir;
	std::unordered_map<MeshId, std::pair<SubmeshId, uint32_t>> m_SubmeshTbl {};
	std::unordered_map<SubmeshId, VertexCluster> m_SubmeshVertices {};
	std::unordered_map<SubmeshId, MaterialId> m_MaterialIdTbl {};
	std::unordered_map<MaterialId, Material> m_Materials {};
	std::map<TextureId, std::filesystem::path> m_TextureTbl {};

	// stride = 16, 32, 64, ... triangles
	std::map<FacePerSubset, VertexCluster> m_Clusters {};
	// on which batches and offset of what
	std::unordered_map<SubmeshId, std::vector<LocationInBatch>> m_BatchInfo {};
	std::unordered_map<SubmeshId, std::vector<EncodedVertex>> m_EncodedClusters {};

	uint32_t m_MeshId = 0;
	uint32_t m_SubmeshId = 0;
	uint32_t m_MaterialId = 0;
	uint32_t m_TextureId = 0;
};
