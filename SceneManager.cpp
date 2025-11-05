#include "SceneManager.h"

// ========================================================
// 内部グローバル状態
// ========================================================
static std::vector<std::unique_ptr<Scene>> SceneList;
static KeyMap SceneMap;
static int CurrentSceneIndex = -1;

// 現在構築中のシーン情報
static bool SceneIsBuilding = false;
static int SceneBuildIndex = -1;

// ========================================================
// シーン管理関数群
// ========================================================

// シーン追加（構築開始）
void AddScene(const char* SceneName)
{
    if (SceneIsBuilding)
    {
        AddMessage("Warning: 前のSceneEndPoint()が呼ばれていません。自動で終了します。\n");
        SceneEndPoint();
    }

    KeyMap_Add(&SceneMap, SceneName);

    SceneList.push_back(std::make_unique<Scene>());
    SceneList.back()->Init();

    SceneIsBuilding = true;
    SceneBuildIndex = static_cast<int>(SceneList.size() - 1);

    AddMessage(("Scene追加: " + std::string(SceneName) + "\n").c_str());
}

// シーン構築終了
void SceneEndPoint()
{
    if (!SceneIsBuilding)
    {
        AddMessage("Warning: SceneEndPoint() 呼び出し時に構築中シーンが存在しません。\n");
        return;
    }

    SceneIsBuilding = false;
    SceneBuildIndex = -1;
    AddMessage("SceneEndPoint: シーン構築を終了しました。\n");
}

// シーン切り替え
void ChangeScene(const char* SceneName)
{
    int idx = KeyMap_GetIndex(&SceneMap, SceneName);
    if (idx == -1)
    {
        AddMessage(("Error: 指定されたシーンが存在しません -> " + std::string(SceneName) + "\n").c_str());
        return;
    }

    // 現在のシーンをリリース
    if (CurrentSceneIndex >= 0 && CurrentSceneIndex < (int)SceneList.size())
    {
        SceneList[CurrentSceneIndex]->Release();
    }

    CurrentSceneIndex = idx;
    AddMessage(("ChangeScene: 現在のシーンを " + std::string(SceneName) + " に変更しました。\n").c_str());
}

// シーン削除
void DeleteScene(const char* SceneName)
{
    int idx = KeyMap_GetIndex(&SceneMap, SceneName);
    if (idx == -1)
    {
        AddMessage(("Error: DeleteScene: 指定されたシーンが存在しません -> " + std::string(SceneName) + "\n").c_str());
        return;
    }

    SceneList[idx]->Release();
    SceneList.erase(SceneList.begin() + idx);
    AddMessage(("DeleteScene: シーン " + std::string(SceneName) + " を削除しました。\n").c_str());

    if (CurrentSceneIndex == idx) CurrentSceneIndex = -1;
}

// 現在のシーン取得
Scene* GetCurrentScene()
{
    if (CurrentSceneIndex < 0 || CurrentSceneIndex >= (int)SceneList.size())
        return nullptr;
    return SceneList[CurrentSceneIndex].get();
}

// シーン更新
void UpdateScene()
{
    Scene* scene = GetCurrentScene();
    if (scene)
        scene->Update();
    else
        AddMessage("UpdateScene: 現在有効なシーンがありません。\n");
}

// シーン描画
void DrawScene()
{
    Scene* scene = GetCurrentScene();
    if (scene)
        scene->Draw();
    else
        AddMessage("DrawScene: 現在有効なシーンがありません。\n");
}