#include <chrono>
#include <iostream>
#include <random>

#include "CullingSoa.h"
#include "GlobalContext.h"

constexpr size_t SoaCapacity = 1 << 14;

namespace DirectX
{
    std::vector<size_t> TickCulling(const std::vector<BoundingSphere>& data,
        const BoundingFrustum& frustum)
    {
        std::vector<size_t> res;
        res.reserve(data.size());
        for (size_t i = 0; i < data.size(); ++i)
            if (frustum.Contains(data[i]) != DISJOINT)
                res.push_back(i);
        return res;
    }
}

int main()
{
    g_Context.Initialize(4);

    CullingSoa<SoaCapacity> cullingSoa;
    using namespace DirectX::SimpleMath;

    std::vector<DirectX::BoundingSphere> data(SoaCapacity);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution dis(-100.0f, 100.0f);
    for (int i = 0; i < SoaCapacity; ++i)
    {
        data[i] = DirectX::BoundingSphere(Vector3
            (dis(gen), dis(gen), dis(gen)), 1.0f);
    }

    DirectX::BoundingFrustum frustum(DirectX::XMMatrixPerspectiveFovLH
        (DirectX::XM_PIDIV4, 1.0f, 1.0f, 1000.0f));

    while (true)
    {
        static int loop = 0;
        static float tsDop = 0.0f;
        static float tsOop = 0.0f;

        std::vector<size_t> soaRes;
        std::vector<size_t> dxRes;

        {
            const auto start = std::chrono::steady_clock::now();
            soaRes = cullingSoa.TickCulling<xsimd::avx2>(data, frustum);
            const auto end = std::chrono::steady_clock::now();

            const auto duration = std::chrono::duration_cast<std::chrono::microseconds>((end - start));
            tsDop += duration.count();
        }

        //{
        //    const auto start = std::chrono::steady_clock::now();
        //    dxRes = DirectX::TickCulling(data, frustum);
        //    //auto res = cullingSoa.TickCulling(data, frustum);
        //    const auto end = std::chrono::steady_clock::now();

        //    const auto duration = std::chrono::duration_cast<std::chrono::microseconds>((end - start));
        //    tsOop += duration.count();
        //}

        assert(soaRes == dxRes);

        if (++loop == 10000)
        {
            system("cls");
            std::cout << "CullingSoa time elapsed : " << tsDop / static_cast<float>(loop) << " us." << std::endl;
            std::cout << "DXCollision time elapsed : " << tsOop / static_cast<float>(loop) << " us." << std::endl;
            loop = 0;
            tsDop = 0.0f;
            tsOop = 0.0f;
        }
    }
}
