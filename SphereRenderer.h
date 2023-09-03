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
        constexpr int tessellation = 7;
        constexpr float diameter = 20;
        if constexpr (tessellation < 3)
            throw std::invalid_argument("tessellation parameter must be at least 3");

        constexpr size_t verticalSegments = tessellation;
        constexpr size_t horizontalSegments = tessellation * 2;

        constexpr float radius = diameter / 2;

        // Create rings of vertices at progressively higher latitudes.
        for (size_t i = 0; i <= verticalSegments; i++)
        {
            // const float v = 1 - float(i) / float(verticalSegments);

            const float latitude = (float(i) * XM_PI / float(verticalSegments)) - XM_PIDIV2;
            float dy, dxz;

            XMScalarSinCos(&dy, &dxz, latitude);

            // Create a single ring of vertices at this latitude.
            for (size_t j = 0; j <= horizontalSegments; j++)
            {
                // const float u = float(j) / float(horizontalSegments);

                const float longitude = float(j) * XM_2PI / float(horizontalSegments);
                float dx, dz;

                XMScalarSinCos(&dx, &dz, longitude);

                dx *= dxz;
                dz *= dxz;

                const float phi = fmodf(longitude, XM_PIDIV2);
                const float r = cosf(latitude);
                float v = r * phi * (2.0f / XM_PI);
                float u = r - v;
                const float theta = longitude - XM_PI;
                if (latitude < 0)
                {
                    const float tmp = u;
                    u = 1 - v;
                    v = 1 - tmp;
                }
                if (abs(theta) > XM_PIDIV2)
                {
                    v = -v;
                }
                if (theta < 0)
                {
                    u = -u;
                }
                u = u * 0.5f + 0.5f;
                v = v * 0.5f + 0.5f;

                const XMVECTOR normal = XMVectorSet(dx, dy, dz, 0);
                const XMVECTOR textureCoordinate = XMVectorSet(u, v, 0, 0);

                vertices.push_back(VertexPositionNormalTexture(
                    XMVectorScale(normal, radius), normal, textureCoordinate));
            }
        }

        // Fill the index buffer with triangles joining each pair of latitude rings.
        constexpr size_t stride = horizontalSegments + 1;

        for (size_t i = 0; i < verticalSegments; i++)
        {
            for (size_t j = 0; j <= horizontalSegments; j++)
            {
                const size_t nextI = i + 1;
                const size_t nextJ = (j + 1) % stride;

                indices.push_back(i * stride + j);
                indices.push_back(nextI * stride + j);
                indices.push_back(i * stride + nextJ);

                indices.push_back(i * stride + nextJ);
                indices.push_back(nextI * stride + j);
                indices.push_back(nextI * stride + nextJ);
            }
        }
    };

    createSphere();
    m_SphereGeo = GeometricPrimitive::CreateCustom(context, vertices, indices);

    m_Texture = std::make_unique<Texture2D>(m_Device,
        //L"Asset/Texture/rock_planet_texture___flat_by_tbh_1138_d34l30e-pre.jpg");
        L"Asset/Texture/tiles_0059_color_2k.jpg");
}

inline void SphereRenderer::Render(
    const Matrix& view, const Matrix& proj) const
{
    m_SphereGeo->Draw(Matrix::Identity, view, proj, DirectX::Colors::White,
        m_Texture->GetSrv());
}
