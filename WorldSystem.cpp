#include <random>
#include "WorldSystem.h"
#include "CullingSoa.h"
#include "Camera.h"
#include "Instance.h"
#include "StaticObject.h"
#include "BvhTree.h"

using namespace DirectX::SimpleMath;

namespace DirectX
{
    std::vector<uint32_t> TickCulling(const std::vector<BoundingSphere>& data,
        const BoundingFrustum& frustum)
    {
        std::vector<uint32_t> res;
        res.reserve(data.size());
        for (uint32_t i = 0; i < data.size(); ++i)
            if (frustum.Contains(data[i]) != DISJOINT)
                res.push_back(i);
        return res;
    }
}

namespace 
{
    constexpr uint32_t BVH_NODE_CAP = 1;
    constexpr BvhTree::SpitMethod BVH_METHOD = BvhTree::SpitMethod::Middle;

    std::vector<StaticObject> GenerateRandom()
    {
        std::vector<StaticObject> res;
        res.reserve(SOA_CAPACITY);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution disX(-2000.0f, 2000.0f);
        std::uniform_real_distribution disY(-2000.0f, 2000.0f);
        std::uniform_real_distribution disZ(-2000.0f, 2000.0f);
        std::uniform_real_distribution disRow(0.0f, DirectX::XM_2PI);
        std::uniform_real_distribution disPitch(0.0f, DirectX::XM_2PI);
        std::uniform_real_distribution disYaw(0.0f, DirectX::XM_2PI);
        std::uniform_real_distribution disScale(10.0f, 50.0f);
        std::uniform_int_distribution disCol(0, 255);
        std::uniform_int_distribution disMat(0, 31);
        std::uniform_int_distribution disGeo(0, 11);
        for (uint32_t i = 0; i < SOA_CAPACITY; ++i)
        {
            auto pos = Vector3(disX(gen), disY(gen), disZ(gen));
            auto rot = Quaternion::CreateFromYawPitchRoll(disYaw(gen), disPitch(gen), disRow(gen));
            auto scale = disScale(gen);
            uint32_t col = disCol(gen) << 24 | disCol(gen) << 16 | disCol(gen) << 8 | 255;
            auto mat = disMat(gen);
            auto geo = disGeo(gen);
            res.emplace_back(pos, rot, scale, geo, mat, col);
        }
        return res;
    }
}

WorldSystem::WorldSystem() : m_Soa(std::make_unique<CullingSoa<SOA_CAPACITY>>())
{
}

void WorldSystem::Initialize()
{
    m_Objects = GenerateRandom();
    m_Bvh = std::make_unique<BvhTree>(m_Objects, BVH_NODE_CAP, BVH_METHOD);
}

std::vector<Instance> WorldSystem::Tick(const Camera& camera) const
{
    const auto frustum = camera.GetFrustum();
    auto visible = m_Bvh->TickCulling(frustum);
    std::vector<DirectX::BoundingSphere> spheres(visible.size());

    for (uint32_t i = 0; i < visible.size(); ++i)
    {
        spheres[i].Center = m_Objects[visible[i]].Position;
        spheres[i].Radius = m_Objects[visible[i]].Scale;
    }

    const auto visible2 = m_Soa->TickCulling<xsimd::avx2>(spheres, frustum);
    //const auto visible2 = TickCulling(spheres, frustum);
    //const auto visible2 = m_Soa->TickCulling(spheres, frustum);

    for (uint32_t i = 0; i < visible2.size(); ++i)
    {
        visible[i] = visible[visible2[i]];
    }
    visible.resize(visible2.size());

    std::vector<Instance> instances(visible.size());
    for (uint32_t i = 0; i < instances.size(); ++i)
    {
        const uint32_t objIdx = visible[i];
        auto& ins = instances[i];
        ins.World = (Matrix::CreateScale(m_Objects[objIdx].Scale) *
            Matrix::CreateFromQuaternion(m_Objects[objIdx].Rotation) *
            Matrix::CreateTranslation(m_Objects[objIdx].Position)).Transpose();
        ins.GeoIdx = m_Objects[objIdx].GeometryIndex;
        ins.MatIdx = m_Objects[objIdx].MaterialIndex;
        ins.Color = m_Objects[objIdx].Color;
        ins.Param = 0;
    }
    return instances;
}

uint32_t WorldSystem::GetObjectCount() const
{
    return m_Objects.size();
}

const std::vector<BvhLinearNode>& WorldSystem::GetBvhTree() const
{
    return m_Bvh->GetTree();
}

void WorldSystem::GenerateBvh(uint32_t objInNode, BvhTree::SpitMethod method)
{
    m_Bvh->GenerateTree(m_Objects, objInNode, method);
}
