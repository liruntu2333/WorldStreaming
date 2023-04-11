#include "AssetImporter.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "VertexPositionNormalTangentTexture.h"

using namespace DirectX::SimpleMath;

AssetImporter::ModelData AssetImporter::LoadAsset(const std::filesystem::path& fPath)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(fPath.string().c_str(),
        aiProcess_Triangulate | aiProcess_CalcTangentSpace | 
        aiProcess_GenNormals | aiProcess_ConvertToLeftHanded);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        throw std::runtime_error(importer.GetErrorString());
    }

    using Vertex = VertexPositionNormalTangentTexture;
    ModelData model{};

    const auto meshCnt = scene->mNumMeshes;
    for (uint32_t i = 0; i < meshCnt; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];
        const auto vertCnt = mesh->mNumVertices;
        std::vector<Vertex> vb;
        for (uint32_t j = 0; j < vertCnt; ++j)
        {
            const auto& pos = mesh->mVertices[j];
            const auto& normal = mesh->mNormals[j];
            const auto& tangent = mesh->mTangents[j];
            const auto& texCoord = mesh->mTextureCoords[0][j];
            vb.emplace_back(
                Vector3(pos.x, pos.y, pos.z),
                Vector3(normal.x, normal.y, normal.z),
                Vector3(tangent.x, tangent.y, tangent.z),
                Vector2(texCoord.x, texCoord.y));
        }

        std::vector<uint32_t> ib;
        const auto faceCnt = mesh->mNumFaces;
        for (uint32_t j = 0; j < faceCnt; ++j)
        {
            const aiFace& face = mesh->mFaces[j];
            for (uint32_t k = 0; k < 3; ++k)
            {
                ib.push_back(face.mIndices[k]);
            }
        }

        const auto matIndex = mesh->mMaterialIndex;
        model.Meshes.emplace_back(std::move(vb), std::move(ib), matIndex);
    }

    std::filesystem::path texPath = fPath.parent_path();
    const auto matCnt = scene->mNumMaterials;
    for (uint32_t i = 0; i < matCnt; ++i)
    {
        const auto material = scene->mMaterials[i];
        aiString tex;
        material->GetTexture(aiTextureType_DIFFUSE, 0, &tex);
        auto name = material->GetName();
        if (tex.length != 0)
        {
            model.Materials.emplace_back(tex.C_Str(), name.C_Str());
        }
    }

    return model;
}
