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

    LoadAllSavedFiles();
	MessageBoxA(nullptr, "GUI Initialized", "Info", MB_OK);
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
static void ShowSolutionExplorer(int WindowPosX, int WindowPosY)
{
    if (!g_ShowSolution) return;

    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2(WindowPosX, WindowPosY),
        ImGuiCond_Always
	);

    if (ImGui::Begin(
        "AddNewFile",
        &g_ShowSolution,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse))
    {
        DrawFileList();
        if(ImGui::Button("Add CPP")) OpenAddNewFileWindow(CodeFile::Type::Cpp);
		if (ImGui::Button("Add Header")) OpenAddNewFileWindow(CodeFile::Type::Header);
    }
    ImGui::End();
}

bool VerticalTextButton(
    const char* label,
    const ImVec2& size,
    const ImVec2& pos = ImVec2(-1, -1)   // ← 左回転（90度）
)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    if (pos.x >= 0 && pos.y >= 0)
        ImGui::SetCursorScreenPos(pos);

    ImGui::PushID(label);

    ImVec2 btn_pos = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton("##vbtn", size);

    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();

    ImDrawList* draw = ImGui::GetWindowDrawList();

    ImU32 bg_col =
        active ? IM_COL32(70, 70, 70, 255) :
        hovered ? IM_COL32(50, 50, 50, 255) :
        IM_COL32(35, 35, 35, 255);



    draw->AddRectFilled(
        btn_pos,
        ImVec2(btn_pos.x + size.x, btn_pos.y + size.y),
        bg_col,
        4.0f
    );

    //==========================
    // 縦文字描画（1文字ずつ）
    //==========================
    ImFont* font = ImGui::GetFont();
    float font_size = ImGui::GetFontSize();

    int len = (int)strlen(label);
    float total_height = len * font_size;

    ImVec2 text_pos(
        btn_pos.x + size.x * 0.5f - font_size * 0.5f,
        btn_pos.y + size.y * 0.5f - total_height * 0.5f
    );

    for (int i = 0; i < len; i++)
    {
        char c[2] = { label[i], 0 };

        draw->AddText(
            font,
            font_size,
            ImVec2(text_pos.x, text_pos.y + i * font_size),
            IM_COL32_WHITE,
            c
        );
    }

    ImGui::PopID();
    return pressed;
}


//
// コードエディタ
//

//----------------------------------------------------
// Token 定義
//----------------------------------------------------
enum TokenKind
{
    TOKEN_VARIABLE,
    TOKEN_FUNCTION
};

struct Token
{
    std::string name;
    TokenKind type;
    int defineCount;
};

struct SearchResult
{
    Token token;
    int score;
};

//----------------------------------------------------
// トークン判定ルール
//----------------------------------------------------
inline bool IsTokenStartChar(char c)
{
    return (c == ' ' || c == '\n' || c == '\r' || c == '\t');
}

inline bool IsTokenEndChar(char c)
{
    return (
        c == ' ' || c == ';' || c == '(' || c == '{' ||
        c == '=' || c == '\n' || c == '\r' || c == '\t'
        );
}

inline bool IsIdentifierHead(char c)
{
    return (isalpha((unsigned char)c) || c == '_');
}

//----------------------------------------------------
// saved 内 + Working.tmp をすべて読み取る
//----------------------------------------------------
static void LoadAllSourceText(std::string& outText)
{
    outText.clear();

    WIN32_FIND_DATAA fd{};
    HANDLE h = FindFirstFileA("saved\\*.*", &fd);
    if (h == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        const char* ext = strrchr(fd.cFileName, '.');

        bool valid =
            (ext && (!strcmp(ext, ".cpp") || !strcmp(ext, ".h"))) ||
            !strcmp(fd.cFileName, "Working.tmp");

        if (!valid)
            continue;

        std::ifstream ifs("saved\\" + std::string(fd.cFileName));
        if (!ifs.is_open())
            continue;

        outText.append(
            std::string(
                std::istreambuf_iterator<char>(ifs),
                std::istreambuf_iterator<char>()
            )
        );
        outText.push_back('\n');

    } while (FindNextFileA(h, &fd));

    FindClose(h);
}

//----------------------------------------------------
// トークン抽出（defineCount 集計・重複統合）
//----------------------------------------------------
static void ExtractTokensFromText(
    const char* text,
    std::vector<Token>& outTokens
)
{
    outTokens.clear();
    const int len = (int)strlen(text);

    for (int i = 0; i < len; i++)
    {
        if (i != 0 && !IsTokenStartChar(text[i - 1]))
            continue;

        if (!IsIdentifierHead(text[i]))
            continue;

        int start = i;
        while (i < len && !IsTokenEndChar(text[i]))
            i++;

        int end = i;
        if (end <= start)
            continue;

        std::string name(text + start, end - start);

        TokenKind kind = TOKEN_VARIABLE;
        if (end < len && text[end] == '(')
            kind = TOKEN_FUNCTION;

        bool found = false;
        for (auto& t : outTokens)
        {
            if (t.name == name && t.type == kind)
            {
                t.defineCount++;
                found = true;
                break;
            }
        }

        if (!found)
        {
            outTokens.push_back({ name, kind, 1 });
        }
    }
}

//----------------------------------------------------
// スコア計算（指定ルール）
//----------------------------------------------------
static int CalcScore(
    const std::string& src,
    const std::string& query,
    int defineCount
)
{
    if (src == query)
        return 40 + defineCount;

    int score = 0;

    // 文字一致 +5
    for (char c : query)
    {
        if (src.find(c) != std::string::npos)
            score += 5;
    }

    // 順序一致 +20
    size_t pos = 0;
    int order = 0;
    for (char c : query)
    {
        pos = src.find(c, pos);
        if (pos == std::string::npos)
            break;
        order++;
        pos++;
    }
    score += order * 20;

    // 定義数
    score += defineCount;

    return score;
}

//----------------------------------------------------
// 検索
//----------------------------------------------------
static void SearchTokens(
    const std::vector<Token>& tokens,
    const std::string& query,
    std::vector<SearchResult>& outResults
)
{
    outResults.clear();

    for (const auto& t : tokens)
    {
        int score = CalcScore(t.name, query, t.defineCount);
        if (score > 0)
            outResults.push_back({ t, score });
    }

    std::sort(outResults.begin(), outResults.end(),
        [](const SearchResult& a, const SearchResult& b)
        {
            return a.score > b.score;
        });
}

//----------------------------------------------------
// カーソル位置トークン抽出
//----------------------------------------------------
static bool ExtractTokenAtCursor(
    const std::string& text,
    int cursorPos,
    std::string& outToken,
    TokenKind& outKind
)
{
    if (cursorPos < 0 || cursorPos >= (int)text.size())
        return false;

    if (IsTokenStartChar(text[cursorPos]) || IsTokenEndChar(text[cursorPos]))
        return false;

    int start = cursorPos;
    int end = cursorPos;

    while (start > 0 && !IsTokenStartChar(text[start - 1]))
        start--;

    while (end < (int)text.size() && !IsTokenEndChar(text[end]))
        end++;

    if (start >= end)
        return false;

    outToken = text.substr(start, end - start);

    outKind = (end < (int)text.size() && text[end] == '(')
        ? TOKEN_FUNCTION
        : TOKEN_VARIABLE;

    return true;
}

//----------------------------------------------------
// デバッグ：カーソル検索（Shift + Q）
//----------------------------------------------------
static void DebugSearchAtCursor(
    const std::string& editorText,
    int cursorPos
)
{
    std::string query;
    TokenKind kind;

    if (!ExtractTokenAtCursor(editorText, cursorPos, query, kind))
        return;

    std::string allText;
    LoadAllSourceText(allText);

    std::vector<Token> tokens;
    ExtractTokensFromText(allText.c_str(), tokens);

    std::vector<SearchResult> results;
    SearchTokens(tokens, query, results);

    std::string msg;
    for (const auto& r : results)
    {
        msg += r.token.name;
        msg += (r.token.type == TOKEN_FUNCTION) ? " [Func] " : " [Var] ";
        msg += "Score:";
        msg += std::to_string(r.score);
        msg += "\n";
    }

    MessageBoxA(nullptr, msg.c_str(), "Search Debug", MB_OK);
}

//----------------------------------------------------
// サジェスト描画（右上固定）
//----------------------------------------------------
void DrawSearchSuggestWindow(
    const std::vector<SearchResult>& results,
    int maxDisplay = 10
)
{
    if (results.empty())
        return;

    ImGuiIO& io = ImGui::GetIO();

    //==============================
    // 右上固定
    //==============================
    const float padding = 10.0f;
    ImVec2 windowSize(320.0f, 220.0f);

    ImVec2 pos(
        io.DisplaySize.x - windowSize.x - padding,
        padding
    );

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

    ImGui::Begin(
        "Suggest",
        nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings
    );

    //==============================
    // ヘッダ
    //==============================
    ImGui::TextUnformatted("type | No / name");
    ImGui::Separator();

    //==============================
    // 内容
    //==============================
    int count = (int)results.size();
    if (count > maxDisplay)
        count = maxDisplay;

    for (int i = 0; i < count; i++)
    {
        const SearchResult& r = results[i];

        // 種別表示
        const char* typeStr =
            (r.token.type == TOKEN_FUNCTION) ? "Func" : "Var ";

        // 番号 (01, 02...)
        char noBuf[8];
        sprintf_s(noBuf, "%02d", i + 1);

        ImGui::Text(
            "%s | %s / %s",
            typeStr,
            noBuf,
            r.token.name.c_str()
        );
    }

    ImGui::End();

    ImGui::PopStyleVar(2);
}
//----------------------------------------------------
// GUI Update 用エントリ
//----------------------------------------------------
void UpdateSearchAtCursor(
    const std::string& editorText,
    int cursorPos
)
{
    if ((GetAsyncKeyState('Q') & 0x8000) &&
        (GetAsyncKeyState(VK_SHIFT) & 0x8000))
    {
        DebugSearchAtCursor(editorText, cursorPos);
    }
}

KeyMap g_UserVarMap;
KeyMap g_UserFuncMap;
static KeyMap g_UserMap;
static bool g_Built = false;

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

    //
    //検索エンジン
    //
    if (!g_Built)
    {
        KeyMap_Init(&g_UserVarMap);
        KeyMap_Init(&g_UserFuncMap);
        //LoadSavedFiles(&g_UserVarMap, &g_UserFuncMap);
        g_Built = true;
    }

    //DebugSearch(60);

    // Shift + Q
    /*if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
        (GetAsyncKeyState('Q') & 0x8000))
    {
        Debug_ShowKeyMap(&g_UserMap);
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


    /*if (GetKeyState(VK_RETURN) < 0)
    {
        sprintf(buf, "%f", vp->Size.x);
        MessageBoxA(nullptr, "X", buf, NULL);
        sprintf(buf, "%f", vp->Size.y);
        MessageBoxA(nullptr, "Y", buf, NULL);
    }*/

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

    /*if (ImGui::Button("Text Save")) {}
    ImGui::SameLine();
    if (ImGui::Button("Compile")) {}
    ImGui::SameLine();
    if (ImGui::Button("Start")) {}

    ImGui::Separator();
    ImGui::Text("Code Editor");*/


    //実装メモ
    //
    // コードエディタの下をポップアップに変更
    // スプリッターで上下サイズ変更可能処理追加
    // 
    //
    ShowCodeEditorUI();

    ImGui::EndChild();

    //========================
    // 右縦タブ（Sボタン）
    //========================
    ImGui::SameLine(0, 0);
    ImGui::BeginChild("RightTabs", ImVec2(40.0f, avail.y), true);

    /*if (ImGui::Button("S", ImVec2(20, 30)))
        g_ShowSolution = !g_ShowSolution;*/

    if(VerticalTextButton("FileList", ImVec2(30, 120)))
		g_ShowSolution = !g_ShowSolution;

    ImGui::EndChild();

    ImGui::End(); // MainWindow

    ShowSolutionExplorer(vp->Size.x - 450, 50);
    DrawAddNewFileWindow();

    ImGui::Render();
    return true;
}
