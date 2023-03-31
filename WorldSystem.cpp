#include <random>
#include "WorldSystem.h"
#include "CullingSoa.h"
#include "Camera.h"
#include "InstanceData.h"
#include "StaticObject.h"

using namespace DirectX::SimpleMath;

namespace DirectX
{
    std::vector<size_t> TickCulling(const std::vector<BoundingSphere>& data,
        const BoundingFrustum& frustum)
    {
        std::vector<size_t> res;
        res.reserve(data.size());
        for (size_t i = 0; i < data.size(); ++i)
            if (frustum.Contains(data[i]) != DISJOINT)
                res.push_back(i);
        return res;
    }
}

WorldSystem::WorldSystem() : m_Soa(std::make_unique<CullingSoa<SoaCapacity>>())
{
}

void WorldSystem::Initialize()
{
    m_Objects.reserve(SoaCapacity);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution disX(-2000.0f, 2000.0f);
    std::uniform_real_distribution disY(0.0f, 1000.0f);
    std::uniform_real_distribution disZ(-2000.0f, 2000.0f);
    std::uniform_real_distribution disRow(0.0f, DirectX::XM_2PI);
    std::uniform_real_distribution disPitch(0.0f, DirectX::XM_2PI);
    std::uniform_real_distribution disYaw(0.0f, DirectX::XM_2PI);
    std::uniform_real_distribution disScale(1.0f, 50.0f);
    std::uniform_int_distribution disCol(0, 255);
    for (size_t i = 0; i < SoaCapacity; ++i)
    {
        auto pos = Vector3(disX(gen), disY(gen), disZ(gen));
        auto rot = Quaternion::CreateFromYawPitchRoll(disYaw(gen), disPitch(gen), disRow(gen));
        auto scale = disScale(gen);
        uint32_t col = disCol(gen) << 24 | disCol(gen) << 16 | disCol(gen) << 8 | 255;
        m_Objects.emplace_back(pos, rot, scale, 0, 0, col);
    }
}

std::vector<InstanceData> WorldSystem::Tick(const Camera& camera)
{
    std::vector<DirectX::BoundingSphere> spheres(m_Objects.size());
    for (size_t i = 0; i < m_Objects.size(); ++i)
    {
        spheres[i].Center = m_Objects[i].Position;
        spheres[i].Radius = m_Objects[i].Scale * 0.815749228f;
    }

    const auto frustum = camera.GetFrustum();
    const auto visible = m_Soa->TickCulling<xsimd::default_arch>(spheres, frustum);
    //const auto visible2 = m_Soa->TickCulling(spheres, frustum);
    //const auto visible2 = TickCulling(spheres, frustum);
    //assert(visible == visible2);

    std::vector<InstanceData> ins(visible.size());
    for (int i = 0; i < ins.size(); ++i)
    {
        ins[i].World = (Matrix::CreateScale(m_Objects[visible[i]].Scale) * 
            Matrix::CreateFromQuaternion(m_Objects[visible[i]].Rotation) *
            Matrix::CreateTranslation(m_Objects[visible[i]].Position)).Transpose();
        ins[i].GeoIdx = m_Objects[visible[i]].GeometryIndex;
        ins[i].MatIdx = m_Objects[visible[i]].MaterialIndex;
        ins[i].Color = m_Objects[visible[i]].Color;
        ins[i].Param = 0;
    }
    return ins;
}

size_t WorldSystem::GetObjectCount() const
{
    return m_Objects.size();
}
