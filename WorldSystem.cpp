#include <random>
#include "WorldSystem.h"
#include "CullingSoa.h"
#include "Camera.h"
#include "ObjectInstance.h"
#include "StaticObject.h"
#include "BvhTree.h"
#include "GpuConstants.h"
#include "AssetLibrary.h"
#include "ObjectInstance.h"

using namespace DirectX::SimpleMath;

namespace DirectX
{
	std::vector<uint32_t> TickCulling(
		const std::vector<BoundingSphere>& data,
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
	constexpr uint32_t BVH_NODE_CAP = 128;
	constexpr BvhTree::SpitMethod BVH_METHOD = BvhTree::SpitMethod::Middle;

	std::vector<StaticObject> GenerateRandom(int geoCnt)
	{
		std::vector<StaticObject> res;
		res.reserve(OBJECT_MAX);
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
		std::uniform_int_distribution disGeo(0, std::max(geoCnt - 1, 0));
		for (uint32_t i = 0; i < OBJECT_MAX; ++i)
		{
			auto pos = Vector3(disX(gen), disY(gen), disZ(gen));
			auto rot = Quaternion::CreateFromYawPitchRoll(disYaw(gen), disPitch(gen), disRow(gen));
			auto scale = disScale(gen);
			uint32_t col = disCol(gen) << 24 | disCol(gen) << 16 | disCol(gen) << 8 | 255;
			auto mat = disMat(gen);
			auto geo = disGeo(gen);
			res.emplace_back(pos, rot, 50.0f, geo, mat, col);
		}
		return res;
	}
}

WorldSystem::WorldSystem(
	std::shared_ptr<GpuConstants> constants, const std::shared_ptr<const AssetLibrary>& assLib) : m_Soa(
		std::make_unique<CullingSoa<OBJECT_MAX>>()), m_Constants(std::move(constants)),
	m_AssetLib(assLib) {}

void WorldSystem::ComputeWorlds()
{
	m_WorldMatrices.clear();
	m_WorldMatrices.reserve(m_Objects.size());
	for (const auto& object : m_Objects)
	{
		m_WorldMatrices.emplace_back((Matrix::CreateScale(object.Scale) *
			Matrix::CreateFromQuaternion(object.Rotation) *
			Matrix::CreateTranslation(object.Position)).Transpose());
	}
}

void WorldSystem::Initialize()
{
	m_Objects = GenerateRandom(m_AssetLib->GetMeshCount());
	m_Bvh = std::make_unique<BvhTree>(m_Objects, BVH_NODE_CAP, BVH_METHOD);
	ComputeWorlds();
}

std::pair<std::vector<SubmeshInstance>, std::vector<ObjectInstance>> WorldSystem::Tick(const Camera& camera) const
{
	const auto frustum = camera.GetFrustum();
	auto visible = m_Bvh->TickCulling(frustum);
	std::vector<DirectX::BoundingSphere> spheres(visible.size());
	for (uint32_t i = 0; i < visible.size(); ++i)
	{
		spheres[i].Center = m_Objects[visible[i]].Position;
		spheres[i].Radius = m_Objects[visible[i]].Scale;
	}

	const auto visible2 = m_Soa->TickCulling<xsimd::default_arch>(spheres, frustum);
	//const auto visible2 = TickCulling(spheres, frustum);
	//const auto visible2 = m_Soa->TickCulling(spheres, frustum);

	for (uint32_t i = 0; i < visible2.size(); ++i)
	{
		visible[i] = visible[visible2[i]];
	}
	visible.resize(visible2.size());

	std::vector<uint32_t> meshIndices;
	meshIndices.reserve(visible.size());
	std::vector<ObjectInstance> objIns;
	objIns.reserve(visible.size());
	for (const uint32_t i : visible)
	{
		meshIndices.push_back(m_Objects[i].GeometryIndex);
		objIns.emplace_back(m_WorldMatrices[i], m_Objects[i].Color);
	}
	auto const submeshIns = m_AssetLib->GetSubmeshQueryList(meshIndices);

	return { submeshIns, objIns };
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
	ComputeWorlds();
}
