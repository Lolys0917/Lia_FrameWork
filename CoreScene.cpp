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
    //AL_Init(); // AssetLoad 初期化
    //// 読み込み予定のアセット登録
    //AL_RegisterAssetToBatch("asset/test.png");
    //AL_RegisterAssetToBatch("asset/est.png");
    //AL_RegisterAssetToBatch("asset/DiscUR_Reel1.png");
    //AL_RegisterAssetToBatch("asset/hamu.png");
    //AL_RegisterAssetToBatch("asset/boy_model4.fbx");
    //AL_RegisterAssetToBatch("asset/Alicia_solid_Unity.FBX");

    AL_Init();
    /*int registered = AL_RegisterFolderRecursive("asset/");
    char buf[128];
    sprintf_s(buf, "Registered %d files from asset folder", registered);
	MessageBoxA(nullptr, buf, "CoreStartUp", MB_OK);*/
    AL_SaveAllPackages("saved/pkg/");

    //AL_RegisterAssetToBatch("asset/model/player.fbx");
    //AL_RegisterAssetToBatch("asset/model/ground.obj");
    
 
    AL_LoadAllPackages("saved/pkg/");

    //// 実行時は逆に pkg 読み込み（アセットをまだ展開しない）
    /*AL_LoadPackageIndex("png", "saved/pkg/Assetpng.pkg");
    AL_LoadPackageIndex("fbx", "saved/pkg/Assetfbx.pkg");
    AL_LoadPackageIndex("obj", "saved/pkg/Assetobj.pkg");*/

    //// .pkgからインデックスで読み込み（DirectXリソース生成）
    AL_LoadFromPackageByName("test.png");
    AL_LoadFromPackageByName("est.png");
    AL_LoadFromPackageByName("DiscUR_Reel1.png");
    AL_LoadFromPackageByName("hamu.png");
    AL_LoadFromPackageByName("boy_model4.fbx");
    AL_LoadFromPackageByName("asset/Alicia_solid_Unity.FBX");
    //AL_LoadFromPackageByName("asset/AFK_Snowman.fbx");
    AL_LoadFromPackageByName("asset/AFK_SnowmanNmap.png");
    AL_LoadFromPackageByName("asset/AFK_Snowman.png");
    //"AFK_SnowmanNmap.png" "AFK_Snowman.png" 
    
    //AL_LoadFromPackageByName("player.fbx");
   /* AL_LoadFromPackageByName("Alicia_body.psd");
    AL_LoadFromPackageByName("Alicia_other.psd");
    AL_LoadFromPackageByName("Alicia_hair.psd");
    AL_LoadFromPackageByName("Alicia_eye.psd");
    AL_LoadFromPackageByName("Alicia_face.psd");
    AL_LoadFromPackageByName("Alicia_rod.psd");
    AL_LoadFromPackageByName("Alicia_wear.psd");
    AL_LoadFromPackageByName("Alicia_body.tga");
    AL_LoadFromPackageByName("Alicia_other.tga");
    AL_LoadFromPackageByName("Alicia_hair.tga");
    AL_LoadFromPackageByName("Alicia_eye.tga");
    AL_LoadFromPackageByName("Alicia_face.tga");
    AL_LoadFromPackageByName("Alicia_rod.tga");
    AL_LoadFromPackageByName("Alicia_wear.tga");*/

    // --- カメラ初期化 ---
    AddCamera("MainCamera");
    SetCameraPos("MainCamera", 0.0f, 3.0f, -7.0f);
    SetCameraLook("MainCamera", 0.0f, 0.0f, 0.0f);

    AddCamera("SubCamera");
    SetCameraPos("SubCamera", 4.0f, 5.0f, -10.0f);
    SetCameraLook("SubCamera", 0.0f, 0.0f, 0.0f);

    AddCamera("SideCamera");
    SetCameraPos("SideCamera", 1.0f, 5.0f, -5.0f);
    SetCameraLook("SideCamera", 0.0f, 0.0f, 0.0f);

    // --- Scene1 ---
    AddScene("Scene1");
    AddGrid("BoxA", Grid_Box);
    AddGrid("BoxB", Grid_Box);
    SetGridPos("BoxA", -2, 0, 0);
    SetGridPos("BoxB", 2, 0, 0);
    SetGridColor("BoxA", 1, 0, 1, 1);

    //AddSpriteWorld("TestSprite00", "asset/test.png");
    //SetSpriteWorldColor("TestSprite00", 1, 1, 1, 1);
    //SetSpriteWorldSize("TestSprite00", 3, 3, 3);
    //SetSpriteWorldPos("TestSprite00", 0, 0, 0);

    /*AddSpriteBox("Box00", "asset/test.png");
    SetSpriteBoxPos("Box00", 0, 0, -2);
    SetSpriteBoxSize("Box00", 2, 2, 2);
    SetSpriteBoxColor("Box00", 1, 1, 1, 1);*/

    SceneEndPoint();


    AddScene("Scene3");
    AddSpriteWorld("TestSprite01", "asset/test.png");
    SetSpriteWorldColor("TestSprite01", 1, 1, 1, 1);
    SetSpriteWorldSize("TestSprite01", 3, 3, 3);
    SetSpriteWorldPos("TestSprite01", -2, 0, 0);

    AddSpriteWorld("TestSprite02", "asset/est.png");
    SetSpriteWorldColor("TestSprite02", 1, 1, 1, 1);
    SetSpriteWorldSize("TestSprite02", 3, 3, 3);
    SetSpriteWorldPos("TestSprite02", -2, 0, 0);
    SetSpriteWorldAngle("TestSprite02", 0, 0.6f, 0);

    AddSpriteScreen("TestUI01", "asset/test.png");
    SetSpriteScreenPos("TestUI01", 0, 0);
    SetSpriteScreenSize("TestUI01", 100, 100);
    SetSpriteScreenColor("TestUI01", 1, 1, 1, 1);

    AddSpriteCylinder("Cylinder01", "asset/DiscUR_Reel1.png");
    SetSpriteCylinderTextureSide("Cylinder01", "asset/DiscUR_Reel1.png"); 
    SetSpriteCylinderTextureTop("Cylinder01", "asset/hamu.png");   
    SetSpriteCylinderTextureBottom("Cylinder01", "asset/hamu.png");
    SetSpriteCylinderSize("Cylinder01", 1, 2, 1);
    SetSpriteCylinderSegment("Cylinder01", 32);
    SetSpriteCylinderPos("Cylinder01", 0, 0, 0);
    SetSpriteCylinderAngle("Cylinder01", 0, 1.57f, 0);

    AddSpriteCylinder("Cylinder02", "asset/DiscUR_Reel1.png");
    SetSpriteCylinderTextureSide("Cylinder02", "asset/DiscUR_Reel1.png");
    SetSpriteCylinderTextureTop("Cylinder02", "asset/hamu.png");
    SetSpriteCylinderTextureBottom("Cylinder02", "asset/hamu.png");
    SetSpriteCylinderSize("Cylinder02", 1, 2, 1);
    SetSpriteCylinderPos("Cylinder02", 1.2f, 0, 0);
    SetSpriteCylinderAngle("Cylinder02", 0, 1.57f, 0);

    AddSpriteCylinder("Cylinder03", "asset/DiscUR_Reel1.png");
    SetSpriteCylinderTextureSide("Cylinder03", "asset/DiscUR_Reel1.png");
    SetSpriteCylinderTextureTop("Cylinder03", "asset/hamu.png");
    SetSpriteCylinderTextureBottom("Cylinder03", "asset/hamu.png");
    SetSpriteCylinderSize("Cylinder03", 1, 2, 1);
    SetSpriteCylinderPos("Cylinder03", 2.4f, 0, 0);
    SetSpriteCylinderAngle("Cylinder03", 0, 1.57f, 0);

    AddModel("Model01", "asset/Alicia_solid_Unity.FBX");
    //AddModel("Model01", "asset/AFK_Snowman.fbx");
    SetModelPos("Model01", 4.2f, 0, -3);
    SetModelSize("Model01", 0.03f, 0.03f, 0.03f);
	SetModelAngle("Model01", -3.14f/2, 3.14f, 0);
	//ModelTexture("Model01", "asset/AFK_Snowman.png");

	SceneEndPoint();

    // --- Scene2 ---
    AddScene("Scene2");
    AddGrid("PolyA", Grid_Polygon);
    SetGridPos("PolyA", 0, 0, 0);
	SetGridColor("PolyA", 0, 1, 1, 1);
    SetGridSides("PolyA", 6);
    AddSpriteWorld("TestSprite03", "asset/est.png");
    SetSpriteWorldColor("TestSprite03", 1, 1, 1, 1);
    SetSpriteWorldSize("TestSprite03", 3, 3, 3);
    SetSpriteWorldPos("TestSprite03", 0, 0, 2);
    SetSpriteWorldAngle("TestSprite03", 0, 0, 0);

    SceneEndPoint();

    // --- シーンごとのカメラ割当 ---
    SetSceneCamera("Scene3", "SubCamera");
    SetSceneCamera("Scene1", "MainCamera");
    SetSceneCamera("Scene2", "SideCamera");
    // 最初のシーン設定
    ChangeScene("Scene3");

    AddGrid("BoxC", Grid_Box);
    SetGridPos("BoxC", 0, 0, 0);

    AddGrid("BoxD", Grid_Box);
    SetGridPos("BoxD", 2, 0, 0);

    AddCollision("player", "playerTag");
    AddCollision("enemy", "enemyTag");
    //AddCollision("enemy", "enemyTag");

    //当たり判定
    SetCollisionPos("player", 1, 0, 0);

}
void CoreSceneUpdate()
{
	static float rot = 0.0f;
	rot += 0.0025f;
    SetModelAngle("Model01", -3.14f / 2, 3.14f + rot, 0);

    if (HitToTag("player", "enemyTag"))
    {
        MessageBoxA(NULL, "Hit", "Hit", S_OK);
    }

    static float pos = -3.0f;
	pos += 0.01f;
    SetGridPos("BoxC", pos, 0, 0);

    SetCameraPos("SubCamera", 5, 6, -7);

    static float Reel = 0.0f;
    static float Reel1 = 0.0f;
    static float Reel2 = 0.0f;
    static float angle = 0;
    static float angle1 = 0;
    static float angle2 = 0;
    static bool moveF = false;
    static bool moveF1 = false;
    static bool moveF2 = false;
    
    if (GetKeyState('A') < 0)
        moveF = false;

    if (GetInputState(Input::Key_1, 0))
    {
        moveF = false;
    }

    if (GetKeyState('S') < 0)
        moveF1 = false;

    if (GetKeyState('D') < 0)
        moveF2 = false;

    if (GetKeyState(VK_SPACE) < 0)
    {
        moveF = true;
        moveF1 = true;
        moveF2 = true;
    }


    if (moveF)
        Reel = 0.1f;
    else
        Reel = 0.0f;

    if (moveF1)
        Reel1 = 0.1f;
    else
        Reel1 = 0.0f;

    if (moveF2)
        Reel2 = 0.1f;
    else
        Reel2 = 0.0f;


    angle += Reel;
    angle1 += Reel1;
    angle2 += Reel2;

    SetSpriteCylinderAngle("Cylinder01", angle, 0, 1.56);
    SetSpriteCylinderSize("Cylinder01", 2, 1, 2);

    SetSpriteCylinderAngle("Cylinder02", angle1, 0, 1.56);
    SetSpriteCylinderSize("Cylinder02", 2, 1, 2);

    SetSpriteCylinderAngle("Cylinder03", angle2, 0, 1.56);
    SetSpriteCylinderSize("Cylinder03", 2, 1, 2);
}
void CoreSceneDraw()
{

}
void CoreSceneEnd()
{

}