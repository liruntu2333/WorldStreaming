#pragma once
#include <filesystem>

struct VertexPositionNormalTangentTexture;
class AssetImporter
{
public:
    struct ModelData
    {
        std::vector<VertexPositionNormalTangentTexture> TriangleList;
        std::filesystem::path TexturePath;
        float BondingRadius = 0.0f;
    };

    static ModelData LoadTriangleList(const std::filesystem::path& fPath);
};

