#pragma once
#include <filesystem>
#include "VertexPositionNormalTangentTexture.h"

struct ImporterMeshData
{
    std::vector<VertexPositionNormalTangentTexture> Vertices{};
    std::vector<uint32_t> Indices{};
    uint32_t MaterialIndex{};

    ImporterMeshData() = default;
    ImporterMeshData(const std::vector<VertexPositionNormalTangentTexture>& vertices, const std::vector<uint32_t>& indices, uint32_t materialIndex)
        : Vertices(vertices), Indices(indices), MaterialIndex(materialIndex) {}
    ImporterMeshData(std::vector<VertexPositionNormalTangentTexture>&& vertices, std::vector<uint32_t>&& indices, uint32_t materialIndex)
        : Vertices(std::move(vertices)), Indices(std::move(indices)), MaterialIndex(materialIndex) {}
};

struct ImporterMaterialData
{
    std::string Name{};
    std::filesystem::path TexturePath;

    ImporterMaterialData() = default;
    ImporterMaterialData(std::string name, std::filesystem::path texturePath)
        : Name(std::move(name)), TexturePath(std::move(texturePath)) {}
};

class AssetImporter
{
public:
    struct ImporterModelData
    {
        std::vector<ImporterMeshData> Meshes{};
        std::vector<ImporterMaterialData> Materials{};

        ImporterModelData() = default;
        ImporterModelData(ImporterModelData&&) = default;
    };

    static ImporterModelData LoadAsset(const std::filesystem::path& fPath);
};

