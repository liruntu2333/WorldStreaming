#pragma once
#include "AlignedVector.h"
#include <directxtk/SimpleMath.h>

template <size_t Capacity>
class CullingSoa
{
public:
    CullingSoa();
    ~CullingSoa() = default;

    CullingSoa(const CullingSoa&) = delete;
    CullingSoa(CullingSoa&&) = delete;
    CullingSoa& operator=(const CullingSoa&) = delete;
    CullingSoa& operator=(CullingSoa&&) = delete;

    template <class Architecture>
    std::vector<size_t> TickCulling(const std::vector<DirectX::BoundingSphere>& data,
        const DirectX::BoundingFrustum& frustum);

    // scalar version
    std::vector<size_t> TickCulling(const std::vector<DirectX::BoundingSphere>& data,
        const DirectX::BoundingFrustum& frustum);

private:
    void Clear();
    void Push(const DirectX::BoundingSphere& sphere);
    void Push(const std::vector<DirectX::BoundingSphere>& data);

    static constexpr size_t FloatCount = 4;
    static constexpr size_t BoolCount = 1;
    static constexpr size_t Alignment = xsimd::avx2::alignment(); // bigger alignment for new arch

    AlignedVector<float, Capacity, Alignment> m_FloatChunk;
    AlignedVector<bool, Capacity, Alignment> m_BoolChunk;
    size_t m_Size = 0;

    float* const m_PositionX;
    float* const m_PositionY;
    float* const m_PositionZ;
    float* const m_Radius;
    bool*  const m_IsVisible;
};

template <size_t Capacity>
CullingSoa<Capacity>::CullingSoa() : m_FloatChunk(FloatCount), m_BoolChunk(BoolCount),
    m_PositionX(m_FloatChunk[0]), m_PositionY(m_FloatChunk[1]), m_PositionZ(m_FloatChunk[2]), m_Radius(m_FloatChunk[3]), m_IsVisible(m_BoolChunk[0])
{
}

template <size_t Capacity>
void CullingSoa<Capacity>::Push(const DirectX::BoundingSphere& sphere)
{
    if (m_Size >= Capacity) return;

    m_PositionX[m_Size] = sphere.Center.x;
    m_PositionY[m_Size] = sphere.Center.y;
    m_PositionZ[m_Size] = sphere.Center.z;
    m_Radius[m_Size] = sphere.Radius;
    m_Size++;
}

template <size_t Capacity>
void CullingSoa<Capacity>::Push(const std::vector<DirectX::BoundingSphere>& data)
{
    for (const auto& sphere : data) Push(sphere);
}

template <size_t Capacity>
std::vector<size_t> CullingSoa<Capacity>::TickCulling(const std::vector<DirectX::BoundingSphere>& data,
    const DirectX::BoundingFrustum& frustum)
{
    Clear();
    Push(data);

    DirectX::XMVECTOR vs[6];
    frustum.GetPlanes(vs, vs + 1, vs + 2, vs + 3, vs + 4, vs + 5);
    DirectX::SimpleMath::Plane planes[6];
    for (size_t i = 0; i < 6; ++i) 
    {
        planes[i] = DirectX::SimpleMath::Plane(vs[i]);
    }

    for (size_t i = 0; i < m_Size; ++i)
    {
        bool isVisible = true;
        for (const auto& plane : planes)
            isVisible = isVisible && plane.w + m_PositionX[i] * plane.x + m_PositionY[i] * plane.y + m_PositionZ[i] * plane.z < m_Radius[i];
        m_IsVisible[i] = isVisible;
    }

    std::vector<size_t> result;
    for (size_t i = 0; i < m_Size; ++i)
    {
        if (m_IsVisible[i]) result.push_back(i);
    }
    return result;
}

template <size_t Capacity>
template <class Architecture>
std::vector<size_t> CullingSoa<Capacity>::TickCulling(const std::vector<DirectX::BoundingSphere>& data,
    const DirectX::BoundingFrustum& frustum)
{
    Clear();
    Push(data);

    DirectX::XMVECTOR vs[6];
    frustum.GetPlanes(vs, vs + 1, vs + 2, vs + 3, vs + 4, vs + 5);
    DirectX::SimpleMath::Plane planes[6];
    for (size_t i = 0; i < 6; ++i)
    {
        planes[i] = DirectX::SimpleMath::Plane(vs[i]);
    }

    using FloatBatch = xsimd::batch<float, Architecture>;
    using BoolBatch = xsimd::batch_bool<float, Architecture>;
    const size_t stride = FloatBatch::size;

    auto dispatch = [stride, &planes, this](size_t from, size_t length)
    {
        const size_t alignedSize = length - length % stride;

        for (size_t i = from; i < from + alignedSize; i += stride)
        {
            BoolBatch visible(true);
            for (const auto& plane : planes)
            {
                // 3 multiply 3 add 1 compare
                FloatBatch dist(plane.w);
                dist += FloatBatch::load_aligned(m_PositionX + i) * plane.x;
                dist += FloatBatch::load_aligned(m_PositionY + i) * plane.y;
                dist += FloatBatch::load_aligned(m_PositionZ + i) * plane.z;
                visible = dist < FloatBatch::load_aligned(m_Radius + i) && visible;
            }
            visible.store_aligned(m_IsVisible + i);
        }

        for (size_t i = from + alignedSize; i < from + length; ++i)
        {
            bool visible = true;
            for (const auto& plane : planes)
                visible &= plane.w + m_PositionX[i] * plane.x + m_PositionY[i] * plane.y + m_PositionZ[i] * plane.z < m_Radius[i];
            m_IsVisible[i] = visible;
        }
    };

    // multi-threading
    //ThreadPool* pool = g_Context.Pool.get();
    //const auto threadCount = g_Context.ThreadCount;
    //std::vector<std::future<void>> futures;
    //const int groupLen = m_Size / threadCount;

    //for (int i = 0; i < threadCount; ++i)
    //    futures.emplace_back(
    //        pool->enqueue(dispatch, i * groupLen, groupLen));

    //for (size_t i = 0; i < threadCount; ++i)
    //    futures[i].wait();

    dispatch(0, m_Size);

    std::vector<size_t> visible;
    for (size_t i = 0; i < m_Size; ++i)
        if (m_IsVisible[i]) 
            visible.push_back(i);

    return visible;
}

template <size_t Capacity>
void CullingSoa<Capacity>::Clear()
{
    m_Size = 0;
}
