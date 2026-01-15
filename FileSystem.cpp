#define NOMINMAX
#include "FileSystem.h"

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
//DLLコンパイラー