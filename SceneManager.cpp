//#include "SceneManager.h"
//
//// ==========================================
//// 内部管理
//// ==========================================
//static std::vector<std::unique_ptr<Scene>> SceneList;
//static KeyMap SceneMap;
//static int CurrentSceneIndex = -1;
//static bool SceneIsBuilding = false;
//static int SceneBuildIndex = -1;
//
//// ==========================================
//// 初期化
//// ==========================================
//void InitSceneManager()
//{
//    KeyMap_Init(&SceneMap);
//    SceneList.clear();
//    CurrentSceneIndex = -1;
//    SceneIsBuilding = false;
//    SceneBuildIndex = -1;
//    AddMessage("SceneManager: 初期化完了\n");
//}
//
//// ==========================================
//// シーン追加（構築開始）
//// ==========================================
//void AddScene(const char* SceneName)
//{
//    if (SceneIsBuilding)
//    {
//        AddMessage("Warning: 前のSceneEndPoint()が呼ばれていません。自動で終了します。\n");
//        SceneEndPoint();
//    }
//
//    KeyMap_Add(&SceneMap, SceneName);
//    SceneList.push_back(std::make_unique<Scene>());
//    SceneList.back()->Init();
//
//    SceneIsBuilding = true;
//    SceneBuildIndex = static_cast<int>(SceneList.size() - 1);
//
//    AddMessage(ConcatCStr("AddScene: シーン追加 -> ", SceneName));
//}
//
//// ==========================================
//// シーン構築終了
//// ==========================================
//void SceneEndPoint()
//{
//    if (!SceneIsBuilding)
//    {
//        AddMessage("Warning: SceneEndPoint() 呼び出し時に構築中シーンが存在しません。\n");
//        return;
//    }
//
//    SceneIsBuilding = false;
//    SceneBuildIndex = -1;
//    AddMessage("SceneEndPoint: シーン構築終了\n");
//}
//
//// ==========================================
//// シーン切り替え
//// ==========================================
//void ChangeScene(const char* SceneName)
//{
//    int idx = KeyMap_GetIndex(&SceneMap, SceneName);
//    if (idx == -1)
//    {
//        AddMessage(ConcatCStr("Error: ChangeScene() 指定シーンが存在しません -> ", SceneName));
//        return;
//    }
//
//    // 現在のシーンをリリース
//    if (CurrentSceneIndex >= 0 && CurrentSceneIndex < (int)SceneList.size())
//        SceneList[CurrentSceneIndex]->Release();
//
//    CurrentSceneIndex = idx;
//
//    AddMessage(ConcatCStr("ChangeScene: シーン切り替え -> ", SceneName));
//}
//
//// ==========================================
//// シーン削除
//// ==========================================
//void DeleteScene(const char* SceneName)
//{
//    int idx = KeyMap_GetIndex(&SceneMap, SceneName);
//    if (idx == -1)
//    {
//        AddMessage(ConcatCStr("Error: DeleteScene() 指定シーンが存在しません -> ", SceneName));
//        return;
//    }
//
//    SceneList[idx]->Release();
//    SceneList.erase(SceneList.begin() + idx);
//    AddMessage(ConcatCStr("DeleteScene: シーン削除 -> ", SceneName));
//
//    if (CurrentSceneIndex == idx)
//        CurrentSceneIndex = -1;
//}
//
//// ==========================================
//// 現在シーン取得
//// ==========================================
//Scene* GetCurrentScene()
//{
//    if (CurrentSceneIndex < 0 || CurrentSceneIndex >= (int)SceneList.size())
//        return nullptr;
//    return SceneList[CurrentSceneIndex].get();
//}
//
//// ==========================================
//// 更新・描画
//// ==========================================
//void UpdateScene()
//{
//    Scene* scene = GetCurrentScene();
//    if (scene)
//        scene->Update();
//}
//void DrawScene()
//{
//    Scene* scene = GetCurrentScene();
//    if (scene)
//        scene->Draw();
//}
//
//// ==========================================
//// 終了処理
//// ==========================================
//void ReleaseSceneManager()
//{
//    for (auto& scene : SceneList)
//        scene->Release();
//
//    SceneList.clear();
//    KeyMap_Free(&SceneMap);
//    CurrentSceneIndex = -1;
//    SceneIsBuilding = false;
//    SceneBuildIndex = -1;
//
//    AddMessage("SceneManager: 解放完了\n");
//}