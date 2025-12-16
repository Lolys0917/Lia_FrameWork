// ObjectManager.cpp
// 修正版：ObjectDataPool の全フィールド初期化／解放、ObjectIdx 同期、境界チェックなどを追加
// Objectの作成・管理
// Componentの管理を行う
// 更新時にユーザーによる操作を反映させる
// Manager.h経由でSceneManagerと連携し、シーンごとのオブジェクト管理を行う
// ObjectManagerはシーン内のオブジェクトを管理し、必要に応じて生成・削除を行う。

#include "Manager.h"
#include "ComponentCamera.h"
#include "ComponentSpriteScreen.h"
#include "ComponentSpriteWorld.h"
#include "ComponentSpriteBox.h"
#include "ComponentSpriteCylinder.h"
#include "ComponentModel.h"
#include "ComponentSound.h"
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
static KeyMap g_ClassTypeMap; // クラスタイプマップ

//-----------------------------------------
// Index管理（保持は ObjectIdx と同期）
//-----------------------------------------
static int UseCamera = -1;
static int CameraIndex = 0, CameraOldIdx = 0;
static int UIIndex = 0, UIOldIndex = 0;
static int SpriteWorldIndex = 0, SpriteWorldOldIndex = 0;
static int SpriteScreenIndex = 0, SpriteScreenOldIndex = 0;
static int SpriteBoxIndex = 0, SpriteBoxOldIndex = 0;
static int SpriteCylinderIndex = 0, SpriteCylinderOldIndex = 0;
static int ModelIndex = 0, ModelOldIndex = 0;
static int BoxColliderIndex = 0, BoxColliderOldIndex = 0;
static int GridIndex = 0, GridOldIndex = 0;
static int CollisionIndex = 0, CollisionOldIndex = 0;

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
// SpriteBox
//-----------------------------------------
void AddSpriteBox(const char* name, const char* pathName)
{
    Vec4_PushBack(&g_ObjectPool.SpriteBoxPos, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.SpriteBoxSize, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.SpriteBoxAngle, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.SpriteBoxColor, { 0,0,0,0 });
    KeyMap_Add(&g_ObjectPool.SpriteBoxMap, name);
    KeyMap_Add(&g_ObjectPool.SpriteBoxTopTexturePathMap,    pathName);
    KeyMap_Add(&g_ObjectPool.SpriteBoxBottomTexturePathMap, pathName);
    KeyMap_Add(&g_ObjectPool.SpriteBoxFrontTexturePathMap,  pathName);
    KeyMap_Add(&g_ObjectPool.SpriteBoxRearTexturePathMap,   pathName);
    KeyMap_Add(&g_ObjectPool.SpriteBoxLeftTexturePathMap,   pathName);
    KeyMap_Add(&g_ObjectPool.SpriteBoxRightTexturePathMap,  pathName);
    SpriteBoxIndex++;
    ObjectIdx.SpriteBoxIndex = SpriteBoxIndex;
}
void SetSpriteBoxPos(const char* name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteBoxMap, name);
    if(idx < 0){ return; }
    Vec4_Set(&g_ObjectPool.SpriteBoxPos, idx, { x,y,z,0 });
}
void SetSpriteBoxSize(const char* name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteBoxMap, name);
    if (idx < 0) { return; }
    Vec4_Set(&g_ObjectPool.SpriteBoxSize, idx, { x,y,z,0 });
}
void SetSpriteBoxAngle(const char* name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteBoxMap, name);
    if (idx < 0) { return; }
    Vec4_Set(&g_ObjectPool.SpriteBoxAngle, idx, { x,y,z,0 });
}
void SetSpriteBoxColor(const char* name, float r, float g, float b, float a)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteBoxMap, name);
    if (idx < 0) { return; }
    Vec4_Set(&g_ObjectPool.SpriteBoxColor, idx, { r,g,b,a });
}
void SetSpriteBoxTextureTop(const char* name, const char* pathName)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteBoxMap, name);
    if (idx < 0) { return; }
    KeyMap_SetKey(&g_ObjectPool.SpriteBoxTopTexturePathMap, idx, pathName);
}
void SetSpriteBoxTextureBottom(const char* name, const char* pathName)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteBoxMap, name);
    if (idx < 0) { return; }
    KeyMap_SetKey(&g_ObjectPool.SpriteBoxBottomTexturePathMap, idx, pathName);
}
void SetSpriteBoxTextureFront(const char* name, const char* pathName)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteBoxMap, name);
    if (idx < 0) { return; }
    KeyMap_SetKey(&g_ObjectPool.SpriteBoxFrontTexturePathMap, idx, pathName);
}
void SetSpriteBoxTextureRear(const char* name, const char* pathName)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteBoxMap, name);
    if (idx < 0) { return; }
    KeyMap_SetKey(&g_ObjectPool.SpriteBoxRearTexturePathMap, idx, pathName);
}
void SetSpriteBoxTextureLeft(const char* name, const char* pathName)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteBoxMap, name);
    if (idx < 0) { return; }
    KeyMap_SetKey(&g_ObjectPool.SpriteBoxLeftTexturePathMap, idx, pathName);
}
void SetSpriteBoxTextureRight(const char* name, const char* pathName)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.SpriteBoxMap, name);
    if (idx < 0) { return; }
    KeyMap_SetKey(&g_ObjectPool.SpriteBoxRightTexturePathMap, idx, pathName);
}
void SetSpriteBoxTexture(const char* name, const char* pathName)
{
    SetSpriteBoxTextureTop(name, pathName);
    SetSpriteBoxTextureBottom(name, pathName);
    SetSpriteBoxTextureFront(name, pathName);
    SetSpriteBoxTextureRear(name, pathName);
    SetSpriteBoxTextureLeft(name, pathName);
    SetSpriteBoxTextureRight(name, pathName);
}

//-----------------------------------------
// SpriteCylinder
//-----------------------------------------
void AddSpriteCylinder(const char* name, const char* pathName)
{
    int typeIdx = object->GetComponentType<SpriteCylinder>();
    Vec4_PushBack(
        &g_ObjectPool.SpriteCylinderInfo, {
            (float)GetCurrentSceneIndex(),
            (float)typeIdx,
            (float)SpriteCylinderOldIndex, 0 });

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
void AddGrid(const char* name, GridType type)
{
    int typeIdx = object->GetComponentType<Grid>();
    Vec4_PushBack(&g_ObjectPool.GridInfo, { (float)GetCurrentSceneIndex(), (float)typeIdx, (float)GridOldIndex, 0 });

    //CurrentSceneをデバッグ表示
	//MessageBoxA(NULL, ConcatCStr("CurrentSceneIndex:", std::to_string(GetCurrentSceneIndex()).c_str()), "Debug", MB_OK);

    Vec4_PushBack(&g_ObjectPool.GridPos, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.GridSize, { 1,1,1,1 });
    Vec4_PushBack(&g_ObjectPool.GridAngle, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.GridColor, { 1,1,1,1 });
    VecInt_PushBack(&g_ObjectPool.GridSides, 4);
    VecInt_PushBack(&g_ObjectPool.GridTypeIndex, type);
    KeyMap_Add(&g_ObjectPool.GridMap, name);
    GridIndex++;
    ObjectIdx.GridIndex = GridIndex;

    //NotifyAddObject(IndexType::Grid);
}
void SetGridPos(const char* Name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.GridMap, Name);
    if (idx < 0) { AddMessage(ConcatCStr("SetGridBoxPos: not found ", Name)); return; }
    Vec4_Set(&g_ObjectPool.GridPos, idx, { x,y,z,0 });
}
void SetGridSize(const char* Name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.GridMap, Name);
    if (idx < 0) { AddMessage(ConcatCStr("SetGridBoxSize: not found ", Name)); return; }
    Vec4_Set(&g_ObjectPool.GridSize, idx, { x,y,z,0 });
}
void SetGridAngle(const char* Name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.GridMap, Name);
    if (idx < 0) { AddMessage(ConcatCStr("SetGridBoxAngle: not found ", Name)); return; }
    Vec4_Set(&g_ObjectPool.GridAngle, idx, { x,y,z,0 });
}
void SetGridColor(const char* Name, float R, float G, float B, float A)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.GridMap, Name);
    if (idx < 0) { AddMessage(ConcatCStr("SetGridBoxColor: not found ", Name)); return; }
    Vec4_Set(&g_ObjectPool.GridColor, idx, { R,G,B,A });
}
void SetGridSides(const char* Name, int sides)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.GridMap, Name);
    if (idx < 0) { AddMessage(ConcatCStr("SetGridBoxSides: not found ", Name)); return; }
    VecInt_Set(&g_ObjectPool.GridSides, idx, sides);
}

//==================================
// Model
//==================================
void AddModel(const char* name, const char* pathName)
{
    Vec4_PushBack(&g_ObjectPool.ModelPos, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.ModelSize, { 1,1,1,1 });
    Vec4_PushBack(&g_ObjectPool.ModelAngle, { 0,0,0,0 });
    VecBool_PushBack(&g_ObjectPool.ModelUseTexture, false);
    KeyMap_Add(&g_ObjectPool.ModelMap, name);
    KeyMap_Add(&g_ObjectPool.ModelFileMap, pathName);
    KeyMap_Add(&g_ObjectPool.ModelTextureMap, "NoTexture");
    ModelIndex++;
    ObjectIdx.ModelIndex = ModelIndex;
}
void SetModelPos(const char* name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.ModelMap, name);
    Vec4_Set(&g_ObjectPool.ModelPos, idx, {x,y,z,0});
}
void SetModelSize(const char* name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.ModelMap, name);
    Vec4_Set(&g_ObjectPool.ModelSize, idx, { x,y,z,1 });
}
void SetModelAngle(const char* name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.ModelMap, name);
    Vec4_Set(&g_ObjectPool.ModelAngle, idx, { x,y,z,0 });
}
void SetModelTexture(const char* name, const char* pathName)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.ModelMap, name);
    KeyMap_SetKey(&g_ObjectPool.ModelTextureMap, idx, pathName);
    VecBool_Set(&g_ObjectPool.ModelUseTexture, idx, true);
}

//========================================
// Collision
//========================================
void AddCollision(const char* name, const char* tag)
{
    KeyMap_Add(&g_ObjectPool.CollisionMap, name);
    KeyMap_Add(&g_ObjectPool.CollisionTagMap, tag);
    KeyMap_Add(&g_ObjectPool.CollisionParentMap, "NoParent");
    Vec4_PushBack(&g_ObjectPool.CollisionPos, { 0,0,0,0 });
    Vec4_PushBack(&g_ObjectPool.CollisionSize, { 1,1,1,1 });
    Vec4_PushBack(&g_ObjectPool.CollisionAngle, { 0,0,0,0 });
    VecBool_PushBack(&g_ObjectPool.CollisionHit, false);
    VecInt_PushBack(&g_ObjectPool.CollisionType, CollisionType::CollisionBox);
    CollisionIndex++;
    ObjectIdx.CollisionIndex = CollisionIndex;
}
//内部機能性関数------------------------------
int Collision_GetIndexByName(const char* name)
{
    return KeyMap_GetIndex(&GetObjectDataPool()->CollisionMap, name);
}
int Collision_GetIndexByTag(const char* tag)
{
    return KeyMap_GetIndex(&GetObjectDataPool()->CollisionTagMap, tag);
}
const char* Collision_GetTagByIndex(int idx)
{
    return KeyMap_GetKey(&GetObjectDataPool()->CollisionTagMap, idx);
}
int Collision_Find(const char* name, const char* tag)
{
    ObjectDataPool* p = GetObjectDataPool();
    int total = GetObjectIndex()->CollisionIndex;

    for (int i = 0; i < total; i++)
    {
        const char* cname = KeyMap_GetKey(&p->CollisionMap, i);
        const char* ctag = KeyMap_GetKey(&p->CollisionTagMap, i);

        if (!strcmp(cname, name) && !strcmp(ctag, tag))
            return i;
    }
    return -1;
}

bool Collision_TagAllow(int idx1, int idx2)
{
    //ALLに入れられたら全てと判定する
    const char* tag1 = Collision_GetTagByIndex(idx1);
    const char* tag2 = Collision_GetTagByIndex(idx2);

    if (strcmp(tag1, "ALL") == 0) return true;
    if (strcmp(tag2, "ALL") == 0) return true;

    return strcmp(tag1, tag2) == 0;
}
bool HitJudgeTo(int idx1, int idx2)
{
    //HitToName用

    ObjectDataPool* p = GetObjectDataPool();

    if (!Collision_TagAllow(idx1, idx2))
    {
        return false;
    }

    //値取得
    Vec4 aPos = Vec4_Get(&p->CollisionPos, idx1);
    Vec4 aSize = Vec4_Get(&p->CollisionSize, idx1);
    Vec4 bPos = Vec4_Get(&p->CollisionPos, idx2);
    Vec4 bSize = Vec4_Get(&p->CollisionSize, idx2);

    //AABB判定
    float dx = fabs(aPos.X - bPos.X);
    float dy = fabs(aPos.Y - bPos.Y);
    float dz = fabs(aPos.Z - bPos.Z);

    if (dx < (aSize.X + bSize.X) * 0.5f &&
        dy < (aSize.Y + bSize.Y) * 0.5f &&
        dz < (aSize.Z + bSize.Z) * 0.5f)
    {
        return true;
    }
    return false;
}
void UpdateCollisionFromParent(int colIndex)
{
    ObjectDataPool* p = GetObjectDataPool();
    const char* parentName = KeyMap_GetKey(&p->CollisionParentMap, colIndex);

    if (strcmp(parentName, "NoParent") == 0)
        return;

    Vec4 parentPos;
}

//API用関数------------------------
bool HitToTag(const char* name, const char* tag)
{
    ObjectDataPool* p = GetObjectDataPool();
    int total = GetObjectIndex()->CollisionIndex;

    // 衝突元（src）
    int idxSrc = Collision_GetIndexByName(name);
    if (idxSrc < 0) return false;

    Vec4 srcPos = Vec4_Get(&p->CollisionPos, idxSrc);
    Vec4 srcSize = Vec4_Get(&p->CollisionSize, idxSrc);

    // ------------- TAG = "ALL" の場合 -------------
    if (strcmp(tag, "ALL") == 0)
    {
        for (int i = 0; i < total; i++)
        {
            if (i == idxSrc) continue;

            Vec4 dstPos = Vec4_Get(&p->CollisionPos, i);
            Vec4 dstSize = Vec4_Get(&p->CollisionSize, i);

            if (fabs(srcPos.X - dstPos.X) < (srcSize.X + dstSize.X) * 0.5f &&
                fabs(srcPos.Y - dstPos.Y) < (srcSize.Y + dstSize.Y) * 0.5f &&
                fabs(srcPos.Z - dstPos.Z) < (srcSize.Z + dstSize.Z) * 0.5f)
            {
                return true;
            }
        }
        return false;
    }

    // ----------- TAG が通常の場合（特定タグだけ判定） ----------
    for (int i = 0; i < total; i++)
    {
        const char* t = Collision_GetTagByIndex(i);
        if (strcmp(t, tag) != 0) continue;
        if (i == idxSrc) continue;

        Vec4 dstPos = Vec4_Get(&p->CollisionPos, i);
        Vec4 dstSize = Vec4_Get(&p->CollisionSize, i);

        if (fabs(srcPos.X - dstPos.X) < (srcSize.X + dstSize.X) * 0.5f &&
            fabs(srcPos.Y - dstPos.Y) < (srcSize.Y + dstSize.Y) * 0.5f &&
            fabs(srcPos.Z - dstPos.Z) < (srcSize.Z + dstSize.Z) * 0.5f)
        {
            return true;
        }
    }

    return false;
}
bool HitToName(const char* name1, const char* name2)
{
    int idx1 = Collision_GetIndexByName(name1);
    int idx2 = Collision_GetIndexByName(name2);

    if (idx1 < 0 || idx2 < 0)
         return false;

    return HitJudgeTo(idx1, idx2);
}
void SetCollisionParent(const char* name, const char* parent)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.CollisionMap, name);
    KeyMap_SetKey(&g_ObjectPool.CollisionParentMap, idx, parent);
}
void SetCollisionPos(const char* name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.CollisionMap, name);
    Vec4_Set(&g_ObjectPool.CollisionPos, idx, { x,y,z,0 });
}
void SetCollisionSize(const char* name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.CollisionMap, name);
    Vec4_Set(&g_ObjectPool.CollisionSize, idx, { x,y,z,1 });
}
void SetCollisionAngle(const char* name, float x, float y, float z)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.CollisionMap, name);
    Vec4_Set(&g_ObjectPool.CollisionAngle, idx, { x,y,z,0 });
}
void SetCollisionType(const char* name, CollisionType type)
{
    int idx = KeyMap_GetIndex(&g_ObjectPool.CollisionMap, name);
    VecInt_Set(&g_ObjectPool.CollisionType, idx, type);
}


  //////////////////////
 // オブジェクト管理 //
//////////////////////
//
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
    while (SpriteBoxOldIndex < SpriteBoxIndex)
    {
        const char* topTexPath    = KeyMap_GetKey(&g_ObjectPool.SpriteBoxTopTexturePathMap, SpriteBoxOldIndex);
        const char* bottomTexPath = KeyMap_GetKey(&g_ObjectPool.SpriteBoxBottomTexturePathMap, SpriteBoxOldIndex);
        const char* frontTexPath  = KeyMap_GetKey(&g_ObjectPool.SpriteBoxFrontTexturePathMap, SpriteBoxOldIndex);
        const char* rearTexPath   = KeyMap_GetKey(&g_ObjectPool.SpriteBoxRearTexturePathMap, SpriteBoxOldIndex);
        const char* leftTexPath   = KeyMap_GetKey(&g_ObjectPool.SpriteBoxLeftTexturePathMap, SpriteBoxOldIndex);
        const char* rightTexPath  = KeyMap_GetKey(&g_ObjectPool.SpriteBoxRightTexturePathMap, SpriteBoxOldIndex);

        object->AddComponent<SpriteBox>()->SetTextureTop(topTexPath);
        object->GetComponent<SpriteBox>(SpriteBoxOldIndex)->SetTextureBottom(bottomTexPath);
        object->GetComponent<SpriteBox>(SpriteBoxOldIndex)->SetTextureFront(frontTexPath);
        object->GetComponent<SpriteBox>(SpriteBoxOldIndex)->SetTextureRear(rearTexPath);
        object->GetComponent<SpriteBox>(SpriteBoxOldIndex)->SetTextureLeft(leftTexPath);
        object->GetComponent<SpriteBox>(SpriteBoxOldIndex)->SetTextureRight(rightTexPath);

        SpriteBoxOldIndex++;
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
    while (ModelOldIndex < ModelIndex)
    {
        const char* ModelPath = KeyMap_GetKey(&g_ObjectPool.ModelFileMap, ModelOldIndex);
        const char* TexturePath = KeyMap_GetKey(&g_ObjectPool.ModelTextureMap, ModelOldIndex);

        object->AddComponent<Model>()->SetModelPath(ModelPath);
        //object->GetComponent<Model>(ModelOldIndex)->SetTexture("asset/AFK_Snowman.png");
        

        ModelOldIndex++;
    }
    while (GridOldIndex < GridIndex)
    {

		object->AddComponent<Grid>()->SetGridType((GridType)VecInt_Get(&g_ObjectPool.GridTypeIndex, GridOldIndex));

		GridOldIndex++;
    }
}


//-----------------------------------------
// ライフサイクル
//-----------------------------------------
void InitDo()
{
    // インデックス初期化・ObjectIdx リセット
    UseCamera = -1;
    CameraIndex = UIIndex = SpriteWorldIndex = ModelIndex = BoxColliderIndex = GridIndex = 0;
    CameraOldIdx = UIOldIndex = SpriteWorldOldIndex = ModelOldIndex = BoxColliderOldIndex = GridOldIndex = 0;

    ObjectIdx.CameraIndex = 0;
    ObjectIdx.SpriteWorldIndex = 0;
    ObjectIdx.SpriteScreenIndex = 0;
    ObjectIdx.ModelIndex = 0;
    ObjectIdx.CollisionIndex = 0;
    ObjectIdx.GridIndex = 0;
    ObjectIdx.EffectIndex = 0;

    // Vec4Init（Pool 全部） --- ここを必ずすべて列挙することが重要
    ObjectDataPool* p = &g_ObjectPool;

    //クラスタイプを禁書目録共有
	KeyMap_Add(&g_ClassTypeMap, "Grid");
	KeyMap_SetKey(&g_ClassTypeMap, object->GetComponentType<Grid>(), "Grid");

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
    Vec4_Init(&p->CollisionPos);
    Vec4_Init(&p->CollisionSize);
    Vec4_Init(&p->CollisionAngle);

    // Grid Box / Polygon
    Vec4_Init(&p->GridPos);
    Vec4_Init(&p->GridSize);
    Vec4_Init(&p->GridAngle);
    Vec4_Init(&p->GridColor);
    // Int/Char/Bool vectors
    VecInt_Init(&p->GridSides);
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
    KeyMap_Init(&p->CollisionMap);
    KeyMap_Init(&p->GridMap);
    KeyMap_Init(&p->SpriteWorldTexturePathMap);
    KeyMap_Init(&p->SpriteScreenTexturePathMap);

    

    ShaderManager_Init();
    InitInput();

    // クラス取得
    grid = new Grid(nullptr);
    grid->Init();
    object = new Object();
    object->Init();
   
    //object->AddComponent<Sound>();
}

void UpdateDo()
{
    ShaderManager_Update();
    UpdateInput();

    CreateObject();
    UpdateScene();
    object->Update();
}

void DrawDo()
{
    DrawScene();
    //object->Draw();
    //object->GetComponent<Model>(0)->Draw();
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

    Vec4_Free(&p->CollisionPos);
    Vec4_Free(&p->CollisionSize);
    Vec4_Free(&p->CollisionAngle);

    Vec4_Free(&p->GridPos);
    Vec4_Free(&p->GridSize);
    Vec4_Free(&p->GridAngle);
    Vec4_Free(&p->GridColor);

    VecInt_Free(&p->GridSides);
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
    KeyMap_Free(&p->CollisionMap);
    KeyMap_Free(&p->GridMap);

    // オブジェクト解放
    if (object) { delete object; object = nullptr; }
    if (grid) { delete grid; grid = nullptr; }
}

void OutObjectIndex(ObjectIndex* out)
{
    *out = ObjectIdx;
}

void SetDefaultShaderVS3D(const char* ShaderName)
{

}
void SetDefaultShaderPS3D(const char* ShaderName)
{

}

void SetDefaultShaderVS2D(const char* ShaderName)
{

}
void SetDefaultShaderPS2D(const char* ShaderName)
{

}