#include "BvhTree.h"

#include <algorithm>
#include <stack>
using namespace DirectX;
using namespace SimpleMath;

struct BvhObjectInfo
{
    BoundingSphere Bound;
    uint32_t ObjectIndex{};

    BvhObjectInfo() = default;
    BvhObjectInfo(uint32_t objIdx, const BoundingSphere& bound) : Bound(bound), ObjectIndex(objIdx) {}
};

namespace
{
    struct BucketInfo
    {
        int Count = 0;
        BoundingSphere Bound;
    };

    float GetDim(const Vector3& v, int dim)
    {
        return *(reinterpret_cast<const float*>(&v) + dim);
    }

    float GetCenterDim(const BoundingBox& bb, int dim)
    {
        return *(reinterpret_cast<const float*>(&bb.Center) + dim);
    }

    float GetCenterDim(const BvhObjectInfo& obj, int dim)
    {
        return *(reinterpret_cast<const float*>(&obj.Bound.Center) + dim);
    }

    Vector3 Offset(const BoundingBox& bb, const Vector3& p)
    {
        const Vector3 o = p - bb.Center + bb.Extents;
        return o / (2.0f * bb.Extents);
    }
}

BvhTree::BvhTree(std::vector<StaticObject>& objects, uint32_t maxObjInNode, SpitMethod method) :
    m_MaxObjInNode(maxObjInNode), m_SplitMethod(method)
{
    if (objects.empty()) return;

    std::vector<BvhObjectInfo> objInfo(objects.size());
    for (uint32_t i = 0; i < objects.size(); ++i)
    {
        objInfo[i] = BvhObjectInfo(i, BoundingSphere(objects[i].Position, objects[i].Scale));
    }

    std::vector<StaticObject> orderedObjects;
    orderedObjects.reserve(objects.size());
    uint32_t totalNodes = 0;
    auto root = BuildBvh(objInfo, 0, objects.size(), totalNodes, objects, orderedObjects);

    objects.swap(orderedObjects);

    uint32_t offset = 0;
    m_Nodes.resize(totalNodes);
    FlattenBvhTree(root.get(), offset);
    assert(totalNodes == offset);
    root = nullptr;
}

std::vector<uint32_t> BvhTree::TickCulling(const BoundingFrustum& frustum) const
{
    std::vector<uint32_t> result;

    if (m_Nodes.empty()) return result;

    std::stack<uint32_t> toVisit;
    toVisit.push(0);
    while (!toVisit.empty())
    {
        const BvhLinearNode& node = m_Nodes[toVisit.top()];
        const uint32_t firstChildOffset = toVisit.top() + 1;
        toVisit.pop();
        const auto contain = frustum.Contains(node.Bound);

        if (contain != DISJOINT)
        {
            if (node.ObjectCount > 0)
            {
                for (uint32_t i = 0; i < node.ObjectCount; ++i)
                    result.push_back(node.ObjectOffset + i);
            }
            else
            {
                toVisit.push(node.SecondChildOffset);
                toVisit.push(firstChildOffset);
            }
        }
    }

    return result;
}

void BvhTree::GenerateTree(std::vector<StaticObject>& objects, uint32_t maxObjInNode, SpitMethod method)
{
    m_MaxObjInNode = maxObjInNode;
    m_SplitMethod = method;

    if (objects.empty()) return;

    std::vector<BvhObjectInfo> objInfo(objects.size());
    for (uint32_t i = 0; i < objects.size(); ++i)
    {
        objInfo[i] = BvhObjectInfo(i, BoundingSphere(objects[i].Position, objects[i].Scale));
    }

    std::vector<StaticObject> orderedObjects;
    orderedObjects.reserve(objects.size());
    uint32_t totalNodes = 0;
    auto root = BuildBvh(objInfo, 0, objects.size(), totalNodes, objects, orderedObjects);

    objects.swap(orderedObjects);

    uint32_t offset = 0;
    m_Nodes.resize(totalNodes);
    FlattenBvhTree(root.get(), offset);
    assert(totalNodes == offset);
    root = nullptr;
}

std::unique_ptr<BvhNode> BvhTree::BuildBvh(
    std::vector<BvhObjectInfo>& objInfo, uint32_t start, uint32_t end,
    uint32_t& totalNodes, std::vector<StaticObject>& objects, std::vector<StaticObject>& orderedObj)
{
    std::unique_ptr<BvhNode> node = std::make_unique<BvhNode>();
    ++totalNodes;

    // Compute bounds of all objects in BVH node
    BoundingSphere allBound;
    for (uint32_t i = start; i < end; ++i)
    {
        if (i == start) allBound = objInfo[i].Bound;
        else BoundingSphere::CreateMerged(allBound, allBound, objInfo[i].Bound);
    }

    const uint32_t nObj = end - start;
    if (nObj <= m_MaxObjInNode) // Create leaf
    {
        const uint32_t orderedIdx = orderedObj.size();
        for (uint32_t i = start; i < end; ++i)
        {
            orderedObj.push_back(objects[objInfo[i].ObjectIndex]);
        }
        node->InitLeaf(orderedIdx, nObj, allBound);
        return node;
    }

    // Compute bound of node centroids, choose split dimension
    BoundingBox centerBound;
    BoundingBox::CreateFromPoints(centerBound, nObj,
        reinterpret_cast<const XMFLOAT3*>(&objInfo[start]), sizeof(BvhObjectInfo));
    const auto& extents = centerBound.Extents;
    const int dim = extents.x > extents.y && extents.x > extents.z ? 0 : extents.y > extents.z ? 1 : 2;
    uint32_t mid;

    if (m_SplitMethod == Middle)
    {
        // Split BVH in half along split dimension
        float midPos = *(reinterpret_cast<float*>(&centerBound.Center) + dim);
        const auto itMid = std::partition(
            objInfo.begin() + start, objInfo.begin() + end,
            [dim, midPos](const BvhObjectInfo& info)
            {
                return GetCenterDim(info, dim) < midPos;
            });
        mid = itMid - objInfo.begin();
    }
    else if (m_SplitMethod == EqualCounts)
    {
        // Split BVH into two equal-sized subsets
        mid = (start + end) / 2;
        std::nth_element(
            objInfo.begin() + start, objInfo.begin() + mid, objInfo.begin() + end,
            [dim](const BvhObjectInfo& a, const BvhObjectInfo& b)
            {
                return GetCenterDim(a, dim) < GetCenterDim(b, dim);
            });
    }
    else if (m_SplitMethod == VolumeHeuristic)
    {
        // Split BVH into 16 buckets and choose split dimension where volume metric is minimal
        constexpr int bucketCnt = 16;
        BucketInfo buckets[bucketCnt];

        for (uint32_t i = start; i < end; ++i)
        {
            Vector3 offset = Offset(centerBound, objInfo[i].Bound.Center);
            auto b = static_cast<int>(GetDim(bucketCnt * offset, dim));
            if (b == bucketCnt) b = bucketCnt - 1;
            assert(b >= 0 && b < bucketCnt);
            if (buckets[b].Count++ == 0) 
                buckets[b].Bound = objInfo[i].Bound;
            else 
                BoundingSphere::CreateMerged(buckets[b].Bound, buckets[b].Bound, objInfo[i].Bound);
        }

        float cost[bucketCnt - 1];
        for (int i = 0; i < bucketCnt - 1; ++i)
        {
            BoundingSphere bound0, bound1;
            int count0 = 0, count1 = 0;
            for (int j = 0; j <= i; ++j)
            {
                if (buckets[j].Count)
                    if (count0 == 0) 
                        bound0 = buckets[j].Bound;
                    else 
                        BoundingSphere::CreateMerged(bound0, bound0, buckets[j].Bound);

                count0 += buckets[j].Count;
            }
            for (int j = i + 1; j < bucketCnt; ++j)
            {
                if (buckets[j].Count)
                    if (count1 == 0) 
                        bound1 = buckets[j].Bound;
                    else 
                        BoundingSphere::CreateMerged(bound1, bound1, buckets[j].Bound);

                count1 += buckets[j].Count;
            }
            cost[i] = 1.0f + 
                (count0 * bound0.Radius * bound0.Radius * bound0.Radius +  count1 * bound1.Radius * bound1.Radius * bound1.Radius) / 
                (allBound.Radius * allBound.Radius * allBound.Radius);
        }

        const auto minCost = std::min_element(&cost[0], &cost[bucketCnt - 1]);
        const int minCostBucket = minCost - &cost[0];
        const float leafCost = nObj;
        if (nObj > m_MaxObjInNode || *minCost < leafCost)
        {
            const auto itMid = std::partition(
                objInfo.begin() + start, objInfo.begin() + end,
                [=](const BvhObjectInfo& oi)
                {
                    const Vector3 offset = Offset(centerBound, oi.Bound.Center);
                    auto b = static_cast<int>(GetDim(bucketCnt * offset, dim));
                    if (b == bucketCnt) b = bucketCnt - 1;
                    assert(b >= 0 && b < bucketCnt);
                    return b <= minCostBucket;
                });
            mid = itMid - objInfo.begin();
        }
        else
        {
            const uint32_t orderedIdx = orderedObj.size();
            for (uint32_t i = start; i < end; ++i)
                orderedObj.push_back(objects[objInfo[i].ObjectIndex]);

            node->InitLeaf(orderedIdx, nObj, allBound);
            return node;
        }
    }

    node->InitInterior(
        BuildBvh(objInfo, start, mid, totalNodes, objects, orderedObj),
        BuildBvh(objInfo, mid, end, totalNodes, objects, orderedObj));

    return node;
}

uint32_t BvhTree::FlattenBvhTree(const BvhNode* node, uint32_t& offset)
{
    BvhLinearNode* linearNode = &m_Nodes[offset];
    linearNode->Bound = node->Bound;
    const uint32_t myOffset = offset++;
    if (node->ObjectCount)
    {
        assert(node->Children[0] == nullptr && node->Children[1] == nullptr);
        linearNode->ObjectCount = node->ObjectCount;
        linearNode->ObjectOffset = node->ObjectOffset;
    }
    else
    {
        linearNode->ObjectCount = 0;
        FlattenBvhTree(node->Children[0].get(), offset);
        linearNode->SecondChildOffset = FlattenBvhTree(node->Children[1].get(), offset);
    }

    return myOffset;
}
