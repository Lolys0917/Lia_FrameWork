// ObjectManager.cpp
// 修正版：ObjectDataPool の全フィールド初期化／解放、ObjectIdx 同期、境界チェックなどを追加
// Objectの作成・管理
// Componentの管理を行う
// 更新時にユーザーによる操作を反映させる
// Manager.h経由でSceneManagerと連携し、シーンごとのオブジェクト管理を行う
// ObjectManagerはシーン内のオブジェクトを管理し、必要に応じて生成・削除を行う。

#include "Manager.h"
#include "ComponentCamera.h"
#include "ComponentSpriteWorld.h"
#include "ComponentSpriteScreen.h"
#include "ComponentSpriteCylinder.h"
#include <string>

// ======================================================
// ObjectManager.cpp（統合・高速化版）
// ======================================================

//-----------------------------------------
// グローバル静的オブジェクト
//-----------------------------------------
static Grid* grid = nullptr;
static Object* object = nullptr;
static ObjectIndex ObjectIdx;
static ObjectDataPool g_ObjectPool; // 実体

//-----------------------------------------
// Index管理（保持は ObjectIdx と同期）
//-----------------------------------------
static int UseCamera = -1;
static int CameraIndex = 0, CameraOldIdx = 0;
static int UIIndex = 0, UIOldIndex = 0;
static int SpriteWorldIndex = 0, SpriteWorldOldIndex = 0;
static int SpriteScreenIndex = 0, SpriteScreenOldIndex = 0;
static int SpriteCylinderIndex = 0, SpriteCylinderOldIndex = 0;
static int ModelIndex = 0, ModelOldIndex = 0;
static int BoxColliderIndex = 0, BoxColliderOldIndex = 0;
static int GridBoxIndex = 0, GridBoxOldIndex = 0;
static int GridPolygonIndex = 0, GridPolygonOldIndex = 0;

//-----------------------------------------
// Getter群
//-----------------------------------------
ObjectDataPool* GetObjectDataPool() { return &g_ObjectPool; }
Grid* GetGridClass() { return grid; }
Object* GetObjectClass() { return object; }
ObjectIndex* GetObjectIndex() { return &ObjectIdx; }
KeyMap* GetCameraKeyMap() { return &g_ObjectPool.CameraMap; }
int GetUseCamera() { return UseCamera; }

//-----------------------------------------
// 汎用Vec4アクセスラッパ（保持は残す）
//-----------------------------------------
Vec4 GetVec4FromPool(IndexType type, int index)
{
    ObjectDataPool* p = GetObjectDataPool();
    switch (type)
    {
    case IndexType::Camera: return Vec4_Get(&p->CameraPos, index);
    case IndexType::GridBox: return Vec4_Get(&p->GridBoxPos, index);
    case IndexType::GridPolygon: return Vec4_Get(&p->GridPolygonPos, index);
    case IndexType::Model: return Vec4_Get(&p->ModelPos, index);
    default: return { 0,0,0,0 };
    }
}

void SetVec4ToPool(IndexType type, int index, Vec4 v)
{
    ObjectDataPool* p = GetObjectDataPool();
    switch (type)
    {
    case IndexType::Camera: Vec4_Set(&p->CameraPos, index, v); break;
    case IndexType::GridBox: Vec4_Set(&p->GridBoxPos, index, v); break;
    case IndexType::GridPolygon: Vec4_Set(&p->GridPolygonPos, index, v); break;
    case IndexType::Model: Vec4_Set(&p->ModelPos, index, v); break;
    default: break;
    }
}

//-----------------------------------------
// Camera管理
//-----------------------------------------
void AddCamera(const char* name) {
    Vec4_PushBack(&g_ObjectPool.CameraPos, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.CameraLook, { 0,0,1,0 });
    KeyMap_Add(&g_ObjectPool.CameraMap, name);
    CameraIndex++;
    ObjectIdx.CameraIndex = CameraIndex;
    // 初回カメラはルート使用カメラにしておく（安全）
    if (UseCamera < 0) UseCamera = 0;
}
void SetCameraPos(const char* name, float x, float y, float z) {
    int idx = KeyMap_GetIndex(&g_ObjectPool.CameraMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("SetCameraPos: camera not found: ", name)); return; }
    Vec4_Set(&g_ObjectPool.CameraPos, idx, { x,y,z,0 });
}
void SetCameraLook(const char* name, float x, float y, float z) {
    int idx = KeyMap_GetIndex(&g_ObjectPool.CameraMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("SetCameraLook: camera not found: ", name)); return; }
    Vec4_Set(&g_ObjectPool.CameraLook, idx, { x,y,z,0 });
}
void UseCameraSet(const char* name) {
    int idx = KeyMap_GetIndex(&g_ObjectPool.CameraMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("UseCameraSet: camera not found: ", name)); return; }
    UseCamera = idx;
}
void SetUseCamera(int index) {
    if (index < 0) { UseCamera = index; return; }
    if (index >= CameraIndex) {
        AddMessage(ConcatCStr("SetUseCamera: invalid index ", std::to_string(index).c_str()));
        return;
    }
    UseCamera = index;
}

//-----------------------------------------
// SpriteWorld
//-----------------------------------------
void AddSpriteWorld(const char* name, const char* pathName)
{
    Vec4_PushBack(&g_ObjectPool.SpriteWorldPos, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.SpriteWorldSize, { 1,1,1,1 });
    Vec4_PushBack(&g_ObjectPool.SpriteWorldAngle, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.SpriteWorldColor, { 1,1,1,1 });
    KeyMap_Add(&g_ObjectPool.SpriteWorldMap, name);
    KeyMap_Add(&g_ObjectPool.SpriteWorldTexturePathMap, pathName);
    SpriteWorldIndex++;
    ObjectIdx.SpriteWorldIndex = SpriteWorldIndex;
}
void SetSpriteWorldPos(const char* name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteWorldMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("SetSpriteWorldPos : sprite not found", name)); return; }
    Vec4_Set(&g_ObjectPool.SpriteWorldPos, idx, { x,y,z,0 });
}
void SetSpriteWorldSize(const char* name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteWorldMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("SetSpriteWorldSize : sprite not found", name)); return; }
    Vec4_Set(&g_ObjectPool.SpriteWorldSize, idx, { x,y,z,0 });
}
void SetSpriteWorldAngle(const char* name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteWorldMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("SetSpriteWorldAngle : sprite not found", name)); return; }
    Vec4_Set(&g_ObjectPool.SpriteWorldAngle, idx, { x,y,z,0 });
}
void SetSpriteWorldColor(const char* name, float r, float g, float b, float a)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteWorldMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("SetSpriteWorldAngle : sprite not found", name)); return; }
    Vec4_Set(&g_ObjectPool.SpriteWorldColor, idx, { r,g,b,a });
}

//-----------------------------------------
// SpriteScreen
//-----------------------------------------
void AddSpriteScreen(const char* name, const char* pathName)
{
    Vec4_PushBack(&g_ObjectPool.SpriteScreenPos, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.SpriteScreenSize, { 100, 100, 100, 100 });
    Vec4_PushBack(&g_ObjectPool.SpriteScreenColor, { 1,1,1,1 });
    VecInt_PushBack(&g_ObjectPool.SpriteScreenAngle, 0);
    KeyMap_Add(&g_ObjectPool.SpriteScreenMap, name);
    KeyMap_Add(&g_ObjectPool.SpriteScreenTexturePathMap, pathName);
    SpriteScreenIndex++;
    ObjectIdx.SpriteScreenIndex = SpriteScreenIndex;
}
void SetSpriteScreenPos(const char* name, float x, float y)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteScreenMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("SetSpriteScreenPos : sprite not found", name)); return; }
    Vec4_Set(&g_ObjectPool.SpriteScreenPos, idx, { x, y, 0, 0 });
}

void SetSpriteScreenSize(const char* name, float x, float y)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteScreenMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("SetSpriteScreenSize : sprite not found", name)); return; }
    Vec4_Set(&g_ObjectPool.SpriteScreenSize, idx, { x, y, 1, 1 });
}
void SetSpriteScreenAngle(const char* name, float angle)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteScreenMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("SetSpriteScreenAngle : sprite not found", name)); return; }
    VecInt_Set(&g_ObjectPool.SpriteScreenAngle, idx, angle);
}
void SetSpriteScreenColor(const char* name, float r, float g, float b, float a)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteScreenMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("SetSpriteScreenColor : sprite not found", name)); return; }
    Vec4_Set(&g_ObjectPool.SpriteScreenColor, idx, { r, g, b, a });
}

//-----------------------------------------
// SpriteCylinder
//-----------------------------------------
void AddSpriteCylinder(const char* name, const char* pathName)
{
    Vec4_PushBack(&g_ObjectPool.SpriteCylinderPos,   { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.SpriteCylinderSize,  { 1,1,1,1 });
    Vec4_PushBack(&g_ObjectPool.SpriteCylinderAngle, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.SpriteCylinderColor, { 1,1,1,1 });
    VecInt_PushBack(&g_ObjectPool.SpriteCylinderSegment, 32);
    KeyMap_Add(&g_ObjectPool.SpriteCylinderMap, name);
    KeyMap_Add(&g_ObjectPool.SpriteCylinderTopTexturePathMap, pathName);
    KeyMap_Add(&g_ObjectPool.SpriteCylinderBottomTexturePathMap, pathName);
    KeyMap_Add(&g_ObjectPool.SpriteCylinderSideTexturePathMap, pathName);
    SpriteCylinderIndex++;
    ObjectIdx.SpriteCylinderIndex = SpriteCylinderIndex;
}
void SetSpriteCylinderPos(const char* name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteCylinderMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("SetSpriteCylinderPos : sprite not found", name)); return; }
    Vec4_Set(&g_ObjectPool.SpriteCylinderPos, idx, { x,y,z,0 });
}
void SetSpriteCylinderSize(const char* name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteCylinderMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("SetSpriteCylinderSize : sprite not found", name)); return; }
    Vec4_Set(&g_ObjectPool.SpriteCylinderSize, idx, { x,y,z,0 });
}
void SetSpriteCylinderAngle(const char* name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteCylinderMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("SetSpriteCylinderAngle : sprite not found", name)); return; }
    Vec4_Set(&g_ObjectPool.SpriteCylinderAngle, idx, { x,y,z,0 });
}
void SetSpriteCylinderColor(const char* name, float r, float g, float b, float a)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteCylinderMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("SetSpriteCylinderColor : sprite not found", name)); return; }
    Vec4_Set(&g_ObjectPool.SpriteCylinderColor, idx, { r,g,b,a });
}
void SetSpriteCylinderSegment(const char* name, int segment)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteCylinderMap, name);
    if (idx < 0) { AddMessage(ConcatCStr("SetSpriteCylinderSegment : sprite not found", name)); return; }
    VecInt_Set(&g_ObjectPool.SpriteCylinderSegment, idx, segment);
}
void SetSpriteCylinderTextureSide(const char* name, const char* pathName){
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteCylinderMap, name);
    if (idx < 0) { AddMessage("SetSpriteCylinderSideTexture : sprite not found"); return; }

    KeyMap_SetKey(&g_ObjectPool.SpriteCylinderSideTexturePathMap, idx, pathName);
}
void SetSpriteCylinderTextureTop(const char* name, const char* pathName){
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteCylinderMap, name);
    if (idx < 0) { AddMessage("SetSpriteCylinderTopTexture : sprite not found"); return; }

    KeyMap_SetKey(&g_ObjectPool.SpriteCylinderTopTexturePathMap, idx, pathName);
}
void SetSpriteCylinderTextureBottom(const char* name, const char* pathName){
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteCylinderMap, name);
    if (idx < 0) { AddMessage("SetSpriteCylinderBottomTexture : sprite not found"); return; }

    KeyMap_SetKey(&g_ObjectPool.SpriteCylinderBottomTexturePathMap, idx, pathName);
}

//-----------------------------------------
// Grid管理
//-----------------------------------------
void AddGridBox(const char* Name)
{
    Vec4_PushBack(&g_ObjectPool.GridBoxPos, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.GridBoxSize, { 1,1,1,1 });
    Vec4_PushBack(&g_ObjectPool.GridBoxAngle, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.GridBoxColor, { 1,1,1,1 });
    KeyMap_Add(&g_ObjectPool.GridBoxMap, Name);
    GridBoxIndex++;
    ObjectIdx.GridBoxIndex = GridBoxIndex;

    NotifyAddObject(IndexType::GridBox);
}
void SetGridBoxPos(const char* Name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.GridBoxMap, Name);
    if (idx < 0) { AddMessage(ConcatCStr("SetGridBoxPos: not found ", Name)); return; }
    Vec4_Set(&g_ObjectPool.GridBoxPos, idx, { x,y,z,0 });
}
void SetGridBoxSize(const char* Name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.GridBoxMap, Name);
    if (idx < 0) { AddMessage(ConcatCStr("SetGridBoxSize: not found ", Name)); return; }
    Vec4_Set(&g_ObjectPool.GridBoxSize, idx, { x,y,z,0 });
}
void SetGridBoxAngle(const char* Name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.GridBoxMap, Name);
    if (idx < 0) { AddMessage(ConcatCStr("SetGridBoxAngle: not found ", Name)); return; }
    Vec4_Set(&g_ObjectPool.GridBoxAngle, idx, { x,y,z,0 });
}
void SetGridBoxColor(const char* Name, float R, float G, float B, float A)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.GridBoxMap, Name);
    if (idx < 0) { AddMessage(ConcatCStr("SetGridBoxColor: not found ", Name)); return; }
    Vec4_Set(&g_ObjectPool.GridBoxColor, idx, { R,G,B,A });
}

void AddGridPolygon(const char* Name)
{
    Vec4_PushBack(&g_ObjectPool.GridPolygonPos, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.GridPolygonSize, { 1,1,1,1 });
    Vec4_PushBack(&g_ObjectPool.GridPolygonAngle, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.GridPolygonColor, { 0,0,0,1 });
    VecInt_PushBack(&g_ObjectPool.GridPolygonSides, 4);
    KeyMap_Add(&g_ObjectPool.GridPolygonMap, Name);
    GridPolygonIndex++;
    ObjectIdx.GridPolygonIndex = GridPolygonIndex;
}
void SetGridPolygonPos(const char* Name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.GridPolygonMap, Name);
    if (idx < 0) { AddMessage(ConcatCStr("SetGridPolygonPos: not found ", Name)); return; }
    Vec4_Set(&g_ObjectPool.GridPolygonPos, idx, { x,y,z,0 });
}
void SetGridPolygonColor(const char* Name, float R, float G, float B, float A)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.GridPolygonMap, Name);
    if (idx < 0) { AddMessage(ConcatCStr("SetGridPolygonColor: not found ", Name)); return; }
    Vec4_Set(&g_ObjectPool.GridPolygonColor, idx, { R,G,B,A });
}
void SetGridPolygonSides(const char* Name, int sides)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.GridPolygonMap, Name);
    if (idx < 0) { AddMessage(ConcatCStr("SetGridPolygonSides: not found ", Name)); return; }
    VecInt_Set(&g_ObjectPool.GridPolygonSides, idx, sides);
}


// オブジェクトの作成・管理
void CreateObject()
{
	//※ コンポーネント(ObjectClass内)のみ追加する
    if (!object) return;

	// Camera
    while (CameraOldIdx < CameraIndex) 
    {
        object->AddComponent<Camera>();
        CameraOldIdx++;
    }
    //SpriteWorld
    while (SpriteWorldOldIndex < SpriteWorldIndex)
    {
        const char* texPath = KeyMap_GetKey(&g_ObjectPool.SpriteWorldTexturePathMap, SpriteWorldOldIndex);

        object->AddComponent<SpriteWorld>()->SetTexture(texPath);
        SpriteWorldOldIndex++;
    }
    while (SpriteScreenOldIndex < SpriteScreenIndex)
    {
        const char* texPath = KeyMap_GetKey(&g_ObjectPool.SpriteScreenTexturePathMap, SpriteScreenOldIndex);

        object->AddComponent<SpriteScreen>()->SetTexture(texPath);
        SpriteScreenOldIndex++;
    }
    while (SpriteCylinderOldIndex < SpriteCylinderIndex)
    {
        const char* topTexPath = KeyMap_GetKey(&g_ObjectPool.SpriteCylinderTopTexturePathMap, SpriteCylinderOldIndex);
        const char* bottomTexPath = KeyMap_GetKey(&g_ObjectPool.SpriteCylinderBottomTexturePathMap, SpriteCylinderOldIndex);
        const char* sideTexPath = KeyMap_GetKey(&g_ObjectPool.SpriteCylinderSideTexturePathMap, SpriteCylinderOldIndex);

        object->AddComponent<SpriteCylinder>()->SetTopTexture(topTexPath);
        object->GetComponent<SpriteCylinder>(SpriteCylinderOldIndex)->SetBottomTexture(bottomTexPath);
        object->GetComponent<SpriteCylinder>(SpriteCylinderOldIndex)->SetSideTexture(sideTexPath);

        SpriteCylinderOldIndex++;
    }
}


//-----------------------------------------
// ライフサイクル
//-----------------------------------------
void InitDo()
{
    // インデックス初期化・ObjectIdx リセット
    UseCamera = -1;
    CameraIndex = UIIndex = SpriteWorldIndex = ModelIndex = BoxColliderIndex = GridBoxIndex = GridPolygonIndex = 0;
    CameraOldIdx = UIOldIndex = SpriteWorldOldIndex = ModelOldIndex = BoxColliderOldIndex = GridBoxOldIndex = GridPolygonOldIndex = 0;

    ObjectIdx.CameraIndex = 0;
    ObjectIdx.SpriteWorldIndex = 0;
    ObjectIdx.SpriteScreenIndex = 0;
    ObjectIdx.ModelIndex = 0;
    ObjectIdx.BoxColliderIndex = 0;
    ObjectIdx.SphereColliderIndex = 0;
    ObjectIdx.CapsuleColliderIndex = 0;
    ObjectIdx.GridLineIndex = 0;
    ObjectIdx.GridBoxIndex = 0;
    ObjectIdx.GridPolygonIndex = 0;
    ObjectIdx.GridSphereIndex = 0;
    ObjectIdx.GridCapsuleIndex = 0;
    ObjectIdx.EffectIndex = 0;

    // Vec4Init（Pool 全部） --- ここを必ずすべて列挙することが重要
    ObjectDataPool* p = &g_ObjectPool;

    // Camera
    Vec4_Init(&p->CameraPos);
    Vec4_Init(&p->CameraLook);

    // UI
    Vec4_Init(&p->UITBLR);
    Vec4_Init(&p->UIAngle);
    Vec4_Init(&p->UIColor);

    // World2d
    Vec4_Init(&p->SpriteWorldPos);
    Vec4_Init(&p->SpriteWorldSize);
    Vec4_Init(&p->SpriteWorldAngle);

    // Model
    Vec4_Init(&p->ModelPos);
    Vec4_Init(&p->ModelSize);
    Vec4_Init(&p->ModelAngle);

    // BoxCollider
    Vec4_Init(&p->BoxColliderPos);
    Vec4_Init(&p->BoxColliderSize);
    Vec4_Init(&p->BoxColliderAngle);

    // Grid Box / Polygon
    Vec4_Init(&p->GridBoxPos);
    Vec4_Init(&p->GridBoxSize);
    Vec4_Init(&p->GridBoxAngle);
    Vec4_Init(&p->GridBoxColor);

    Vec4_Init(&p->GridPolygonPos);
    Vec4_Init(&p->GridPolygonSize);
    Vec4_Init(&p->GridPolygonAngle);
    Vec4_Init(&p->GridPolygonColor);

    // Int/Char/Bool vectors
    VecInt_Init(&p->GridPolygonSides);
    VecC_Init(&p->TexturePath);
    VecC_Init(&p->ModelPath);
    VecInt_Init(&p->NumberOfScenes);
    VecInt_Init(&p->ModelType);
    VecBool_Init(&p->BillboardW2d);

    // KeyMaps
    KeyMap_Init(&p->CameraMap);
    KeyMap_Init(&p->ModelMap);
    KeyMap_Init(&p->TextureMap);
    KeyMap_Init(&p->SpriteWorldMap);
    KeyMap_Init(&p->UIMap);
    KeyMap_Init(&p->BoxColliderMap);
    KeyMap_Init(&p->GridBoxMap);
    KeyMap_Init(&p->GridPolygonMap);
    KeyMap_Init(&p->SpriteWorldTexturePathMap);
    KeyMap_Init(&p->SpriteScreenTexturePathMap);

    // クラス取得
    grid = new Grid();
    grid->Init();
    object = new Object();
    object->Init();

    CreateObject();
}

void UpdateDo()
{
    CreateObject();
    UpdateScene();
    object->Update();
}

void DrawDo()
{
    DrawScene();
    object->Draw();
}

void ReleaseDo()
{
    // Free all pools properly
    ObjectDataPool* p = &g_ObjectPool;

    Vec4_Free(&p->CameraPos);
    Vec4_Free(&p->CameraLook);

    Vec4_Free(&p->UITBLR);
    Vec4_Free(&p->UIAngle);
    Vec4_Free(&p->UIColor);

    Vec4_Free(&p->SpriteWorldPos);
    Vec4_Free(&p->SpriteWorldSize);
    Vec4_Free(&p->SpriteWorldAngle);

    Vec4_Free(&p->ModelPos);
    Vec4_Free(&p->ModelSize);
    Vec4_Free(&p->ModelAngle);

    Vec4_Free(&p->BoxColliderPos);
    Vec4_Free(&p->BoxColliderSize);
    Vec4_Free(&p->BoxColliderAngle);

    Vec4_Free(&p->GridBoxPos);
    Vec4_Free(&p->GridBoxSize);
    Vec4_Free(&p->GridBoxAngle);
    Vec4_Free(&p->GridBoxColor);

    Vec4_Free(&p->GridPolygonPos);
    Vec4_Free(&p->GridPolygonSize);
    Vec4_Free(&p->GridPolygonAngle);
    Vec4_Free(&p->GridPolygonColor);

    VecInt_Free(&p->GridPolygonSides);
    VecC_Free(&p->TexturePath);
    VecC_Free(&p->ModelPath);
    VecInt_Free(&p->NumberOfScenes);
    VecInt_Free(&p->ModelType);
    VecBool_Free(&p->BillboardW2d);

    KeyMap_Free(&p->CameraMap);
    KeyMap_Free(&p->ModelMap);
    KeyMap_Free(&p->TextureMap);
    KeyMap_Free(&p->SpriteWorldMap);
    KeyMap_Free(&p->UIMap);
    KeyMap_Free(&p->BoxColliderMap);
    KeyMap_Free(&p->GridBoxMap);
    KeyMap_Free(&p->GridPolygonMap);

    // オブジェクト解放
    if (object) { delete object; object = nullptr; }
    if (grid) { delete grid; grid = nullptr; }
}

void OutObjectIndex(ObjectIndex* out)
{
    *out = ObjectIdx;
}
