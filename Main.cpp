//プロジェクト概要
//ゲーム制作用のフレームワーク
// 
//基本機能
// オブジェクトの描画
// シェーダー作成＆適応
//

 //
//インクルード_____________
#include <Windows.h>
#include <DirectXMath.h>

#include <string>
#include <sstream>

#include "Main.h"

 //
//ライブラリ_______________
#pragma comment (lib, "d3d11.lib")


using namespace DirectX;

//グローバル
HWND g_hwnd = nullptr;
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11Texture2D* g_pTexture2D = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;

int ScreenWidth = 800;
int ScreenHeight = 600;

ID3D11Device* GetDevice() { return g_pd3dDevice; }
ID3D11DeviceContext* GetContext() { return g_pImmediateContext; }
IDXGISwapChain* GetSwapChain() { return g_pSwapChain; }
ID3D11RenderTargetView* GetRenderTargetView() { return g_pRenderTargetView; }
ID3D11Texture2D* GetDepthStencil() { return g_pTexture2D; }
ID3D11DepthStencilView* GetDepthStencilView() { return g_pDepthStencilView; }

int GetScreenWidth() { return ScreenWidth; }
int GetScreenHeight() { return ScreenHeight; }

// ウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
 //
//D3D初期化_______________
HRESULT InitD3D(HWND hWnd) {

    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = ScreenWidth;
    scd.BufferDesc.Height = ScreenHeight;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 0;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;

    // ✅ STEP.1 D3D11CreateDeviceAndSwapChain(HW)
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, nullptr, 0, D3D11_SDK_VERSION,
        &scd, &g_pSwapChain, &g_pd3dDevice,
        &featureLevel, &g_pImmediateContext
    );
    if (FAILED(hr)) {
        char msg[256];
        sprintf_s(msg, "STEP 1 failed: HW CreateDeviceAndSwapChain\nHRESULT = 0x%08X", hr);
        MessageBoxA(hWnd, msg, "Error", MB_OK | MB_ICONERROR);
    }

    // ✅ STEP.2 WARP fallback
    if (FAILED(hr)) {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            0, nullptr, 0, D3D11_SDK_VERSION,
            &scd, &g_pSwapChain, &g_pd3dDevice,
            &featureLevel, &g_pImmediateContext
        );
        if (FAILED(hr)) {
            char msg[256];
            sprintf_s(msg, "STEP 2 failed: WARP CreateDeviceAndSwapChain\nHRESULT = 0x%08X", hr);
            MessageBoxA(hWnd, msg, "Error", MB_OK | MB_ICONERROR);
            return hr;
        }
    }

    // ✅ STEP.3 バックバッファ
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
        (LPVOID*)&pBackBuffer);
    if (FAILED(hr)) {
        char msg[256];
        sprintf_s(msg, "STEP 3 failed: SwapChain->GetBuffer\nHRESULT = 0x%08X", hr);
        MessageBoxA(hWnd, msg, "Error", MB_OK | MB_ICONERROR);
        return hr;
    }

    // ✅ STEP.4 RenderTargetView
    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr,
        &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr)) {
        char msg[256];
        sprintf_s(msg, "STEP 4 failed: CreateRenderTargetView\nHRESULT = 0x%08X", hr);
        MessageBoxA(hWnd, msg, "Error", MB_OK | MB_ICONERROR);
        return hr;
    }

    return S_OK;
}

//D3D解放
void ReleaseD3D()
{
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
}

//Main
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) 
{

    MSG msg{};

    // ウィンドウクラス定義
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0, 0,
                      hInstance, nullptr, nullptr, nullptr, nullptr,
                      "DirectXWindowClass", nullptr };
    RegisterClassEx(&wc);

    // ウィンドウ作成
    g_hwnd = CreateWindowA(wc.lpszClassName, "LiaFrameWork",
        WS_OVERLAPPEDWINDOW, 100, 100, ScreenWidth, ScreenHeight,
        nullptr, nullptr, wc.hInstance, nullptr);

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    // Direct3D初期化
    if (FAILED(InitD3D(g_hwnd))) {
        DWORD error = GetLastError();
        wchar_t msg[256];
        swprintf_s(msg, L"D3D 初期化失敗\nError Code: %lu", error);
        MessageBoxW(g_hwnd, msg, L"Error", MB_OK | MB_ICONERROR);

        // デバッグ用 一旦止める（消えるのを防止）
        MessageBox(g_hwnd, "ウィンドウを閉じるには OK を押してください", "Debug Stop", MB_OK);

        return 0;
    }

    //更新処理
    // FPS 用変数（GetTickCount64 を使用）
    UINT64 lastTick = GetTickCount64();   // ms
    UINT64 lastTitleUpdateTick = lastTick;
    unsigned long frameCount = 0;
    const UINT64 updateIntervalMs = 1000; // タイトル更新間隔（ms） --- 1000ms = 1秒

    // 主ループ
    bool running = true;
    while (running) {
        // メッセージ処理（ノンブロッキング）
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!running) break;

        // --- レンダリング（ここに描画処理を入れる） ---
        const float clearColor[4] = { 0.1f, 0.2f, 0.3f, 1.0f };
        g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

        // swap
        g_pSwapChain->Present(1, 0);
        // -----------------------------------------------

        // FPS カウント
        ++frameCount;
        UINT64 now = GetTickCount64();
        UINT64 elapsedSinceUpdate = now - lastTitleUpdateTick;
        if (elapsedSinceUpdate >= updateIntervalMs) {
            double seconds = elapsedSinceUpdate / 1000.0;
            double fps = frameCount / seconds;

            // タイトル更新
            wchar_t title[256];
            swprintf_s(title, L"Tick64 FPS Sample - FPS: %.2f (%u frames / %.2f s)", fps, frameCount, seconds);
            SetWindowTextW(g_hwnd, title);

            // リセット
            lastTitleUpdateTick = now;
            frameCount = 0;
        }

        // オプション: CPU 使用率を落とすために少し Sleep を挟む（必要に応じて）
        // Sleep(0);
    }

    // 終了処理
    ReleaseD3D();
    UnregisterClass(wc.lpszClassName, hInstance);
}
