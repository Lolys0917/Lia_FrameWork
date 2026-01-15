#define NOMINMAX
#include "GUI.h"
#include "Main.h"

#include <Windows.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

//====================================================
// 状態
//====================================================
static float g_LeftWidth = 400.0f;
static float g_Splitter = 5.0f;
static bool  g_ShowSolution = false;

// GameView 情報（外部連携用）
static ImVec2 g_GameViewPos{};
static ImVec2 g_GameViewSize{};

ImVec2 GetGameViewPos() { return g_GameViewPos; }
ImVec2 GetGameViewSize() { return g_GameViewSize; }

//====================================================
// 初期化
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
// 解放
//====================================================
void GUIRelease()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

//====================================================
// ポップアップ
//====================================================
static void ShowSolutionExplorer()
{
    if (!g_ShowSolution) return;

    ImGui::SetNextWindowSize(ImVec2(400, 120), ImGuiCond_Always);

    if (ImGui::Begin(
        "AddNewFile",
        &g_ShowSolution,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse))
    {
        ImGui::Text("Add New File");
        if (ImGui::Button("Close"))
            g_ShowSolution = false;
    }
    ImGui::End();
}

//====================================================
// 更新
//====================================================
bool GUIUpdate()
{
    /*ImGuiIO& io = ImGui::GetIO();
    static ImVec2 prev = { 0,0 };

    if (prev.x != io.DisplaySize.x || prev.y != io.DisplaySize.y)
    {
        char buf[128];
        sprintf_s(buf, "ImGui DisplaySize: %.0f x %.0f",
            io.DisplaySize.x, io.DisplaySize.y);
        OutputDebugStringA(buf);
        OutputDebugStringA("\n");
        prev = io.DisplaySize;
    }*/

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    //========================
    // メインウィンドウ
    //========================
    ImGuiViewport* vp = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);

    ImGui::Begin("MainWindow",
        nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus
    );

    //ImGui::SetWindowPos(vp->Pos);
    ImVec2 DispSize = ImGui::GetIO().DisplaySize;

    //ImGui::SetWindowSize(vp->Size);

    char buf[64];


    if (GetKeyState(VK_RETURN) < 0)
    {
        sprintf(buf, "%f", vp->Size.x);
        MessageBoxA(nullptr, "X", buf, NULL);
        sprintf(buf, "%f", vp->Size.y);
        MessageBoxA(nullptr, "Y", buf, NULL);
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float rightWidth = avail.x - g_LeftWidth - g_Splitter - 40.0f;

    //========================
    // 左カラム
    //========================
    ImGui::BeginChild("LeftColumn", ImVec2(g_LeftWidth, avail.y), true);

    float gameHeight = g_LeftWidth * 9.0f / 16.0f;
    ImGui::BeginChild("GameView", ImVec2(0, gameHeight), true);

    //GameView 情報取得
    g_GameViewPos = ImGui::GetCursorScreenPos();
    g_GameViewSize = ImGui::GetContentRegionAvail();

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
    // スプリッター
    //========================
    ImGui::SameLine(0, 0);
    ImGui::Button("##Splitter", ImVec2(g_Splitter, avail.y));
    if (ImGui::IsItemActive())
        g_LeftWidth += ImGui::GetIO().MouseDelta.x;

    ImGui::SameLine(0, 0);

    //========================
    // 右エリア
    //========================
    ImGui::BeginChild("RightArea", ImVec2(rightWidth, avail.y), true);

    if (ImGui::Button("Text Save")) {}
    ImGui::SameLine();
    if (ImGui::Button("Compile")) {}
    ImGui::SameLine();
    if (ImGui::Button("Start")) {}

    ImGui::Separator();
    ImGui::Text("Code Editor");

    ImGui::EndChild();

    //========================
    // 右縦タブ（Sボタン）
    //========================
    ImGui::SameLine(0, 0);
    ImGui::BeginChild("RightTabs", ImVec2(40.0f, avail.y), true);

    if (ImGui::Button("S", ImVec2(30, 30)))
        g_ShowSolution = !g_ShowSolution;

    ImGui::EndChild();

    ImGui::End(); // MainWindow

    ShowSolutionExplorer();

    ImGui::Render();
    return true;
}
