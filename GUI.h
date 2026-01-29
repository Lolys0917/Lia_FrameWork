#pragma once

#include <Windows.h>
#include <imgui.h>
#include <imgui_internal.h>
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
};
struct SearchResult
{
    Token token;
    int score;
};