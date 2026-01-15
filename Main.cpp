//====================================================
// Main.cpp
// DX11 + ImGui + GameView 完全構成
//====================================================

#include "Main.h"
#include "GUI.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <atomic>

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

//====================================================
// グローバル
//====================================================
HWND g_hwnd = nullptr;

static ComPtr<ID3D11Device>           g_Device;
static ComPtr<ID3D11DeviceContext>    g_Context;
static ComPtr<IDXGISwapChain>         g_SwapChain;

static ComPtr<ID3D11Texture2D>        g_BackBuffer;
static ComPtr<ID3D11RenderTargetView> g_BackBufferRTV;
static ComPtr<ID3D11DepthStencilView> g_BackBufferDSV;

// GameView
static ComPtr<ID3D11Texture2D>        g_GameViewTex;
static ComPtr<ID3D11RenderTargetView> g_GameViewRTV;
static ComPtr<ID3D11ShaderResourceView> g_GameViewSRV;
static ComPtr<ID3D11DepthStencilView> g_GameViewDSV;

int g_WindowWidth = 1280;
int g_WindowHeight = 720;

//====================================================
// Getter
//====================================================
ID3D11Device* GetDevice() { return g_Device.Get(); }
ID3D11DeviceContext* GetContext() { return g_Context.Get(); }
ComPtr<ID3D11ShaderResourceView> GetGameViewSRV() { return g_GameViewSRV; }

HWND GetHwnd()
{
    return g_hwnd;
}

//====================================================
// WndProc（完全版）
//====================================================
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void OnResize(UINT width, UINT height)
{
    if (!g_SwapChain) return;

    g_Context->OMSetRenderTargets(0, nullptr, nullptr);

    g_BackBufferRTV.Reset();
    g_BackBuffer.Reset();

    g_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

    g_SwapChain->GetBuffer(0, IID_PPV_ARGS(&g_BackBuffer));
    g_Device->CreateRenderTargetView(g_BackBuffer.Get(), nullptr, &g_BackBufferRTV);

    // ★ Viewport 更新
    D3D11_VIEWPORT vp{};
    vp.Width = (float)width;
    vp.Height = (float)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    g_Context->RSSetViewports(1, &vp);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            UINT w = LOWORD(lParam);
            UINT h = HIWORD(lParam);
            OnResize(w, h);
        }
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

//====================================================
// GameView 作成
//====================================================
void CreateGameView(int width, int height)
{
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    g_Device->CreateTexture2D(&desc, nullptr, &g_GameViewTex);
    g_Device->CreateRenderTargetView(g_GameViewTex.Get(), nullptr, &g_GameViewRTV);
    g_Device->CreateShaderResourceView(g_GameViewTex.Get(), nullptr, &g_GameViewSRV);

    desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ComPtr<ID3D11Texture2D> depth;
    g_Device->CreateTexture2D(&desc, nullptr, &depth);
    g_Device->CreateDepthStencilView(depth.Get(), nullptr, &g_GameViewDSV);
}

//====================================================
// D3D 初期化
//====================================================
bool InitD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferCount = 2;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;

    if (FAILED(D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &scd, &g_SwapChain,
        &g_Device, nullptr, &g_Context)))
        return false;

    g_SwapChain->GetBuffer(0, IID_PPV_ARGS(&g_BackBuffer));
    g_Device->CreateRenderTargetView(g_BackBuffer.Get(), nullptr, &g_BackBufferRTV);

    CreateGameView(1280, 720);
    return true;
}

//====================================================
// Main
//====================================================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{

    WNDCLASSEX wc{
        sizeof(WNDCLASSEX),
        CS_CLASSDC,
        WndProc,
        0, 0,
        hInst,
        nullptr, nullptr, nullptr, nullptr,
        "DX11Window",
        nullptr
    };
    // 1. Win32 Window 作成
    RegisterClassEx(&wc);

    g_hwnd = CreateWindow(
        wc.lpszClassName,
        "DX11 + ImGui",
        WS_OVERLAPPEDWINDOW,
        100, 100,
        g_WindowWidth, g_WindowHeight,
        nullptr, nullptr,
        wc.hInstance,
        nullptr
    );

    ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hwnd);

    // 2. DX 初期化（HWND 必須）
    InitD3D(g_hwnd);

    // 3. ImGui 初期化（HWND + Device 必須）
    GUIInit();

    MSG msg{};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        // ===== GameView =====
        {
            ID3D11RenderTargetView* rtv = g_GameViewRTV.Get();
            g_Context->OMSetRenderTargets(1, &rtv, g_GameViewDSV.Get());

            D3D11_VIEWPORT vp{};
            vp.Width = (float)1280;
            vp.Height = (float)720;
            vp.MinDepth = 0;
            vp.MaxDepth = 1;
            g_Context->RSSetViewports(1, &vp);

            float c[4] = { 0.2f,0.2f,0.4f,1 };
            g_Context->ClearRenderTargetView(rtv, c);
            g_Context->ClearDepthStencilView(g_GameViewDSV.Get(), D3D11_CLEAR_DEPTH, 1, 0);
        }

        // ===== BackBuffer =====
        {
            ID3D11RenderTargetView* bb = g_BackBufferRTV.Get();
            g_Context->OMSetRenderTargets(1, &bb, nullptr);

            RECT rc;
            GetClientRect(g_hwnd, &rc);

            D3D11_VIEWPORT vp{};
            vp.Width = (float)(rc.right - rc.left);
            vp.Height = (float)(rc.bottom - rc.top);
            vp.MinDepth = 0;
            vp.MaxDepth = 1;
            g_Context->RSSetViewports(1, &vp);

            float c[4] = { 0,0,0,1 };
            g_Context->ClearRenderTargetView(bb, c);
        }

        // ===== ImGui =====
        GUIUpdate();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_SwapChain->Present(1, 0);
    }

    GUIRelease();
    UnregisterClass(wc.lpszClassName, hInst);
    return 0;
}
