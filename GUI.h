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
    std::string text;
    int cursor = 0;

    float scrollY = 0.0f;
};

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