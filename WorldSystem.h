#pragma once
#define NOMINMAX
#include <memory>
#include <vector>
#include <directxtk/SimpleMath.h>

#include "CullingSoa.h"
#include "BvhTree.h"

struct Instance;
struct StaticObject;
class Camera;
class Renderer;

constexpr uint32_t SOA_CAPACITY = 1 << 13;

class WorldSystem
{
public:

    WorldSystem();
    ~WorldSystem() = default;

    void Initialize();
    [[nodiscard]] std::vector<Instance> Tick(const Camera& camera) const;

    [[nodiscard]] uint32_t GetObjectCount() const;
    [[nodiscard]] const std::vector<BvhLinearNode>& GetBvhTree() const;
    void GenerateBvh(uint32_t objInNode, BvhTree::SpitMethod method);

private:

    std::vector<StaticObject> m_Objects{};
    std::unique_ptr<CullingSoa<SOA_CAPACITY>> m_Soa = nullptr;
    std::unique_ptr<BvhTree> m_Bvh = nullptr;
};

