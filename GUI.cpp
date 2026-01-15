#define NOMINMAX
#include "GUI.h"
#include "Main.h"

#include <Windows.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

#include <vector>
#include <atomic>

static float g_LeftWidth = 400.0f;
static float g_Splitter = 5.0f;
static bool  g_ShowSolution = false;

static ImVec2 g_GameViewSize{};
static ImVec2 g_GameViewPos{};

ImVec2 GetGameViewSize() { return g_GameViewSize; }
ImVec2 GetGameViewPos() { return g_GameViewPos; }

//====================================================
// Init
//====================================================
bool GUIInit()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(GetHwnd());
    ImGui_ImplDX11_Init(GetDevice(), GetContext());

    return true;
}

//====================================================
// Release
//====================================================
void GUIRelease()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

//====================================================
// Update
//====================================================
bool GUIUpdate()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("MainWindow",
        nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove);

    ImGui::SetWindowPos(ImVec2(0, 0));
    ImGui::SetWindowSize(ImGui::GetIO().DisplaySize);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float rightWidth = avail.x - g_LeftWidth;

    //========================
    // Left
    //========================
    ImGui::BeginChild("Left", ImVec2(g_LeftWidth, avail.y), true);

    float gameHeight = g_LeftWidth * 9.0f / 16.0f;
    ImGui::BeginChild("GameView", ImVec2(0, gameHeight), true);

    g_GameViewSize = ImGui::GetContentRegionAvail();
    g_GameViewPos = ImGui::GetWindowPos();

    if(GetKeyState(VK_RETURN) & 0x8000)
    {
        MessageBoxA(NULL, std::to_string(g_GameViewSize.x).c_str(), "Width", MB_OK);
        MessageBoxA(NULL, std::to_string(g_GameViewSize.y).c_str(), "Height", MB_OK);
	}

    ImGui::Image(
        (ImTextureID)GetGameViewSRV().Get(),
        g_GameViewSize
    );

    ImGui::EndChild();

    ImGui::BeginChild("Inspector", ImVec2(0, 0), true);
    ImGui::Text("Inspector");
    ImGui::EndChild();

    ImGui::EndChild();

    //========================
    // Splitter
    //========================
    ImGui::SameLine();
    ImGui::Button("##Split", ImVec2(g_Splitter, avail.y));
    if (ImGui::IsItemActive())
        g_LeftWidth += ImGui::GetIO().MouseDelta.x;

    ImGui::SameLine();

    //========================
    // Right
    //========================
    ImGui::BeginChild("Right", ImVec2(rightWidth, avail.y), true);

    if (ImGui::Button("Save"))
    {

    }
    ImGui::SameLine();
    if (ImGui::Button("Compile"))
    {

    }
    ImGui::SameLine();
    if (ImGui::Button("Start"))
    {

    }

    ImGui::Separator();
    ImGui::Text("Code Editor");

    ImGui::EndChild();

    ImGui::End();

    ImGui::Render();
    return true;
}
