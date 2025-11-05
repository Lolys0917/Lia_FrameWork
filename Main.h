#pragma once

#include <d3d11.h>

ID3D11Device* GetDevice();
ID3D11DeviceContext* GetContext();
IDXGISwapChain* GetSwapChain();
ID3D11RenderTargetView* GetRenderTargetView();
ID3D11Texture2D* GetDepthStencil();
ID3D11DepthStencilView* GetDepthStencilView();
int GetScreenWidth();
int GetScreenHeight();