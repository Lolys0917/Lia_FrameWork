#pragma once

#include <string>

// ========================================
// コードファイルを表す構造体
struct CodeFile
{
    enum class Type { Header, Cpp };  // ファイルタイプをヘッダーかCPPに分類
    std::string fileName;             // ファイル名
    Type type;                       // ファイルタイプ
    std::string content;             // ファイル内容（テキスト）
};

void ShowCodeEditorUI();
void LoadAllSavedFiles();
void OpenAddNewFileWindow(CodeFile::Type type);
void DrawAddNewFileWindow();
void DrawFileList();