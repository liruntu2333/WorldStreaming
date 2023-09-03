// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ dir + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs


#include <chrono>

#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "GlobalContext.h"
#include "WorldSystem.h"
#include "PlaneRenderer.h"
#include "InstancingRenderer2.h"
#include "InstancingRenderer3.h"
#include "SphereRenderer.h"
#include "Camera.h"
#include "ObjectInstance.h"
#include "MergedSubmeshInstance.h"
#include "DebugRenderer.h"
#include "AssetLibrary.h"
#include "AssetLibraryOffline.h"

#include "Texture2D.h"

// Data
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

using namespace DirectX::SimpleMath;

namespace
{
	std::unique_ptr<DirectX::Texture2D> g_depthStencil = nullptr;

	std::shared_ptr<GpuConstants> g_GpuConstants = nullptr;
	// std::shared_ptr<std::vector<SubmeshInstance>> g_SubmeshIns = nullptr;
	// std::shared_ptr<std::vector<ObjectInstance>> g_ObjectIns = nullptr;
	// std::shared_ptr<std::vector<DividedSubmeshInstance>> g_DividedIns = nullptr;

	std::unique_ptr<Renderer> g_PlaneRender = nullptr;
	// std::unique_ptr<InstancingRenderer> g_MeshRender = nullptr;
	std::unique_ptr<SphereRenderer> g_SphereRenderer = nullptr;
	// std::unique_ptr<Renderer> g_DebugRender = nullptr;
	std::unique_ptr<Camera> g_Camera = nullptr;
	// std::unique_ptr<WorldSystem> g_WorldSystem = nullptr;
	//std::shared_ptr<AssetLibrary> g_AssetLibrary = nullptr;
	// std::shared_ptr<AssetLibrary> g_AssetLibraryOffline = nullptr;
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
	WNDCLASSEXW wc = {
		sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL
	};
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"World Streaming", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL,
		NULL, wc.hInstance, NULL);

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
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
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

		// static bool freezeFrustum = false;
		// static int loopCnt = 0;
		// static float timeSum = 0.0f;
		// static float timeAvg = 0.0f;
		//
		// auto begin = std::chrono::steady_clock::now();
		// auto [objIns, drawArgs] = g_WorldSystem->Tick(*g_Camera, freezeFrustum);
		// auto end = std::chrono::steady_clock::now();
		// *g_ObjectIns = std::move(objIns);
		// *g_DividedIns = std::move(drawArgs);
		// timeSum += std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
		// if (++loopCnt >= 100)
		// {
		// 	timeAvg = timeSum / loopCnt;
		// 	loopCnt = 0;
		// 	timeSum = 0.0f;
		// }
		// ImGui::Begin("Culling tick");
		// if (ImGui::Button("Freeze Frustum")) freezeFrustum = !freezeFrustum;
		// ImGui::SameLine();
		// static bool visualizeBs = false;
		// if (ImGui::Button("Visualize Bounding")) visualizeBs = !visualizeBs;
		// ImGui::Text("Object count : %d Visible count : %d Culling avg time : %2.0fus Frame rate : %2.0f FPS",
		// 	g_WorldSystem->GetObjectCount(), g_ObjectIns->size(), timeAvg, io.Framerate);
		// static int splitMethod = 0;
		// static int objectInNode = 128;
		//
		// bool changed = false;
		// changed |= ImGui::DragInt("Object in node", &objectInNode, 1, 1, INT32_MAX);
		// changed |= ImGui::RadioButton("Middle", &splitMethod, 0);
		// ImGui::SameLine();
		// changed |= ImGui::RadioButton("Equal Count", &splitMethod, 1);
		// ImGui::SameLine();
		// changed |= ImGui::RadioButton("Volume Heuristic", &splitMethod, 2);
		// if (changed)
		// 	g_WorldSystem->GenerateBvh(objectInNode, static_cast<BvhTree::SpitMethod>(splitMethod));

		//static int renderMode = 0;
		//ImGui::RadioButton("44 Byte Vertex", &renderMode, 0);
		//ImGui::SameLine();
		//ImGui::RadioButton("16 Byte Vertex", &renderMode, 1);
		//ImGui::SameLine();
		//ImGui::RadioButton("24 Byte Vertex", &renderMode, 2);

		//uint32_t bufferMb = renderMode == 0
		//	                    ? g_MeshRender->InstancingRenderer1::GetVertexBufferByteSize()
		//	                    : renderMode == 1
		//		                      ? g_MeshRender->InstancingRenderer2::GetVertexBufferByteSize()
		//		                      : g_MeshRender->InstancingRenderer3::GetVertexBufferByteSize();
		//bufferMb /= 1024 * 1024;
		//ImGui::Text("Vertex Buffer size : %d MB", bufferMb);

		// ImGui::End();

		// Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] = {
			clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w
		};
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, g_depthStencil->GetDsv());
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		g_pd3dDeviceContext->ClearDepthStencilView(g_depthStencil->GetDsv(),
			D3D11_CLEAR_DEPTH, 1.0f, 0);

		g_Camera->SetViewPort(g_pd3dDeviceContext);

		// g_PlaneRender->UpdateBuffer(g_pd3dDeviceContext);
		// g_PlaneRender->Render(g_pd3dDeviceContext);
		//
		// g_MeshRender->UpdateBuffer(g_pd3dDeviceContext);
		// g_MeshRender->Render(g_pd3dDeviceContext);

		//if (renderMode == 0)
		//{
		//	g_MeshRender->InstancingRenderer1::UpdateBuffer(g_pd3dDeviceContext);
		//	g_MeshRender->InstancingRenderer1::Render(g_pd3dDeviceContext);
		//}
		//else if (renderMode == 1)
		//{
		//	g_MeshRender->InstancingRenderer2::UpdateBuffer(g_pd3dDeviceContext);
		//	g_MeshRender->InstancingRenderer2::Render(g_pd3dDeviceContext);
		//}
		//else
		//{
		//	g_MeshRender->InstancingRenderer3::UpdateBuffer(g_pd3dDeviceContext);
		//	g_MeshRender->InstancingRenderer3::Render(g_pd3dDeviceContext);
		//}

		// if (visualizeBs)
		// {
		// 	g_DebugRender->UpdateBuffer(g_pd3dDeviceContext);
		// 	g_DebugRender->Render(g_pd3dDeviceContext);
		// }
		Matrix view = g_Camera->GetViewMatrix();
		Matrix proj = g_Camera->GetProjectionMatrix();
		g_SphereRenderer->Render(view, proj);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		//g_pSwapChain->Present(1, 0); // Present with vsync
		g_pSwapChain->Present(0, 0); // Present without vsync
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
	HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
		featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel,
		&g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, createDeviceFlags, featureLevelArray, 2,
			D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain)
	{
		g_pSwapChain->Release();
		g_pSwapChain = NULL;
	}
	if (g_pd3dDeviceContext)
	{
		g_pd3dDeviceContext->Release();
		g_pd3dDeviceContext = NULL;
	}
	if (g_pd3dDevice)
	{
		g_pd3dDevice->Release();
		g_pd3dDevice = NULL;
	}
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
	if (g_mainRenderTargetView)
	{
		g_mainRenderTargetView->Release();
		g_mainRenderTargetView = NULL;
	}
}

void InitWorldStreaming()
{
	g_Context.Initialize(4);
	g_GpuConstants = std::make_shared<GpuConstants>();

	// g_SubmeshIns = std::make_shared<std::vector<SubmeshInstance>>();
	// g_ObjectIns = std::make_shared<std::vector<ObjectInstance>>();
	// g_DividedIns = std::make_shared<std::vector<DividedSubmeshInstance>>();

	//g_AssetLibrary = std::make_shared<AssetLibrary>(L"./Asset/Mesh", g_GpuConstants);
	//g_AssetLibrary->Initialize();
	//
	// g_AssetLibraryOffline = std::make_shared<AssetLibraryOffline>();
	// g_AssetLibraryOffline->Initialize();

	g_PlaneRender = std::make_unique<PlaneRenderer>(g_pd3dDevice, g_GpuConstants);
	g_PlaneRender->Initialize(g_pd3dDeviceContext);
	//
	// g_MeshRender = std::make_unique<InstancingRenderer2>(g_pd3dDevice, g_GpuConstants, g_SubmeshIns, g_ObjectIns,
	// 	g_AssetLibraryOffline, g_DividedIns);
	// g_MeshRender->Initialize(g_pd3dDeviceContext);

	g_SphereRenderer = std::make_unique<SphereRenderer>(g_pd3dDevice);
	g_SphereRenderer->Initialize(g_pd3dDeviceContext);
	//
	// g_WorldSystem = std::make_unique<WorldSystem>(g_GpuConstants, g_AssetLibraryOffline);
	// g_WorldSystem->Initialize();
	//
	// g_DebugRender = std::make_unique<DebugRenderer>(g_pd3dDevice, g_GpuConstants, g_WorldSystem->GetBvhTree());
	// g_DebugRender->Initialize(g_pd3dDeviceContext);

	g_Camera = std::make_unique<Camera>();
}

void UpdateConstants(const ImGuiIO& io)
{
	g_Camera->Update(io);

	g_GpuConstants->View = g_Camera->GetViewMatrix().Transpose();
	g_GpuConstants->Proj = g_Camera->GetProjectionMatrix().Transpose();
	g_GpuConstants->ViewProj = g_Camera->GetViewProjectionMatrix().Transpose();
	g_GpuConstants->EyePosition = g_Camera->GetPosition();
	g_GpuConstants->DeltaTime = io.DeltaTime;
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
	case WM_SIZE: if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			CreateRenderTarget();
		}
		return 0;
	case WM_SYSCOMMAND: if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY: ::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
