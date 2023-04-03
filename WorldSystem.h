#pragma once
#define NOMINMAX
#include <memory>
#include <vector>
#include <directxtk/SimpleMath.h>

#include "CullingSoa.h"

struct InstanceData;
struct StaticObject;
class Camera;
class Renderer;

constexpr size_t SoaCapacity = 1 << 13;

class WorldSystem
{
public:

    WorldSystem();
    ~WorldSystem() = default;

    void Initialize();
    std::vector<InstanceData> Tick(const Camera& camera, const std::vector<float>& rads);

    [[nodiscard]] size_t GetObjectCount() const;

private:

    std::vector<StaticObject> m_Objects{};
    std::unique_ptr<CullingSoa<SoaCapacity>> m_Soa = nullptr;
};

