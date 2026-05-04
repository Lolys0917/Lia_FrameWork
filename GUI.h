#pragma once

#include <Windows.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include <string>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

bool GUIInit();
void GUIRelease();
bool GUIUpdate();

//
// コードエディタ
//
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

    int start;
    int end;
};
struct SearchResult
{
    Token token;
    int score;
};

//---------------
// CodeEditor.cpp
//---------------

struct CodeEditorState
{
    //テキスト
    std::string text;
    int cursor = 0;
	//スクロール位置
    float scrollY = 0.0f;
	float scrollX = 0.0f;
    // 選択範囲
    int selectStart = -1;
    int selectEnd = -1;
    bool selecting = false;
	// Undo/Redo
    // 差分情報
    std::vector<int> undoPos;
    std::vector<std::string> undoRemoved;
    std::vector<std::string> undoInserted;
    std::vector<int> redoPos;
    std::vector<std::string> redoRemoved;
    std::vector<std::string> redoInserted;
	// Undo/Redoのインデックスと最大数
    int undoIndex = -1;
    int undoMax = 100;
    int lastEditType = 0;
    double lastEditTime = 0.0;
};

//Undo/Redoの動作タイプ
enum EditType
{
    EDIT_NONE,
    EDIT_CHAR,
    EDIT_BACKSPACE,
    EDIT_ENTER,
    EDIT_PASTE,
    EDIT_CUT,
    EDIT_SUGGEST,
    EDIT_MOUSE
};

//シンタックスハイライト用のトークン
struct HLToken
{
    std::string text;
    ImU32 color;
};

bool Keyword(const std::string& s);
void Tokenize(const std::string& src, std::vector<HLToken>& out);
void CodeEditorInput(CodeEditorState& ed);
void CodeEditorDraw(CodeEditorState& ed);

void SetCodeText(std::string inText);
std::string GetCodeText();