#define NOMINMAX
#include "MeshSplitter.h"

#include <fstream>
#include <iostream>

#include "directxtk/SimpleMath.h"
#include <unordered_set>
#include <nlohmann/json.hpp>

using namespace DirectX;
using namespace SimpleMath;

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

	template <typename T>
	void OutputVertexBin(const std::vector<T>& vts, uint32_t faceStride)
	{
		const std::string name = "Asset/MeshBin/" + std::to_string(faceStride) + ".bin";
		std::ofstream ofs(name, std::ios::binary | std::ios::out);
		if (!ofs)
			throw std::runtime_error("failed to open " + name);
		for (const auto& v : vts)
			ofs.write(reinterpret_cast<const char*>(&v), sizeof(T));
		ofs.close();

		std::cout << name << " generated" << std::endl;
	}
}

void MeshSplitter::Split()
{
	LoadMeshes(m_AssetDir);

	SplitIntoCluster();

	EncodeVertices();

	OutputFile();
}

void MeshSplitter::LoadMeshes(const std::filesystem::path& dir)
{
	std::unordered_map<std::string, const uint32_t> texIdMap;
	texIdMap.emplace("default.dds", m_TextureId);

	std::unordered_set<std::string> texExist;
	for (const auto& entry : std::filesystem::directory_iterator("Asset/Texture"))
		texExist.emplace(entry.path().filename().string());

	m_TextureTbl[GetTextureId()] = "default.dds";
	m_Materials[GetMaterialId()] = Material();
	for (const auto& entry : std::filesystem::directory_iterator("Asset/Mesh"))
	{
		if (!entry.is_regular_file() || entry.path().extension() != ".fbx") continue;
		AssetImporter::ImporterModelData model = AssetImporter::LoadAsset(entry.path());

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
			m_SubmeshVertices[GetSubmeshId()] = triangles;
		}

		// generate material ids
		[[maybe_unused]] const uint32_t matCnt = model.Materials.size();
		const MaterialId matIdStart = m_MaterialId;
		for (const auto& material : model.Materials)
		{
			if (material.TexturePath.empty() ||
				texExist.find(material.TexturePath.string()) == texExist.end())
			{
				m_Materials[GetMaterialId()] = Material();
				continue;
			}

			if (auto find = texIdMap.find(material.TexturePath.string()); find != texIdMap.end())
				m_Materials[GetMaterialId()] = Material(find->second);
			else
			{
				const auto texId = GetTextureId();
				m_TextureTbl[texId] = material.TexturePath.string();
				texIdMap.emplace(material.TexturePath.string(), texId);
				m_Materials.emplace(GetMaterialId(), Material(texId));
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
	uint64_t faceCnt = 0;
	uint64_t maxFace = 0;
	for (const auto& [id, vts] : m_SubmeshVertices)
	{
		const uint32_t vtxCnt = vts.size();
		faceCnt += vtxCnt / 3;
		maxFace = std::max(maxFace, static_cast<uint64_t>(vtxCnt / 3));
	}
	faceCnt /= m_SubmeshVertices.size();
	std::cout << "Meshes loaded, " << m_SubmeshVertices.size() << " sub-meshes, " << 
		faceCnt << " faces on average" <<
		", " << maxFace << " faces at most" << std::endl;
}

void MeshSplitter::SplitIntoCluster()
{
	m_Clusters.clear();
	constexpr uint32_t MIN_TRIANGLE_COUNT = 16;

	for (const auto& [id, vts] : m_SubmeshVertices)
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
				std::copy(begin, end, std::back_inserter(m_Clusters[batchStride]));
				for (int i = vtxCnt; i < MIN_TRIANGLE_COUNT * 3; ++i)
					m_Clusters[batchStride].emplace_back();
			}
			else
				std::copy_n(begin, batchStride * 3, std::back_inserter(m_Clusters[batchStride]));

			const uint32_t offsetInBatch = m_Clusters[batchStride].size() / (batchStride * 3) - 1;
			m_BatchInfo[id].emplace_back(batchStride, offsetInBatch);

			if (faceCnt > MIN_TRIANGLE_COUNT) divide(begin + batchStride * 3, end);
		};
		divide(vts.begin(), vts.end());
	}
}

void MeshSplitter::EncodeVertices()
{
	m_EncodedClusters.clear();
	for (const auto& [faceStride, vts] : m_Clusters)
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
		}
		if (encoded.size() % (faceStride * 3) != 0)
			throw std::runtime_error("Invalid encoded vertex count");
		m_EncodedClusters[faceStride] = encoded;
	}
}

void MeshSplitter::OutputFile() const
{
	for (const auto& dir : std::filesystem::directory_iterator(L"Asset/MeshBin"))
		std::filesystem::remove(dir.path());
	for (const auto& [faceCnt, evs] : m_EncodedClusters)
	{
		OutputVertexBin(evs, faceCnt);
	}

	const nlohmann::json jTexList(m_TextureTbl);
	std::ofstream o("Json/TextureList.json");
	o << std::setw(4) << jTexList;
	o.close();
	std::cout << "TextureList.json generated" << std::endl;

	const nlohmann::json jSubmeshTbl(m_SubmeshTbl);
	o.open("Json/SubmeshTable.json");
	o << std::setw(4) << jSubmeshTbl;
	o.close();
	std::cout << "SubmeshTable.json generated" << std::endl;

	std::map<SubmeshId, TextureId> texIdMap;
	for (const auto& [submesh, material] : m_MaterialIdTbl)
	{
		const auto texId = m_Materials.at(material).TexIdx;
		texIdMap.emplace(submesh, texId);
	}

	const nlohmann::json jTexIdTbl(texIdMap);
	o.open("Json/TextureTable.json");
	o << std::setw(4) << jTexIdTbl;
	o.close();
	std::cout << "TextureTable.json generated" << std::endl;

	const nlohmann::json jBatchInfo(m_BatchInfo);
	o.open("Json/BatchInfo.json");
	o << std::setw(4) << jBatchInfo;
	o.close();
	std::cout << "BatchInfo.json generated" << std::endl;
}

void MeshSplitter::NormalizeVertices(AssetImporter::ImporterModelData& model)
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
