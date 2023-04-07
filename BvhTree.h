#pragma once

#include <memory>
#include <directxtk/SimpleMath.h>
#include "StaticObject.h"

struct BvhObjectInfo;
struct BvhLinearNode
{
    DirectX::BoundingSphere Bound;
    union
    {
        uint32_t ObjectOffset{};
        uint32_t SecondChildOffset;
    };
    uint32_t ObjectCount{};

    BvhLinearNode() = default;
};

struct BvhNode
{
    DirectX::BoundingSphere Bound;
    std::unique_ptr<BvhNode> Children[2];
    uint32_t ObjectOffset;
    uint32_t ObjectCount;

    BvhNode() = default;
    void InitLeaf(uint32_t start, uint32_t n, const DirectX::BoundingSphere& bound)
    {
        ObjectOffset = start;
        ObjectCount = n;
        Bound = bound;
        Children[0] = nullptr;
        Children[1] = nullptr;
    }
    void InitInterior(std::unique_ptr<BvhNode> c0, std::unique_ptr<BvhNode> c1)
    {
        Children[0] = std::move(c0);
        Children[1] = std::move(c1);
        Bound = Children[0]->Bound;
        Bound.CreateMerged(Bound, Bound, Children[1]->Bound);
        ObjectCount = 0;
    }
};

class BvhTree
{
public:

    enum SpitMethod : int
    {
        Middle = 0,
        EqualCounts,
        VolumeHeuristic,
    };

    BvhTree(std::vector<StaticObject>& objects, uint32_t maxObjInNode, SpitMethod method);
    [[nodiscard]] std::vector<uint32_t> TickCulling(const DirectX::BoundingFrustum& frustum) const;

    [[nodiscard]] const std::vector<BvhLinearNode>& GetTree() const { return m_Nodes; }

    void GenerateTree(std::vector<StaticObject>& objects, uint32_t maxObjInNode, SpitMethod method);

private:

    std::unique_ptr<BvhNode> BuildBvh(std::vector<BvhObjectInfo>& objInfo, uint32_t start, uint32_t end,
                                      uint32_t& totalNodes, std::vector<StaticObject>& objects, std::vector<StaticObject>& orderedObj);

    uint32_t FlattenBvhTree(const BvhNode* node, uint32_t& offset);

    std::vector<BvhLinearNode> m_Nodes{};
    uint32_t m_MaxObjInNode;
    SpitMethod m_SplitMethod;
};
