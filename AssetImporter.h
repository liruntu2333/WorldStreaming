#pragma once
#include <filesystem>
#include "VertexPositionNormalTangentTexture.h"

struct Mesh
{
    std::vector<VertexPositionNormalTangentTexture> Vertices{};
    std::vector<uint32_t> Indices{};
    uint32_t MaterialIndex{};

    Mesh() = default;
    Mesh(const std::vector<VertexPositionNormalTangentTexture>& vertices, const std::vector<uint32_t>& indices, uint32_t materialIndex)
        : Vertices(vertices), Indices(indices), MaterialIndex(materialIndex) {}
    Mesh(std::vector<VertexPositionNormalTangentTexture>&& vertices, std::vector<uint32_t>&& indices, uint32_t materialIndex)
        : Vertices(std::move(vertices)), Indices(std::move(indices)), MaterialIndex(materialIndex) {}
};

struct Material
{
    std::filesystem::path TexturePath;
    std::string Name{};

    Material() = default;
    Material(std::filesystem::path texturePath, std::string name)
        : TexturePath(std::move(texturePath)), Name(std::move(name)) {}
};

class AssetImporter
{
public:
    struct ModelData
    {
        std::vector<Mesh> Meshes{};
        std::vector<Material> Materials{};

        ModelData() = default;
        ModelData(ModelData&&) = default;
    };

    static ModelData LoadAsset(const std::filesystem::path& fPath);
};

