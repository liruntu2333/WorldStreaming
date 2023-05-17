#pragma once

#include "AssetLibrary.h"

class AssetLibraryOffline : public AssetLibrary
{
public:
	AssetLibraryOffline() = default;
	~AssetLibraryOffline() override = default;

	void Initialize() override;
	[[nodiscard]] std::vector<DividedSubmeshInstance>
	QuerySubmeshDivide(const std::vector<MeshId>& objects) const override;

protected:
	std::unordered_map<SubmeshId, TextureId> m_SubmeshTextureMap {};
};
