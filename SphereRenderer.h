#pragma once

#include "Renderer.h"
#include <directxtk/GeometricPrimitive.h>
#include <directxtk/SimpleMath.h>
#include "Texture2D.h"

class SphereRenderer : public Renderer
{
public:
    SphereRenderer(ID3D11Device* device) : Renderer(device) {}
    ~SphereRenderer() override = default;

    void Initialize(ID3D11DeviceContext* context) override;
    void Render(ID3D11DeviceContext* context) override {}
    void UpdateBuffer(ID3D11DeviceContext* context) override {}
    void Render(
        const Matrix& view,
        const Matrix& proj) const;

protected:
    std::unique_ptr<DirectX::GeometricPrimitive> m_SphereGeo = nullptr;
    std::unique_ptr<DirectX::Texture2D> m_Texture = nullptr;
};

inline void SphereRenderer::Initialize(ID3D11DeviceContext* context)
{
    using namespace DirectX;
    GeometricPrimitive::VertexCollection vertices;
    GeometricPrimitive::IndexCollection indices;
    vertices.clear();
    indices.clear();

    auto createSphere = [&]
    {
        constexpr int tessellation = 32;
        constexpr float diameter = 20;
        if constexpr (tessellation < 3 || tessellation % 2 != 0)
            throw std::invalid_argument("tessellation parameter must be at least 3, and even.");

        constexpr size_t verticalSegments = tessellation;
        constexpr size_t quadSegments = verticalSegments / 2;
        constexpr size_t horizontalVertices = (quadSegments + 1) * 4;

        constexpr float radius = diameter / 2;

        // Create rings of vertices at progressively higher latitudes.
        for (size_t i = 0; i <= verticalSegments; i++)
        {
            // const float v = 1 - float(i) / float(verticalSegments);

            const float latitude = (float(i) / verticalSegments - 0.5f) * XM_PI;
            float dy, dxz;

            XMScalarSinCos(&dy, &dxz, latitude);

            // Create a single ring of vertices at this latitude.
            for (int k = 0; k < 4; ++k)
            {
                for (size_t j = 0; j <= quadSegments; j++)
                {
                    // const float u = float(j) / float(horizontalSegments);
                    const float longitude = (float(j) / quadSegments + k) * XM_PIDIV2;
                    float dx, dz;

                    XMScalarSinCos(&dx, &dz, longitude);

                    dx *= dxz;
                    dz *= dxz;

                    const float r = 1.0f - abs(latitude / XM_PIDIV2);
                    float v = r * j / quadSegments;
                    float u = r - v;
                    if (latitude < 0)
                    {
                        const float uu = 1 - v;
                        const float vv = 1 - u;
                        u = uu;
                        v = vv;
                    }

                    if (k == 1)
                    {
                        const float uu = -v;
                        const float vv = u;
                        u = uu;
                        v = vv;
                    }
                    else if (k == 2)
                    {
                        u = -u;
                        v = -v;
                    }
                    else if (k == 3)
                    {
                        const float uu = v;
                        const float vv = -u;
                        u = uu;
                        v = vv;
                    }

                    u = u * 0.5f + 0.5f;
                    v = -v * 0.5f + 0.5f; // flip v to map from RH to LH texcoords

                    const XMVECTOR normal = XMVectorSet(dx, dy, dz, 0);
                    const XMVECTOR textureCoordinate = XMVectorSet(u, v, 0, 0);

                    vertices.push_back(VertexPositionNormalTexture(
                        XMVectorScale(normal, radius), normal, textureCoordinate));
                }
            }
        }

        // Fill the index buffer with triangles joining each pair of latitude rings.
        constexpr size_t stride = horizontalVertices;

        for (size_t i = 0; i < verticalSegments; i++)
        {
            for (int k = 0; k < 4; ++k)
            {
                for (size_t j = k * (quadSegments + 1); j < k * (quadSegments + 1) + quadSegments; j++)
                {
                    const size_t nextI = i + 1;
                    const size_t nextJ = j + 1;

                    indices.push_back(i * stride + j);
                    indices.push_back(nextI * stride + j);
                    indices.push_back(i * stride + nextJ);

                    indices.push_back(i * stride + nextJ);
                    indices.push_back(nextI * stride + j);
                    indices.push_back(nextI * stride + nextJ);
                }
            }
        }
    };

    createSphere();
    m_SphereGeo = GeometricPrimitive::CreateCustom(context, vertices, indices);

    m_Texture = std::make_unique<Texture2D>(m_Device,
        L"Asset/Texture/jixie01ceshi.escene.png");
        // L"Asset/Texture/tiles_0059_color_2k.jpg");
}

inline void SphereRenderer::Render(
    const Matrix& view, const Matrix& proj) const
{
    m_SphereGeo->Draw(Matrix::Identity, view, proj, DirectX::Colors::White,
        m_Texture->GetSrv());
}
