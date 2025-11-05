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

// シーン追加（構築開始）
void AddScene(const char* SceneName);

// シーン構築終了
void SceneEndPoint();

// シーン切り替え
void ChangeScene(const char* SceneName);

// シーン削除
void DeleteScene(const char* SceneName);

// 現在のシーン取得
Scene* GetCurrentScene();

// 現在のシーン更新／描画
void UpdateScene();
void DrawScene();