#include "DebugRenderer.h"

#include <queue>
#include <directxtk/GeometricPrimitive.h>

#include "Constants.h"
#include "BvhTree.h"

using namespace DirectX;
using namespace SimpleMath;

namespace 
{
    const XMVECTORF32 s_LevelColor[] = {
        Colors::Red,
        Colors::Green,
        Colors::Blue,
        Colors::Yellow,
        Colors::Cyan,
        Colors::Magenta,
        Colors::White,
        Colors::Black,
        Colors::Gray,
    };
}

DebugRenderer::DebugRenderer(ID3D11Device* device, std::shared_ptr<Constants> constants,
    const std::vector<BvhLinearNode>& tree) :
    Renderer(device), m_Constants(std::move(constants)), m_BsTree(tree)
{
}

void DebugRenderer::Initialize(ID3D11DeviceContext* context)
{
    m_SphereGeo = GeometricPrimitive::CreateSphere(context, 2.0f, 16, false);
}

void DebugRenderer::Render(ID3D11DeviceContext* context)
{
    const auto view = m_Constants->View.Transpose();
    const auto proj = m_Constants->Proj.Transpose();

    int visibleDepth = std::log2(m_BsTree.size()) - 1;
    visibleDepth = visibleDepth < 0 ? 0 : visibleDepth;
    std::queue<uint32_t> q;
    if (!m_BsTree.empty()) q.push(0);
    int depth = 0;
    while (!q.empty())
    {
        const int size = q.size();
        const auto color = s_LevelColor[(depth - visibleDepth) % _countof(s_LevelColor)];
        for (int i = 0; i < size; ++i)
        {
            const auto nodeIdx = q.front();
            auto& node = m_BsTree[nodeIdx];
            q.pop();
            if (depth >= visibleDepth)
            {
                auto world = Matrix::CreateScale(node.Bound.Radius) * Matrix::CreateTranslation(node.Bound.Center);
                m_SphereGeo->Draw(world, view, proj, color, nullptr, true);
            }

            if (node.ObjectCount == 0)
            {
                q.push(nodeIdx + 1);
                q.push(node.SecondChildOffset);
            }
        }
        ++depth;
    }
}

void DebugRenderer::UpdateBuffer(ID3D11DeviceContext* context)
{
}
