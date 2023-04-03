// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs


#include <chrono>
#include <random>

#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "GlobalContext.h"
#include "WorldSystem.h"
#include "PlaneRenderer.h"
#include "ModelRenderer.h"
#include "Camera.h"
#include "InstanceData.h"
#include "StaticObject.h"

// Data
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

using namespace DirectX::SimpleMath;

namespace
{
	std::unique_ptr<DirectX::Texture2D> g_depthStencil = nullptr;

    std::shared_ptr<Constants> g_PassConstants = nullptr;
    std::shared_ptr<std::vector<InstanceData>> g_Instances = nullptr;
    std::shared_ptr<std::vector<float>> g_BondingRadius = nullptr;
    std::unique_ptr<Renderer> g_PlaneRender = nullptr;
    std::unique_ptr<Renderer> g_ModelRender = nullptr;
    std::unique_ptr<Camera> g_Camera = nullptr;
    std::unique_ptr<WorldSystem> g_WorldSystem = nullptr;
}

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void InitWorldStreaming();
void UpdateConstants(const ImGuiIO& io);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int main(int, char**)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"World Streaming", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    // Load Fonts

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    InitWorldStreaming();

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        UpdateConstants(io);

        // tick particle system
        {
        	//static int cnt = 0;
         //   static float timeSum = 0.0;
         //   static float timeAvg = 0.0;

        	//auto start = std::chrono::steady_clock::now();
        	//{
         //       auto soaRes = g_CullingSoa->TickCulling<Architecture>(g_BsData, g_Camera->GetFrustum());
        	//}
         //   auto end = std::chrono::steady_clock::now();
         //   const auto duration = std::chrono::duration_cast<std::chrono::microseconds>((end - start));
         //   timeSum += duration.count();

	        //if (++cnt > 100)
	        //{
         //       timeAvg = timeSum / static_cast<float>(cnt);
         //       timeSum = 0.0;
         //       cnt = 0;
	        //}

            *g_Instances = g_WorldSystem->Tick(*g_Camera, *g_BondingRadius);

        	ImGui::Begin("Culling tick");
            ImGui::Text("Object count : %d \tVisible count : %d", g_WorldSystem->GetObjectCount(), g_Instances->size());
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, g_depthStencil->GetDsv());
    	g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        g_pd3dDeviceContext->ClearDepthStencilView(g_depthStencil->GetDsv(),
                                                   D3D11_CLEAR_DEPTH, 1.0f, 0);

        g_Camera->SetViewPort(g_pd3dDeviceContext);
        g_PlaneRender->Render(g_pd3dDeviceContext);
        g_ModelRender->Render(g_pd3dDeviceContext);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);

	D3D11_TEXTURE2D_DESC texDesc;
	pBackBuffer->GetDesc(&texDesc);
    CD3D11_TEXTURE2D_DESC dsDesc(DXGI_FORMAT_D24_UNORM_S8_UINT,
        texDesc.Width, texDesc.Height, 1, 0, D3D11_BIND_DEPTH_STENCIL,
        D3D11_USAGE_DEFAULT);
	g_depthStencil = std::make_unique<DirectX::Texture2D>(g_pd3dDevice, dsDesc);
	g_depthStencil->CreateViews(g_pd3dDevice);
	
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

void InitWorldStreaming()
{
    g_Context.Initialize(4);

    g_WorldSystem = std::make_unique<WorldSystem>();
    g_WorldSystem->Initialize();
    g_Instances = std::make_shared<std::vector<InstanceData>>();
    g_BondingRadius = std::make_shared<std::vector<float>>();

    g_PassConstants = std::make_shared<Constants>();
    g_PlaneRender = std::make_unique<PlaneRenderer>(g_pd3dDevice, g_PassConstants);
    g_PlaneRender->Initialize(g_pd3dDeviceContext);

    g_ModelRender = std::make_unique<ModelRenderer>(g_pd3dDevice, L"./Asset/patrick/patrick.obj", g_PassConstants, g_Instances);
    *g_BondingRadius = g_ModelRender->Initialize(g_pd3dDeviceContext);

    g_Camera = std::make_unique<Camera>();
}

void UpdateConstants(const ImGuiIO& io)
{
    g_Camera->Update(io);

    g_PassConstants->View = g_Camera->GetViewMatrix().Transpose();
    g_PassConstants->Proj = g_Camera->GetProjectionMatrix().Transpose();
    g_PassConstants->ViewProj = g_Camera->GetViewProjectionMatrix().Transpose();
    g_PassConstants->EyePosition = g_Camera->GetPosition();
    g_PassConstants->DeltaTime = io.DeltaTime;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

