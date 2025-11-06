#pragma once
#include <vector>
#include <string>
#include <memory>
#include "Object.h"
#include "Main.h"
#include "ObjectManager.h"

class Scene {
public:
    std::vector<std::unique_ptr<Object>> objects;

    void Init() {}
    void Update() {
        for (auto& obj : objects) obj->Update();
    }
    void Draw() {
        for (auto& obj : objects) obj->Draw();
    }
    void Release() {
        for (auto& obj : objects) obj->Release();
        objects.clear();
    }

    Object* AddObject() {
        auto obj = std::make_unique<Object>();
        obj->Init();
        objects.push_back(std::move(obj));
        return objects.back().get();
    }
};

// ==========================================
// SceneManager関数群
// ==========================================
void InitSceneManager();              // 初期化
void AddScene(const char* SceneName); // シーン追加（構築開始）
void SceneEndPoint();                 // 構築終了
void ChangeScene(const char* SceneName); // シーン切り替え
void DeleteScene(const char* SceneName); // シーン削除
Scene* GetCurrentScene();             // 現在シーン取得
void UpdateScene();                   // 更新
void DrawScene();                     // 描画
void ReleaseSceneManager();           // 解放