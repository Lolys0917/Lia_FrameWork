// The project type: Game Engine Core Module
// The code defines core scene lifecycle functions for initialization, updating, drawing, and ending scenes.

// プログラムのメインとなるシーン管理
// ここに処理を書くことでシーンの初期化、更新、描画、終了処理を行うことができます。
// Initよりも先にStartupが呼ばれます。

#include "CoreScene.h"
#include "Manager.h"

void CoreStartUp()
{
    // --- カメラ初期化 ---
    AddCamera("MainCamera");
    SetCameraPos("MainCamera", 1.0f, 3.0f, -10.0f);
    SetCameraLook("MainCamera", 1.0f, 0.0f, 0.0f);

    AddCamera("SubCamera");
    SetCameraPos("SubCamera", 3.0f, 5.0f, -5.0f);
    SetCameraLook("SubCamera", 0.0f, 0.0f, 0.0f);

    AddCamera("SideCamera");
    SetCameraPos("SideCamera", 0.0f, 5.0f, -5.0f);
    SetCameraLook("SideCamera", 0.0f, 0.0f, 0.0f);

    // --- Scene1 ---
    AddScene("Scene1");
    AddGridBox("BoxA");
    AddGridBox("BoxB");
    SetGridBoxPos("BoxA", -2, 0, 0);
    SetGridBoxPos("BoxB", 2, 0, 0);
    SceneEndPoint();


    AddScene("Scene3");
	SceneEndPoint();

    // --- Scene2 ---
    AddScene("Scene2");
    AddGridPolygon("PolyA");
    SetGridPolygonPos("PolyA", 0, 0, 0);
	SetGridPolygonColor("PolyA", 0, 1, 1, 1);
    SetGridPolygonSides("PolyA", 6);
    SceneEndPoint();

    // --- シーンごとのカメラ割当 ---
    SetSceneCamera("Scene3", "SubCamera");
    SetSceneCamera("Scene1", "MainCamera");
    SetSceneCamera("Scene2", "SideCamera");
    // 最初のシーン設定
    ChangeScene("Scene2");

    AddGridBox("BoxC");
    SetGridBoxPos("BoxC", 0, 0, 0);
    // Camera, Gridなど初期化
}
void CoreSceneUpdate()
{
    static float pos = 0.0f;
	pos += 0.01f;
}
void CoreSceneDraw()
{

}
void CoreSceneEnd()
{

}