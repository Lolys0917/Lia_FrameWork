// StartUp.cpp
#include "ObjectManager.h"

void StartUp()
{
    // --- カメラ初期化 ---
    AddCamera("MainCamera");
    SetCameraPos("MainCamera", 0.0f, 3.0f, -10.0f);
    SetCameraLook("MainCamera", 0.0f, 0.0f, 0.0f);

    AddCamera("SideCamera");
    SetCameraPos("SideCamera", 10.0f, 5.0f, 0.0f);
    SetCameraLook("SideCamera", 0.0f, 0.0f, 0.0f);

    // --- Scene1 ---
    AddScene("Scene1");
    AddGridBox("BoxA");
    AddGridBox("BoxB");
    SetGridBoxPos("BoxA", -2, 0, 0);
    SetGridBoxPos("BoxB", 2, 0, 0);
    SceneEndPoint();

    // --- Scene2 ---
    AddScene("Scene2");
    AddGridPolygon("PolyA");
    SetGridPolygonPos("PolyA", 0, 0, 0);
    SetGridPolygonSides("PolyA", 5);
    SceneEndPoint();

    // --- シーンごとのカメラ割当 ---
    SetSceneCamera("Scene1", "MainCamera");
    SetSceneCamera("Scene2", "SideCamera");

    // 最初のシーン設定
    ChangeScene("Scene1");

    // Camera, Gridなど初期化
    CreateCamera();
    SettingCameraOnce();
    //CreateGrid();
}