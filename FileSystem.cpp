#define NOMINMAX
#include "FileSystem.h"
#include "Manager.h"

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

#pragma comment (lib, "OpenGL32.lib")

namespace fs = std::filesystem;

// ______________________________________________________________ //
// ImVec2 加算オペレータ（ImGuiの2Dベクトルの足し算を便利にする） //
inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)
{
    return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

///////////////////////////////////////////////
// グローバル変数
///////////////////////////////////////////////

//DLLモジュール
static HMODULE DLL_Module = nullptr;

//ファンクションタイプ
using FuncType = void(*)();
FuncType Func;

static std::vector<CodeFile> g_Files;                   // 読み込んだファイル一覧
static int g_SelectedFileIndex = -1;                    // 現在選択中のファイルのインデックス
static const std::string g_SaveDir = "saved/";          // ファイル保存先ディレクトリ
static const std::string g_DllSaveDir = "saved/dll/";   // DLL保存先ディレクトリ
static std::string g_LastLoadedFileName;                // 最後にロードしたファイル名（再ロード防止用）
static std::string g_CompaileOut = "CompaileFiled";     // コンパイル結果文字列

static bool g_StopDLL;
static bool g_GameRunning;

// 基本的なキーワードなどを含むサジェスト用単語リスト
static std::vector<std::string> g_IntelliSenseWords =
{ "int", "float", "void", "if", "else", "for", "while", "return", "struct", "class",
  "enum", "static", "const", "std::vector", "std::string", "char", "return", "while",
  "std::", "vector", "string", "using", "struct", "typedef", "cout", "cin", "namespase",
  "include", "pragma once", "#include" };

// シンプルキーワードリスト（ifやforなど）
static const std::vector<std::string> keywords =
{ "if", "for", "while", "switch", "return", "else" , ",", "typedef" };


//ポップアップ用のフラグ
static bool show_solution_explorer = false;
static bool show_properties = false;
static bool show_project = false;

static std::vector<std::string> g_CurrentSuggestions; // 現在のサジェスト候補

static std::vector<std::string> g_IncludeFiles = {};

static bool g_ShowSuggestions = false; // サジェスト表示フラグ

// グローバル変数（ファイル上部などで定義）
int g_SelectedSuggestionIndex = -1;

static char buffer[65536] = { 0 }; // 入力テキストバッファ
static int g_CursorPos = 0;        // カーソル位置（バッファ内インデックス）

bool g_DebugMode = false; // 任意でtrueにすればデバッグ表示

static const std::string g_SuggestFile = "saved/suggest.txt";
static std::vector<std::string> g_Suggestions;

// ここから関数 //============================

// Shift-JISをUTF-8に変換する関数（Windows特有のコードページ変換）
// 引数: 変換する文字列
// 戻り値: UTF-8文字列
const char* ConvertToUTF8(const char* sent)
{
    static std::string result; // staticで返却用文字列を保持

    // ① Shift-JIS → UTF-16変換
    int wideLen = MultiByteToWideChar(CP_ACP, 0, sent, -1, NULL, 0);
    if (wideLen == 0) return "";

    std::wstring wideStr(wideLen, 0);
    MultiByteToWideChar(CP_ACP, 0, sent, -1, &wideStr[0], wideLen);

    // ② UTF-16 → UTF-8変換
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
    if (utf8Len == 0) return "";

    result.resize(utf8Len);
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &result[0], utf8Len, NULL, NULL);

    return result.c_str();
}
///////////////////////////////////////////////
// ファイル操作関連

//ファイル読み込み_____________
bool LoadCodeFile(const std::string& path, std::string& Content)
{
    //ファイルオープン
    std::ifstream file(path);
    if (!file.is_open()) return false;

    //イテレータで読み込み
    Content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return true;
}
//ファイル保存_________________
bool SaveCodeFile(const std::string& fileName, const std::string& content)
{
    namespace fs = std::filesystem;

    // 保存ディレクトリを用意
    std::string cppDir = g_SaveDir;             // cpp用
    std::string headerDir = g_SaveDir + "/dll";      // h用

    if (!fs::exists(cppDir)) fs::create_directory(cppDir);
    if (!fs::exists(headerDir)) fs::create_directory(headerDir);

    fs::path path(fileName);
    std::string ext = path.extension().string();

    //拡張子ごとに保存先を割り振る
    std::string savePath;

    if (ext == ".h")
    {
        savePath = headerDir + "/" + path.filename().string();
    }
    else if (ext == ".cpp")
    {
        savePath = cppDir + "/" + path.filename().string();
    }
    else
    {
        // 対応していない拡張子は cppDir に保存
        savePath = cppDir + "/" + path.filename().string();
    }

    std::cout << savePath << " : Save" << std::endl;

    std::ofstream file(savePath);
    if (!file.is_open()) return false;

    file.write(content.c_str(), content.size());
    return true;
}
// ファイル削除処理(インデックス指定)
void RemoveFile(int index)
{
    // インデックスの範囲チェック
    if (index < 0 || index >= (int)g_Files.size()) return;

    const CodeFile& file = g_Files[index];

    // 実際のファイルパスを作成
    std::string filePath = g_SaveDir + file.fileName;

    // ファイルが存在すれば削除
    if (fs::exists(filePath)) fs::remove(filePath);

    // CPPファイルなら対応するDLLも削除
    if (file.type == CodeFile::Type::Cpp)
    {
        std::string dllName = g_DllSaveDir + fs::path(file.fileName).replace_extension(".dll").string();
        if (fs::exists(dllName)) fs::remove(dllName);
    }

    // メモリ上のリストからも削除
    g_Files.erase(g_Files.begin() + index);

    // 選択ファイルのインデックス調整
    if (g_Files.empty()) g_SelectedFileIndex = -1;
    else if (index <= g_SelectedFileIndex) g_SelectedFileIndex = std::max(0, g_SelectedFileIndex - 1);
}
//ファイル追加________________________
void AddNewFile(CodeFile::Type type)
{
    int count = 0;
    std::string baseName = "NewFile";
    std::string ext = (type == CodeFile::Type::Header) ? ".h" : ".cpp";
    std::string fileName;

    // 同名ファイルが存在しないユニークなファイル名を作成
    do {
        fileName = baseName + std::to_string(count++) + ext;
    } while (std::any_of(g_Files.begin(), g_Files.end(), [&](const CodeFile& f) { return f.fileName == fileName; }));

    // 新しいファイル構造体作成
    CodeFile newFile;
    newFile.fileName = fileName;
    newFile.type = type;

    // ファイル内容初期化
    if (type == CodeFile::Type::Header)
        newFile.content = "#pragma once\n\n"; // ヘッダーならプラグマ once のみ
    else
        newFile.content = "#include <stdio.h>\n\nextern \"C\" __declspec(dllexport) void Run()\n{\n    printf(\"Hello from DLL!\\n\");\n}\n"; // CPPなら簡単なDLLコード

    // 保存用ディレクトリがなければ作成
    if (!fs::exists(g_SaveDir)) fs::create_directory(g_SaveDir);

    // ファイル保存
    if (type == CodeFile::Type::Header)
    {
        SaveCodeFile("saved/dll/" + fileName, newFile.content);
    }
    else
    {
        SaveCodeFile(g_SaveDir + fileName, newFile.content);
    }

    // グローバルリストに追加し、選択ファイルを新規に切り替え
    g_Files.push_back(newFile);
    g_SelectedFileIndex = (int)g_Files.size() - 1;
}
//全てのコードファイルを保存
//※ファイルがなかったときは作成する処理を含む
//※外部から追加ファイルがあったとき用に最初に一度処理する
void AllCodeSave()
{
    g_Files.clear();

    //保存用ディレクトリ
    std::string cppDir = g_SaveDir;
    std::string hDir = g_SaveDir + "dll/";

    //ディレクトリがない場合は作成する
    if (!fs::exists(cppDir)) fs::create_directory(cppDir);
    if (!fs::exists(hDir)) fs::create_directory(hDir);

    // cppDir を探索
    for (const auto& entry : fs::directory_iterator(cppDir))
    {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".cpp") continue; // cppのみ

        CodeFile file;
        file.fileName = entry.path().filename().string();
        file.type = CodeFile::Type::Cpp;
        LoadCodeFile(entry.path().string(), file.content);
        g_Files.push_back(file);
    }

    // headerDir を探索
    for (const auto& entry : fs::directory_iterator(hDir))
    {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".h") continue; // hのみ

        CodeFile file;
        file.fileName = entry.path().filename().string();
        file.type = CodeFile::Type::Header;
        LoadCodeFile(entry.path().string(), file.content);
        g_Files.push_back(file);
    }

    if (!g_Files.empty())
        g_SelectedFileIndex = 0;
}

// コマンド実行用関数（出力を文字列として取得）
std::string ExecCommand(const std::string& command)
{
    std::array<char, 128> buffer;
    std::string result;

    // パイプを使ってコマンドを実行し、読み取り専用でオープン
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");

    // パイプから読み取れる限り読み取り続ける
    while (fgets(buffer.data(), (int)buffer.size(), pipe) != nullptr)
    {
        result += buffer.data();
    }

    _pclose(pipe);
    return result;
}

//DLLコンパイル用のビジュアルスタジオパスを取得
std::string g_VcVarsAllPath;
bool ResolveVcVarsAll()
{
    const char* vswhere =
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe";

    if (!fs::exists(vswhere))
        return false;

    // installationPath を取得
    std::string cmd =
        std::string("\"") + vswhere +
        "\" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 "
        "-property installationPath";

    std::string installPath = ExecCommand(cmd);

    // 改行除去
    installPath.erase(
        std::remove(installPath.begin(), installPath.end(), '\r'),
        installPath.end());
    installPath.erase(
        std::remove(installPath.begin(), installPath.end(), '\n'),
        installPath.end());

    if (installPath.empty())
        return false;

    fs::path vcvars =
        fs::path(installPath) /
        "VC" / "Auxiliary" / "Build" / "vcvarsall.bat";

    if (!fs::exists(vcvars))
        return false;

    g_VcVarsAllPath = "\"" + vcvars.string() + "\"";

    //MessageBoxA(nullptr, g_VcVarsAllPath.c_str(), "VS_Path", NULL);

    return true;
}


//DLLコンパイラー
// まとめビルド: saved/*.cpp を全部リンクして 1 つの DLL を作る
bool CompileAllToSingleDll(const std::string& dllFileName)
{
    ResolveVcVarsAll();

    const std::string vcvarsall_path = g_VcVarsAllPath;

    // 保存先の確保
    if (!fs::exists(g_SaveDir))    fs::create_directories(g_SaveDir);
    if (!fs::exists(g_DllSaveDir)) fs::create_directories(g_DllSaveDir);

    // ① saved/ 配下の .cpp を収集
    std::vector<std::string> sources;
    for (const auto& entry : fs::directory_iterator(g_SaveDir))
    {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() == ".cpp")
            sources.push_back(entry.path().string());
    }

    if (sources.empty())
    {
        MessageBoxA(nullptr, "saved/ に .cpp がありません。", "Build All", MB_OK | MB_ICONWARNING);
        return false;
    }

    // ② レスポンスファイル（@file）に .cpp の一覧を書き出し（パスにスペースがあっても OK）
    //    ※ response file のパス自体はスペース無しにしておく
    std::string rspPath = g_SaveDir + "__all_sources.rsp"; // 例: saved/__all_sources.rsp
    {
        std::ofstream rsp(rspPath, std::ios::binary);
        for (const auto& s : sources)
        {
            rsp << '\"' << s << '\"' << "\r\n";
        }
    }

    // ③ 出力 DLL パス
    std::string outDll = g_DllSaveDir + dllFileName;

    // ④ cl.exe コマンドを作成
    //  - /LD              : DLL を生成
    //  - /EHsc            : 例外ハンドリング
    //  - /utf-8           : UTF-8 ソース（日本語コメント対策）
    //  - /I saved         : saved をインクルード検索パスに追加（ヘッダー参照用）
    //  - @saved\*.rsp     : 収集した .cpp を一括指定
    //  - /Fe:...          : DLL 出力名
    //  - /link ...        : リンカオプション
    std::ostringstream oss;
    oss << "cmd /c \"call " << vcvarsall_path << " x64 && "
        << "cl /nologo /LD /EHsc /utf-8 "
        << "/I\"" << g_SaveDir << "\" "
        << "@\"" << rspPath << "\" "
        << "/Fe:\"" << outDll << "\" "
        << "/link /INCREMENTAL:NO "
        << "\"saved/dll/Lia_FrameWork.lib\"\"";

    std::string command = oss.str();

    // ⑤ 実行 & ログ取得（今の仕組みに合わせる）
    g_CompaileOut = ExecCommand(command);

    if (!g_DebugMode)
    {
        // 失敗時ログも拾えるようにファイルにも吐いておく
        std::string redirectCmd = command + " > saved\\__compile_log_all.txt 2>&1";
        STARTUPINFOA si = {};
        PROCESS_INFORMATION pi = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        std::string cmdLine = "cmd /C " + redirectCmd;
        BOOL ok = CreateProcessA(nullptr, cmdLine.data(), nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        if (ok)
        {
            WaitForSingleObject(pi.hProcess, INFINITE);
            DWORD exitCode = 0;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            if (exitCode != 0)
            {
                std::ifstream log("saved\\__compile_log_all.txt");
                std::ostringstream oss2; oss2 << log.rdbuf();
                g_CompaileOut = oss2.str();
                MessageBoxA(nullptr, "まとめビルドに失敗しました。ログを確認してください。", "Build All", MB_OK | MB_ICONERROR);
                return false;
            }
        }
    }

    return fs::exists(outDll);
}

bool LoadGameDll(const std::string& dllName)
{
    if (g_LastLoadedFileName == dllName) return true; // 同じDLLなら再ロードしない
    g_LastLoadedFileName = dllName;
    if (DLL_Module)
    {
        FreeLibrary(DLL_Module);
        DLL_Module = nullptr;
    }
    DLL_Module = LoadLibraryA(dllName.c_str());
    if (!DLL_Module)
    {
        MessageBoxA(nullptr, "DLLのロードに失敗しました", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}
bool DeleteGameDll()
{
    if (DLL_Module)
    {
        FreeLibrary(DLL_Module);
        DLL_Module = nullptr;
    }
    return true;
}
bool RunGameRelease(const std::string& dllName)
{
    //ReleaseDo();
    Func = (FuncType)GetProcAddress(DLL_Module, "Release");
    if (Func)
    {
        Func(); // 実行
    }
    FreeLibrary(DLL_Module);
    return true;
}
bool RunGameInit(const std::string& dllName)
{
    //InitDo();
    Func = (FuncType)GetProcAddress(DLL_Module, "Init");
    if (Func)
    {
        Func(); // 実行
    }
    return true;
}
bool RunGameUpdate(const std::string& dllName)
{
    MSG msg = {};

    if (GetKeyState(VK_ESCAPE) < 0 || g_StopDLL)
    {
        DeleteGameDll();

        g_StopDLL = false;
        g_GameRunning = false;
        return false;
    }
    /*if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }*/

    Func = (FuncType)GetProcAddress(DLL_Module, "Update");
    if (Func)
    {
        Func(); // 実行
    }

    return true;
}

static bool RunStart = false;
static bool Running = false;
void SetRunStart()
{
    RunStart = true;
    Running = true;
}
void SetRunEnd()
{
    DeleteGameDll();
    RunStart = false;
    Running = false;
}
void RunningGame()
{
    if (RunStart)
    {
        RunGameInit("user.dll");
        RunStart = false;
    }
    
    if (Running)
    {
        RunGameUpdate("user.dll");
    }
}

//ゲームの最初のフレームだけ実行
void GameStartDraw()
{
    
}



// グローバル/ファイル頭で定義
static bool g_ShowNewFileWindow = false;
static CodeFile::Type g_NewFileType;
static char g_NewFileName[128] = "";


// 新しいファイル追加の呼び出し（ボタンなどから呼ぶ）
void OpenAddNewFileWindow(CodeFile::Type type)
{
    g_ShowNewFileWindow = true;
    g_NewFileType = type;
    g_NewFileName[0] = '\0';
}

// ファイル保存関数
bool SaveFileContent(const std::string& fileName, const std::string& content)
{
    namespace fs = std::filesystem;

    // 保存ディレクトリを用意
    std::string cppDir = g_SaveDir;             // cpp用
    std::string headerDir = g_SaveDir + "/dll";      // h用

    if (!fs::exists(cppDir)) fs::create_directory(cppDir);
    if (!fs::exists(headerDir)) fs::create_directory(headerDir);

    fs::path path(fileName);
    std::string ext = path.extension().string();

    // 保存先を拡張子で切り替え
    std::string savePath;
    if (ext == ".h")
    {
        savePath = headerDir + "/" + path.filename().string();
    }
    else if (ext == ".cpp")
    {
        savePath = cppDir + "/" + path.filename().string();
    }
    else
    {
        // 対応していない拡張子は cppDir に保存
        savePath = cppDir + "/" + path.filename().string();
    }

    std::cout << savePath << " : Save" << std::endl;

    std::ofstream file(savePath);
    if (!file.is_open()) return false;

    file.write(content.c_str(), content.size());
    return true;
}
// ===============================
// ファイル読み込み関数
bool LoadFileContent(const std::string& path, std::string& outContent)
{
    std::ifstream file(path);
    if (!file.is_open()) return false; // ファイルオープン失敗時はfalseを返す

    // ストリームイテレータを使って一気にファイル全体を文字列に読み込み
    outContent.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return true;
}
void DrawAddNewFileWindow()
{
    if (!g_ShowNewFileWindow)
        return;

    ImGui::SetNextWindowSize(ImVec2(400, 120), ImGuiCond_Appearing);
    if (ImGui::Begin("AddNewFile", &g_ShowNewFileWindow, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("NewFileNameSetting / No filetype setting required");
        ImGui::InputText("##FileName", g_NewFileName, IM_ARRAYSIZE(g_NewFileName));

        if (ImGui::Button("OK"))
        {
            std::string fileNameStr = g_NewFileName;
            if (fileNameStr.empty())
                fileNameStr = "NewFile";

            std::string ext = (g_NewFileType == CodeFile::Type::Header) ? ".h" : ".cpp";
            int count = 0;

            std::string finalName;
            do {
                finalName = fileNameStr + ((count > 0) ? std::to_string(count) : "") + ext;
                count++;
            } while (std::any_of(g_Files.begin(), g_Files.end(),
                [&](const CodeFile& f) { return f.fileName == finalName; }));

            CodeFile newFile;
            newFile.fileName = finalName;
            newFile.type = g_NewFileType;

            if (g_NewFileType == CodeFile::Type::Header)
                newFile.content = "#pragma once\n\n";
            else
                newFile.content =
                "A project should have only one Init, Update, Draw, and Release.";

            if (!fs::exists(g_SaveDir))
                fs::create_directory(g_SaveDir);

            std::string savePath = (g_NewFileType == CodeFile::Type::Header) ? "saved/dll/" + finalName : g_SaveDir + finalName;
            SaveFileContent(savePath, newFile.content);

            g_Files.push_back(newFile);
            g_SelectedFileIndex = static_cast<int>(g_Files.size()) - 1;

            g_ShowNewFileWindow = false; // ウィンドウ閉じる
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            g_ShowNewFileWindow = false;
        }

        ImGui::End();
    }
}
void LoadAllSavedFiles()
{
    g_Files.clear();

    // 保存ディレクトリ
    std::string cppDir = g_SaveDir;             // cpp用 (例: ./saved)
    std::string headerDir = g_SaveDir + "dll/";   // ヘッダ用 (例: ./saved/h)

    // ディレクトリがなければ作成
    if (!fs::exists(cppDir)) fs::create_directory(cppDir);
    if (!fs::exists(headerDir)) fs::create_directory(headerDir);

    // cppDir を探索
    for (const auto& entry : fs::directory_iterator(cppDir))
    {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".cpp") continue; // cppのみ

        CodeFile file;
        file.fileName = entry.path().filename().string();
        file.type = CodeFile::Type::Cpp;
        LoadFileContent(entry.path().string(), file.content);
        g_Files.push_back(file);
    }

    // headerDir を探索
    for (const auto& entry : fs::directory_iterator(headerDir))
    {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".h") continue; // hのみ

        CodeFile file;
        file.fileName = entry.path().filename().string();
        file.type = CodeFile::Type::Header;
        LoadFileContent(entry.path().string(), file.content);
        g_Files.push_back(file);
    }

    if (!g_Files.empty())
        g_SelectedFileIndex = 0;
}


// バッファのカーソル位置から現在の単語を抽出する関数
std::string ExtractWordAtCursor(const char* buf, int cursorPos)
{
    if (!buf || cursorPos <= 0) return "";

    // カーソル位置から左に単語の開始位置を探す
    int start = cursorPos - 1;
    while (start >= 0 && (isalnum((unsigned char)buf[start]) || buf[start] == '_')) start--;
    start++; // 開始位置は単語の最初に戻す

    // カーソル位置から右に単語の終了位置を探す
    int end = cursorPos;
    while (buf[end] && (isalnum((unsigned char)buf[end]) || buf[end] == '_')) end++;

    return std::string(buf + start, buf + end);
}
// サジェスト候補を更新する関数
void UpdateSuggestions(const std::string& input)
{
    g_CurrentSuggestions.clear();
    if (input.empty()) return;

    auto ToLower = [](const std::string& str) -> std::string {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return lower;
        };

    std::string lowerInput = ToLower(input);
    std::set<std::string> uniqueSuggestions;

    // ① キーワード一覧から
    for (const auto& word : g_IntelliSenseWords)
    {
        if (ToLower(word).find(lowerInput) == 0)
        {
            uniqueSuggestions.insert(word);
            if (uniqueSuggestions.size() >= 10) break;
        }
    }

    // ② buffer内から関数・変数を抽出
    std::string source(buffer);
    std::regex re(R"(\b([a-zA-Z_][a-zA-Z0-9_]*)[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)(\s*\([^;{]*\))?)");
    std::smatch match;
    auto it = source.cbegin();

    static const std::vector<std::string> keywords = { "if", "for", "while", "switch", "return", "else" };

    while (std::regex_search(it, source.cend(), match, re))
    {
        std::string type = match[1].str();
        std::string name = match[2].str();
        std::string args = match[3].str();

        if (std::find(keywords.begin(), keywords.end(), type) != keywords.end())
        {
            it = match.suffix().first;
            continue;
        }

        std::string suggestion = (!args.empty()) ? (name + args) : (type + " " + name);
        if (ToLower(suggestion).find(lowerInput) != std::string::npos)
        {
            uniqueSuggestions.insert(suggestion);
            if (uniqueSuggestions.size() >= 10) break;
        }

        it = match.suffix().first;
    }

    // ③ #include "..." または <...> のファイルパスを使ってファイル内容からサジェスト
    std::regex reInclude(R"(#include\s*[<"]([^">]+)[>"])");
    std::smatch includeMatch;
    auto it2 = source.cbegin();

    while (std::regex_search(it2, source.cend(), includeMatch, reInclude))
    {
        std::string filePath = includeMatch[1].str();

        // サジェストとしてファイル名表示
        if (ToLower(filePath).find(lowerInput) != std::string::npos)
        {
            uniqueSuggestions.insert(filePath);
            if (uniqueSuggestions.size() >= 10) break;
        }

        // ファイルの実在確認と読み込み
        // 修正点：保存先を指定（"saved/"ディレクトリ）
        std::string fullPath = g_SaveDir + filePath;
        if (std::filesystem::exists(fullPath))
        {
            std::ifstream file(fullPath);
            if (file.is_open())
            {
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                auto it3 = content.cbegin();

                while (std::regex_search(it3, content.cend(), match, re))
                {
                    std::string type = match[1].str();
                    std::string name = match[2].str();
                    std::string args = match[3].str();

                    if (std::find(keywords.begin(), keywords.end(), type) != keywords.end())
                    {
                        it3 = match.suffix().first;
                        continue;
                    }

                    std::string suggestion = (!args.empty()) ? (name + args) : (type + " " + name);
                    if (ToLower(suggestion).find(lowerInput) != std::string::npos)
                    {
                        uniqueSuggestions.insert(suggestion);
                        if (uniqueSuggestions.size() >= 10) break;
                    }

                    it3 = match.suffix().first;
                }
            }
        }

        it2 = includeMatch.suffix().first;
    }

    // 最終的な出力
    g_CurrentSuggestions.assign(uniqueSuggestions.begin(), uniqueSuggestions.end());
}

void DrawFileList()
{
    for (int i = 0; i < (int)g_Files.size(); i++)
    {
        ImGui::PushID(i);

        bool isSelected = (g_SelectedFileIndex == i);
        if (ImGui::Selectable(g_Files[i].fileName.c_str(), isSelected))
            g_SelectedFileIndex = i;

        // =========================
        // 右クリックメニュー
        // =========================
        if (ImGui::BeginPopupContextItem()) // ← 右クリック
        {
            if (ImGui::MenuItem("Delete"))
            {
                RemoveFile(i);
                ImGui::EndPopup();
                ImGui::PopID();
                break;
            }

            if (ImGui::MenuItem("Rename"))
            {
                // Rename 処理

            }

            ImGui::EndPopup();
        }

        ImGui::PopID();
    }
}
struct EditorState
{
    std::string text;
    int cursorPos = 0;
};

static int EditorInputCallback(ImGuiInputTextCallbackData* data)
{
    EditorState* state = (EditorState*)data->UserData;

    state->cursorPos = data->CursorPos;
    state->text = data->Buf;

    return 0;
}
//----------------------------------------------------
// サジェスト描画（右上固定）
//----------------------------------------------------
void DrawSearchSuggestWindow(
    const std::vector<SearchResult>& results,
    int maxDisplay = 5
)
{
    //if (results.empty())
    //    return;

    ImGuiIO& io = ImGui::GetIO();

    //==============================
    // 右上固定
    //==============================
    const float paddingX = 50.0f;
    const float paddingY = 60.0f;
    ImVec2 windowSize(320.0f, 220.0f);

    ImVec2 pos(
        io.DisplaySize.x - windowSize.x - paddingX, paddingY);

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

    ImGui::Begin(
        "Suggest",
        nullptr,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing
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
        if(!results.empty())
        {
            ImGui::Text(
                "%s | %s / %s",
                typeStr,
                noBuf,
                r.token.name.c_str()
            );
        }
    }

    ImGui::End();

    ImGui::PopStyleVar(2);
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
//====================================================
// サジェストをカーソル位置に挿入
//====================================================
static void InsertSuggestionAtCursor(
    char* buffer,
    int bufferSize,
    int& cursorPos,
    const std::string& insertText
)
{
    std::string text = buffer;

    // 左側の単語開始を探す
    int start = cursorPos;
    while (start > 0 && (isalnum((unsigned char)text[start - 1]) || text[start - 1] == '_'))
        start--;

    // 置換
    text.replace(start, cursorPos - start, insertText);

    // バッファへ反映
    strncpy(buffer, text.c_str(), bufferSize - 1);
    buffer[bufferSize - 1] = '\0';

    cursorPos = start + (int)insertText.size();
}
//====================================================
// Ctrl + 数字キーでサジェスト確定
//====================================================
static void HandleSuggestConfirm(
    char* buffer,
    int bufferSize,
    int& cursorPos,
    const std::vector<SearchResult>& results,
    int maxCount = 5
)
{
    if (!(GetAsyncKeyState(VK_CONTROL) & 0x8000))
        return;

    int count = std::min((int)results.size(), maxCount);
    
    for (int i = 0; i < count; i++)
    {
        int key = '1' + i;
        if (GetAsyncKeyState(key) & 0x8000)
        {
            InsertSuggestionAtCursor(
                buffer,
                bufferSize,
                cursorPos,
                results[i].token.name
            );
            break;
        }
    }
}
static void ReplaceTokenAtCursor(
    char* buffer,
    int bufferSize,
    int& cursorPos,
    const std::string& newWord
)
{
    if (!buffer) return;

    int len = (int)strlen(buffer);
    if (cursorPos <= 0 || cursorPos > len) return;

    // 左へ
    int start = cursorPos - 1;
    while (start >= 0 &&
        (isalnum((unsigned char)buffer[start]) || buffer[start] == '_'))
        start--;
    start++;

    // 右へ
    int end = cursorPos;
    while (end < len &&
        (isalnum((unsigned char)buffer[end]) || buffer[end] == '_'))
        end++;

    std::string before(buffer, start);
    std::string after(buffer + end);

    std::string result = before + newWord + after;

    strncpy_s(buffer, bufferSize, result.c_str(), _TRUNCATE);

    cursorPos = start + (int)newWord.size();
}
struct WordRange
{
    int start;
    int end;
};

static bool GetWordRangeAtCursor(
    const char* buf,
    int cursorPos,
    WordRange& outRange
)
{
    if (!buf || cursorPos <= 0)
        return false;

    int len = (int)strlen(buf);
    if (cursorPos > len)
        cursorPos = len;

    int start = cursorPos - 1;
    while (start >= 0 &&
        (isalnum((unsigned char)buf[start]) || buf[start] == '_'))
    {
        start--;
    }
    start++;

    int end = cursorPos;
    while (end < len &&
        (isalnum((unsigned char)buf[end]) || buf[end] == '_'))
    {
        end++;
    }

    if (start >= end)
        return false;

    outRange.start = start;
    outRange.end = end;
    return true;
}
static void ApplySuggestionByIndex(
    int index,
    const std::vector<SearchResult>& results,
    char* buffer,
    int& cursorPos
)
{
    if (index < 0 || index >= (int)results.size())
        return;

    WordRange range;
    if (!GetWordRangeAtCursor(buffer, cursorPos, range))
        return;

    std::string before(buffer, buffer + range.start);
    std::string after(buffer + range.end);

    const std::string& insert = results[index].token.name;

    std::string newText = before + insert + after;

    strncpy(buffer, newText.c_str(), 65535);
    buffer[65535] = '\0';

    cursorPos = range.start + (int)insert.size();
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

static bool  g_ShowNamePopup = false;
static bool  g_netStart = false;
static char  g_NameInput[128] = "";
bool GetNetStart()
{
    return g_netStart;
}
void ShowCodeEditorUI()
{
    /*ImGui::BeginChild("FileList", ImVec2(200, 0), true);
    ImGui::Text("Files:");
    ImGui::Separator();

    for (int i = 0; i < (int)g_Files.size(); i++)
    {
        ImGui::PushID(i);
        bool isSelected = (g_SelectedFileIndex == i);
        if (ImGui::Selectable(g_Files[i].fileName.c_str(), isSelected))
            g_SelectedFileIndex = i;
        ImGui::SameLine();
        if (ImGui::Button("Del")) { RemoveFile(i); ImGui::PopID(); break; }
        ImGui::PopID();
    }

    ImGui::Separator();
    if (ImGui::Button("Add Header")) OpenAddNewFileWindow(CodeFile::Type::Header);
    if (ImGui::Button("Add CPP")) OpenAddNewFileWindow(CodeFile::Type::Cpp);
    ImGui::EndChild();

    ImGui::SameLine();*/

    ImGui::BeginGroup();
    ImGui::Text("Editor:");

    if (g_SelectedFileIndex >= 0)
    {
        auto& file = g_Files[g_SelectedFileIndex];
        if (g_LastLoadedFileName != file.fileName)
        {
            strncpy(buffer, file.content.c_str(), sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';
            g_LastLoadedFileName = file.fileName;
        }

        ImVec2 availSize = ImGui::GetContentRegionAvail();
        float editorHeight = availSize.y * 0.6f;

        if (ImGui::Button("Text Save"))
            SaveFileContent(g_SaveDir + file.fileName, file.content);

		// 自動保存処理
		static int AutoSaveTextCount = 0;
		static int AutoSaveTextInterval = 300; // 300フレームごとに自動保存

        if(AutoSaveTextCount >= AutoSaveTextInterval)
        {
            SaveFileContent(g_SaveDir + file.fileName, file.content);
            AutoSaveTextCount = 0;
		}
		AutoSaveTextCount++;

		// ボタン群
        ImGui::SameLine();
		// DLLコンパイル＆実行ボタン
        if (ImGui::Button("DLL Compile") && file.type == CodeFile::Type::Cpp)
        {
            SaveFileContent(g_SaveDir + file.fileName, file.content);
            std::string dllName = fs::path(file.fileName).replace_extension(".dll").string();
            //ReleaseDo();
            RunGameRelease("saved/dll/user.dll");

            CompileAllToSingleDll("user");

            LoadGameDll("saved/dll/user.dll");

            GameStartDraw();

            DeleteGameDll();
        }
        ImGui::SameLine();
		// DLL実行＆停止ボタン
        if (ImGui::Button("Run DLL"))
        {
            LoadGameDll("saved/dll/user.dll");

            RunGameInit("saved/dll/user.dll");
            g_GameRunning = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop DLL"))
        {
            g_StopDLL = true;
        }
        /////
		//
        if (ImGui::Button("Host start"))
        {
            g_netStart = true;
            InitNet(1);
            Net_AddSendStruct("player", { 0,0,0,0 });
			MessageBoxA(NULL, GetLocalIP().c_str(), "Hosting Start", MB_OK);
        }

        if (ImGui::Button("Input IP"))
        {
			g_netStart = true;
            g_ShowNamePopup = true;
            ImGui::OpenPopup("NameInputPopup");
            Net_AddSendStruct("player", { 0,0,0,0 });
        }
        if (ImGui::BeginPopupModal("NameInputPopup",
            NULL,
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("プレイヤー名を入力してください");

            // 入力欄
            ImGui::InputText(
                "##PlayerName",
                g_NameInput,
                sizeof(g_NameInput)
            );

            ImGui::Spacing();

            //----------------------------------------
            // OKボタン
            //----------------------------------------
            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                printf("input: %s\n", g_NameInput);
                SetIP(g_NameInput);
                InitNet(2);
                
                g_ShowNamePopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            //----------------------------------------
            // キャンセル
            //----------------------------------------
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                g_ShowNamePopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        ImGui::BeginChild("EditorChild", ImVec2(0, editorHeight), true, ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs);

        ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
        if (ImGui::InputTextMultiline("##source", buffer, sizeof(buffer), ImVec2(-1, -1), flags))
        {
            file.content = buffer;
        }

        // --- カーソル位置検索取得 ---
        static std::vector<Token> tokens;
        static std::vector<SearchResult> results;

        tokens.clear();
        ExtractTokensFromText(buffer, tokens);

        std::string query;
        TokenKind kind;

        if (ExtractTokenAtCursor(buffer, g_CursorPos - 1, query, kind))
        {
            SearchTokens(tokens, query, results);
        }
        else
        {
            results.clear();
        }

        //==============================
        // サジェスト描画
        //==============================
        //for (int i = 0; i < (int)results.size() && i < 5; i++)
        //{
        //    if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
        //        (GetAsyncKeyState('1' + i) & 0x8000))
        //    {
        //        ReplaceTokenAtCursor(
        //            buffer,
        //            sizeof(buffer),
        //            g_CursorPos,
        //            results[i].token.name
        //        );

        //        // file.content にも反映
        //        g_Files[g_SelectedFileIndex].content = buffer;
        //        break;
        //    }
        //}
        //HandleSuggestConfirm(                   //サジェスト適応
        //    buffer,
        //    sizeof(buffer),
        //    g_CursorPos,
        //    results,
        //    5
        //);
        
        DrawSearchSuggestWindow(results, 5);    //描画

        // Ctrl + 1〜5 で確定
        if (ImGui::GetIO().KeyCtrl)
        {
            bool TokenAdd = false;

            if (ImGui::IsKeyPressed(ImGuiKey_1)) { ApplySuggestionByIndex(0, results, buffer, g_CursorPos); TokenAdd = true; }
            if (ImGui::IsKeyPressed(ImGuiKey_2)) { ApplySuggestionByIndex(1, results, buffer, g_CursorPos); TokenAdd = true; }
            if (ImGui::IsKeyPressed(ImGuiKey_3)) { ApplySuggestionByIndex(2, results, buffer, g_CursorPos); TokenAdd = true; }
            if (ImGui::IsKeyPressed(ImGuiKey_4)) { ApplySuggestionByIndex(3, results, buffer, g_CursorPos); TokenAdd = true; }
            if (ImGui::IsKeyPressed(ImGuiKey_5)) { ApplySuggestionByIndex(4, results, buffer, g_CursorPos); TokenAdd = true; }

            if(TokenAdd)
            {
                ImGui::SetKeyboardFocusHere(-1);
                g_Files[g_SelectedFileIndex].content = buffer; // ← 忘れず反映
                ImGui::ClearActiveID();
                TokenAdd = false;
            }
        }


        //UpdateSearchAtCursor(buffer, g_CursorPos);


        
        ImGuiID id = ImGui::GetID("##source");
        ImGuiInputTextState* state = ImGui::GetInputTextState(id);
        if (state)
            g_CursorPos = state->GetCursorPos();

        //// === サジェスト欄（スクロール付き） ===
        //std::string currentWord = ExtractWordAtCursor(buffer, g_CursorPos);
        //UpdateSuggestions(currentWord);

        //if (!g_CurrentSuggestions.empty())
        //{
        //    ImVec2 suggestBoxPos = ImGui::GetItemRectMin() + ImVec2(ImGui::GetItemRectSize().x - 220, 0);
        //    ImGui::SetCursorScreenPos(suggestBoxPos);
        //    ImGui::BeginChild("##SuggestionScroll", ImVec2(200, 150), true, ImGuiWindowFlags_AlwaysUseWindowPadding);

        //    static int selectedIndex = -1;

        //    for (int i = 0; i < g_CurrentSuggestions.size(); ++i)
        //    {
        //        bool isSelected = (i == selectedIndex);

        //        if (ImGui::Selectable(g_CurrentSuggestions[i].c_str(), isSelected))
        //        {
        //            // 補完挿入（単純追加、カーソル位置への挿入に変更も可能）
        //            file.content += g_CurrentSuggestions[i];
        //            strncpy(buffer, file.content.c_str(), sizeof(buffer) - 1);
        //            buffer[sizeof(buffer) - 1] = '\0';

        //            selectedIndex = -1;
        //        }

        //        if (isSelected)
        //            ImGui::SetScrollHereY(0.5f);
        //    }

        //    ImGui::EndChild();
        //}

        ImGui::EndChild(); // EditorChild


        ImGui::BeginChild("CompileOutput", ImVec2(0, 0), true);
        ImGui::TextWrapped("%s", ConvertToUTF8(g_CompaileOut.c_str()));
        ImGui::EndChild();
    }
    else
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "No file selected");
    }

    ImGui::EndGroup();
}