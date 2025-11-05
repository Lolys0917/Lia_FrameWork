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

#include "ObjectManager.h"

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
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &scd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pImmediateContext
    );
    if (FAILED(hr)) return hr;

    // ================================
    // バックバッファ取得
    // ================================
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr)) return hr;

    // レンダーターゲットビュー作成
    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr)) return hr;

    // ================================
    // 深度バッファ (Zバッファ) 作成
    // ================================

    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = ScreenWidth;
    descDepth.Height = ScreenHeight;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pTexture2D);
    if (FAILED(hr)) return hr;

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

    hr = g_pd3dDevice->CreateDepthStencilView(g_pTexture2D, &descDSV, &g_pDepthStencilView);
    g_pTexture2D->Release();
    if (FAILED(hr)) return hr;

    // ================================
    // レンダーターゲットとZバッファをセット
    // ================================
    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

    // ================================
    // ビューポート設定
    // ================================
    D3D11_VIEWPORT vp = {};
    vp.Width = (FLOAT)ScreenWidth;
    vp.Height = (FLOAT)ScreenHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports(1, &vp);

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

    InitDo();

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
        GetContext()->ClearRenderTargetView(GetRenderTargetView(), clearColor);
        GetContext()->ClearDepthStencilView(GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        UpdateDo();
        DrawDo();
        // swap
        GetSwapChain()->Present(1, 0);
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
