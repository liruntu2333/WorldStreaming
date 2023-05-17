#pragma once
#define NOMINMAX
#include <memory>
#include <vector>
#include <directxtk/SimpleMath.h>

#include "AssetLibrary.h"
#include "CullingSoa.h"
#include "BvhTree.h"
#include "GlobalContext.h"

struct ObjectInstance;
struct SubmeshInstance;
struct GpuConstants;
struct ObjectId;
struct StaticObject;
class Camera;
class Renderer;

class WorldSystem
{
public:
	WorldSystem(std::shared_ptr<GpuConstants> constants, const std::shared_ptr<const AssetLibrary>& assLib);
	~WorldSystem() = default;

	void Initialize();
	[[nodiscard]] std::pair<std::vector<ObjectInstance>, std::vector<DividedSubmeshInstance>> Tick(
		const Camera& camera, bool freezeFrustum) const;

	[[nodiscard]] uint32_t GetObjectCount() const;
	[[nodiscard]] const std::vector<BvhLinearNode>& GetBvhTree() const;
	void GenerateBvh(uint32_t objInNode, BvhTree::SpitMethod method);

private:
	void ComputeWorlds();

	std::vector<StaticObject> m_Objects {};
	std::vector<DirectX::SimpleMath::Matrix> m_WorldMatrices {};
	std::unique_ptr<CullingSoa<OBJECT_MAX>> m_Soa = nullptr;
	std::unique_ptr<BvhTree> m_Bvh = nullptr;
	std::shared_ptr<GpuConstants> m_Constants = nullptr;
	std::shared_ptr<const AssetLibrary> m_AssetLib = nullptr;
};
