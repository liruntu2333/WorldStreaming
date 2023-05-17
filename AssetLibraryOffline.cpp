#include "AssetLibraryOffline.h"
#include <fstream>
#include <nlohmann/json.hpp>

void AssetLibraryOffline::Initialize()
{
	// read json file
	m_TextureTbl.clear();
	std::ifstream i("Asset/Json/TextureList.json");
	nlohmann::json j;
	i >> j;
	i.close();
	auto texTbl = j.get<std::map<TextureId, std::string>>();
	for (const auto& [id, path] : texTbl)
		m_TextureTbl[id] = path;

	i.open("Asset/Json/SubmeshTable.json");
	i >> j;
	i.close();
	m_SubmeshTbl = j.get<std::unordered_map<MeshId, std::pair<SubmeshId, uint32_t>>>();

	i.open("Asset/Json/TextureTable.json");
	i >> j;
	i.close();
	m_SubmeshTextureMap = j.get<std::unordered_map<SubmeshId, TextureId>>();

	i.open("Asset/Json/BatchInfo.json");
	i >> j;
	i.close();
	m_BatchInfo = j.get<std::unordered_map<SubmeshId, std::vector<LocationInBatch>>>();
}

std::vector<DividedSubmeshInstance> AssetLibraryOffline::QuerySubmeshDivide(const std::vector<MeshId>& objects) const
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
				layers[fCnt].emplace_back(offset, m_SubmeshTextureMap.at(subId), i);
		}
	}
	std::vector<DividedSubmeshInstance> drawParams;
	for (auto& [fCnt, ins] : layers)
	{
		drawParams.emplace_back(fCnt, std::move(ins));
	}
	return drawParams;
}
