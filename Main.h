#pragma once

#include <d3d11.h>
#include "Manager.h"
#include "FileSystem.h"
#include "GUI.h"
#include <Windows.h>
// ImGui
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
// GLFW
#include <GLFW/glfw3.h>
#include <gl/GL.h>
// STL
#include <vector>
#include <memory>
#include <stdexcept>
#include <array>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <regex>
#include <chrono>
#include <set>

#include <Windows.h>
#include <DirectXMath.h>

#include <string>
#include <sstream>

#include <atomic>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>

#pragma comment (lib, "OpenGL32.lib")

ID3D11Device* GetDevice();
ID3D11DeviceContext* GetContext();
IDXGISwapChain* GetSwapChain();
ID3D11RenderTargetView* GetRenderTargetView();
ID3D11Texture2D* GetDepthStencil();
ID3D11DepthStencilView* GetDepthStencilView();

//Microsoft::WRL::ComPtr<ID3D11RenderTargetView> GetGameViewRTV();
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetGameViewSRV();
Microsoft::WRL::ComPtr<ID3D11Texture2D> GetGameViewDepthTex();
Microsoft::WRL::ComPtr<ID3D11DepthStencilView> GetGameViewDSV();
// BackBuffer
Microsoft::WRL::ComPtr<ID3D11Texture2D>        GetBackBufferTex();
Microsoft::WRL::ComPtr<ID3D11RenderTargetView> GetBackBufferRTV();
Microsoft::WRL::ComPtr<ID3D11DepthStencilView> GetBackBufferDSV();

void SetGameSize(int width, int height);

HWND GetHwnd();