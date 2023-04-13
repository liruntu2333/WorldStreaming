#pragma once
#define NOMINMAX
#include <memory>
#include <vector>
#include <directxtk/SimpleMath.h>

#include "CullingSoa.h"
#include "BvhTree.h"

struct ObjectInstance;
struct SubmeshInstance;
class AssetLibrary;
struct GpuConstants;
struct ObjectId;
struct StaticObject;
class Camera;
class Renderer;

constexpr uint32_t SOA_CAPACITY = 1 << 10;

class WorldSystem
{
public:
	WorldSystem(std::shared_ptr<GpuConstants> constants, const std::shared_ptr<const AssetLibrary>& assLib);
	~WorldSystem() = default;

	void Initialize();
	[[nodiscard]] std::pair<std::vector<SubmeshInstance>, std::vector<ObjectInstance>> Tick(const Camera& camera) const;

	[[nodiscard]] uint32_t GetObjectCount() const;
	[[nodiscard]] const std::vector<BvhLinearNode>& GetBvhTree() const;
	void GenerateBvh(uint32_t objInNode, BvhTree::SpitMethod method);

private:
	void ComputeWorlds();

	std::vector<StaticObject> m_Objects {};
	std::vector<DirectX::SimpleMath::Matrix> m_WorldMatrices {};
	std::unique_ptr<CullingSoa<SOA_CAPACITY>> m_Soa = nullptr;
	std::unique_ptr<BvhTree> m_Bvh                  = nullptr;
	std::shared_ptr<GpuConstants> m_Constants       = nullptr;
	std::shared_ptr<const AssetLibrary> m_AssetLib  = nullptr;
};
