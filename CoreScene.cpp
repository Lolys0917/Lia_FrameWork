// The project type: Game Engine Core Module
// The code defines core scene lifecycle functions for initialization, updating, drawing, and ending scenes.

// プログラムのメインとなるシーン管理
// ここに処理を書くことでシーンの初期化、更新、描画、終了処理を行うことができます。
// Initよりも先にStartupが呼ばれます。

#include "CoreScene.h"
#include "Manager.h"
#include "AssetLoad.h"

void CoreStartUp()
{
    AL_Init(); // AssetLoad 初期化
    // 読み込み予定のアセット登録
    AL_RegisterAssetToBatch("asset/test.png");
    AL_RegisterAssetToBatch("asset/est.png");
    //AL_RegisterAssetToBatch("asset/model/player.fbx");
    //AL_RegisterAssetToBatch("asset/model/ground.obj");
    
    // pkgファイル化（ゲーム出力時のみ使用）
    AL_SaveAllPackages("saved/pkg/");

    // 実行時は逆に pkg 読み込み（アセットをまだ展開しない）
    AL_LoadPackageIndex("png", "saved/pkg/Assetpng.pkg");
    AL_LoadPackageIndex("fbx", "saved/pkg/Assetfbx.pkg");
    AL_LoadPackageIndex("obj", "saved/pkg/Assetobj.pkg");

    // .pkgからインデックスで読み込み（DirectXリソース生成）
    AL_LoadFromPackageByName("asset/test.png");
    AL_LoadFromPackageByName("asset/est.png");
    //AL_LoadFromPackageByName("asset/model/player.fbx");

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
    AddSpriteWorld("TestSprite01", "asset/test.png");
    SetSpriteWorldColor("TestSprite01", 1, 1, 1, 1);
    SetSpriteWorldSize("TestSprite01", 3, 3, 3);
    SetSpriteWorldPos("TestSprite01", 0, 0, 0);

    AddSpriteWorld("TestSprite02", "asset/est.png");
    SetSpriteWorldColor("TestSprite02", 1, 1, 1, 1);
    SetSpriteWorldSize("TestSprite02", 3, 3, 3);
    SetSpriteWorldPos("TestSprite02", 0, 0, 0);
    SetSpriteWorldAngle("TestSprite02", 0, 0.6f, 0);

    AddSpriteScreen("TestUI01", "asset/test.png");
    SetSpriteScreenPos("TestUI01", 0, 0);
    SetSpriteScreenSize("TestUI01", 100, 100);
    SetSpriteScreenColor("TestUI01", 1, 1, 1, 1);

    //AddSpriteScreen("TestUI02", "asset/test.png");

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
    ChangeScene("Scene3");

    AddGridBox("BoxC");
    SetGridBoxPos("BoxC", 0, 0, 0);


    AddGridBox("BoxD");
    SetGridBoxPos("BoxD", 2, 0, 0);
    // Camera, Gridなど初期化

}
void CoreSceneUpdate()
{
    static float pos = 0.0f;
	pos += 0.01f;
    SetGridBoxPos("BoxC", pos, 0, 0);
}
void CoreSceneDraw()
{

}
void CoreSceneEnd()
{

}