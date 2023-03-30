#include "AssetImporter.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "VertexPositionNormalTangentTexture.h"

using namespace DirectX::SimpleMath;

AssetImporter::ModelData AssetImporter::LoadTriangleList(const std::filesystem::path& fPath)
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
    std::vector<Vertex> triangleList;

    const auto meshCnt = scene->mNumMeshes;
    for (uint32_t i = 0; i < meshCnt; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];
        const auto faceCnt = mesh->mNumFaces;

        triangleList.reserve(triangleList.size() + faceCnt * 3);

        for (uint32_t j = 0; j < faceCnt; ++j)
        {
            const aiFace& face = mesh->mFaces[j];
            for (uint32_t k = 0; k < 3; ++k)
            {
                const auto idx = face.mIndices[k];

                Vector3 pos(mesh->mVertices[idx].x, mesh->mVertices[idx].y, mesh->mVertices[idx].z);
                Vector3 norm(mesh->mNormals[idx].x, mesh->mNormals[idx].y, mesh->mNormals[idx].z);
                Vector3 tan(mesh->mTangents[idx].x, mesh->mTangents[idx].y, mesh->mTangents[idx].z);
                Vector2 texCoord(mesh->mTextureCoords[0][idx].x, mesh->mTextureCoords[0][idx].y);

                triangleList.emplace_back(pos, norm, tan, texCoord);
            }
        }
    }

    std::filesystem::path texPath = fPath.parent_path();
    const auto matCnt = scene->mNumMaterials;
    for (uint32_t i = 0; i < matCnt; ++i)
    {
        const auto material = scene->mMaterials[1];
        aiString texPathStr;
        material->GetTexture(aiTextureType_DIFFUSE, 0, &texPathStr);
        if (texPathStr.length != 0)
        {
            texPath /= texPathStr.C_Str();
            break;
        }
    }

    return {triangleList, texPath };
}
