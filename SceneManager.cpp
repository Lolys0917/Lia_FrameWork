// SceneManager.cpp（修正版）
// SceneRange の管理を行う。ObjectDataPool 側の完全初期化を前提に安全チェックを追加。
// Objectにインデックスを割り振り、Sceneごとに管理する仕組みを提供。

#include "Manager.h"
#include <vector>

// Scene範囲構造体（元通り）
typedef struct {
    int StartIndex_Grid, EndIndex_Grid;
    int StartIndex_GridBox, EndIndex_GridBox;
    int StartIndex_GridPolygon, EndIndex_GridPolygon;
    int StartIndex_Camera, EndIndex_Camera;
    int StartIndex_SpriteWorld, EndIndex_SpriteWorld;
    int StartIndex_SpriteScreen, EndIndex_SpriteScreen;
    int UseCameraIndex;
    bool Finalized;
} SceneRange;

static std::vector<SceneRange> SceneRanges;
static KeyMap SceneMap;
static int CurrentSceneIndex = -1;
static int ActiveSceneIndex = -1;
void SettingScene();
void SceneEndPoint();

//-----------------------------------------
// Scene操作
//-----------------------------------------
void AddScene(const char* name)
{
    KeyMap_Add(&SceneMap, name);
    int newIndex = KeyMap_GetIndex(&SceneMap, name);
    CurrentSceneIndex = newIndex;
    ActiveSceneIndex = newIndex; // ✅追加された瞬間アクティブ化

    ObjectIndex* idx = GetObjectIndex();
    SceneRange range{};
    range.StartIndex_Camera = idx->CameraIndex;
    range.EndIndex_Camera = idx->CameraIndex;
    range.StartIndex_SpriteWorld = idx->SpriteWorldIndex;
    range.EndIndex_SpriteWorld = idx->SpriteWorldIndex;
    range.StartIndex_SpriteScreen = idx->SpriteScreenIndex;
    range.EndIndex_SpriteScreen = idx->SpriteScreenIndex;
    range.StartIndex_GridBox = idx->GridBoxIndex;
    range.EndIndex_GridBox = idx->GridBoxIndex;
    range.StartIndex_GridPolygon = idx->GridPolygonIndex;
    range.EndIndex_GridPolygon = idx->GridPolygonIndex;
    range.StartIndex_Grid = idx->GridLineIndex;
    range.EndIndex_Grid = idx->GridLineIndex;
    range.UseCameraIndex = -1;
    range.Finalized = false;

    SceneRanges.push_back(range);
}

void SceneEndPoint()
{
    if (SceneRanges.empty()) return;
    ObjectIndex* idx = GetObjectIndex();
    SceneRange& r = SceneRanges[CurrentSceneIndex];

    r.EndIndex_Camera = idx->CameraIndex;
    r.EndIndex_SpriteWorld = idx->SpriteWorldIndex;
    r.EndIndex_SpriteScreen = idx->SpriteScreenIndex;
    r.EndIndex_GridBox = idx->GridBoxIndex;
    r.EndIndex_GridPolygon = idx->GridPolygonIndex;
    r.EndIndex_Grid = idx->GridLineIndex;
    r.Finalized = true;

    ActiveSceneIndex = -1;
}
void RefreshSceneRange()
{
    if (CurrentSceneIndex < 0 || CurrentSceneIndex >= (int)SceneRanges.size()) return;

    SceneRange& range = SceneRanges[CurrentSceneIndex];
    if (range.Finalized) return; // ✅ 確定済みは更新しない

    ObjectIndex* idx = GetObjectIndex();
    range.EndIndex_Camera = idx->CameraIndex;
    range.EndIndex_SpriteWorld = idx->SpriteWorldIndex;
    range.EndIndex_SpriteScreen = idx->SpriteScreenIndex;
    range.EndIndex_GridBox = idx->GridBoxIndex;
    range.EndIndex_GridPolygon = idx->GridPolygonIndex;
    range.EndIndex_Grid = idx->GridLineIndex;
}
//-----------------------------------------
// Scene初期化
//-----------------------------------------
void InitScene(const char* name)
{
    int index = KeyMap_GetIndex(&SceneMap, name);
    if (index == -1) { AddMessage(ConcatCStr("InitScene failed: ", name)); return; }

    SceneRange& range = SceneRanges[index];
    CurrentSceneIndex = index;
    ObjectDataPool* pool = GetObjectDataPool();
    //Camera
    int useCam = range.UseCameraIndex >= 0 ? range.UseCameraIndex : GetUseCamera();
    if (useCam >= 0 && useCam < (int)pool->CameraPos.size) {
        Vec4 pos = Vec4_Get(&pool->CameraPos, useCam);
        Vec4 look = Vec4_Get(&pool->CameraLook, useCam);
        // safety: object and component count
        if (GetObjectClass() && useCam >= 0) {
            GetObjectClass()->GetComponent<Camera>(useCam)->SetCameraView({ pos.X,pos.Y,pos.Z,0 }, { look.X,look.Y,look.Z,0 });
        }
    }
    //SpriteWorld
    for (int i = range.StartIndex_SpriteWorld; i < range.EndIndex_SpriteWorld; i++)
    {
        if (i < 0 || i >= (int)pool->GridBoxPos.size) continue;
        Vec4 v4Pos = Vec4_Get(&pool->SpriteWorldPos, i);
        GetObjectClass()->GetComponent<SpriteWorld>(i)->SetPos(v4Pos.X, v4Pos.Y, v4Pos.Z);
    }
    // Grid の初期化（ここでは色のみ復帰）
    for (int i = range.StartIndex_GridBox; i < range.EndIndex_GridBox; ++i)
    {
        if (i < 0 || i >= (int)pool->GridBoxColor.size) continue;
        Vec4 col = Vec4_Get(&pool->GridBoxColor, i);
        GetGridClass()->SetColor({ col.X, col.Y, col.Z, col.W });
    }
    for (int i = range.StartIndex_GridPolygon; i < range.EndIndex_GridPolygon; ++i)
    {
        if (i < 0 || i >= (int)pool->GridPolygonColor.size) continue;
        Vec4 col = Vec4_Get(&pool->GridPolygonColor, i);
        GetGridClass()->SetColor({ col.X, col.Y, col.Z, col.W });
    }

    AddMessage(ConcatCStr("InitScene(): ", name));
}

//-----------------------------------------
// Sceneコピー（簡易）
/* 元の処理を維持。コピー時にPool 内 Vec を push する処理は
   ObjectManager 側の API と合わせて呼ぶ実装が望ましいが、
   簡略版として SceneRange の複製で対応。 */
void CopyScene(const char* srcScene, const char* newScene)
{
    int srcIndex = KeyMap_GetIndex(&SceneMap, srcScene);
    if (srcIndex == -1) { AddMessage(ConcatCStr("CopyScene failed: ", srcScene)); return; }

    KeyMap_Add(&SceneMap, newScene);
    SceneRange src = SceneRanges[srcIndex];
    SceneRange dst = src;
    SceneRanges.push_back(dst);
    AddMessage(ConcatCStr("CopyScene(): ", newScene));
}

//-----------------------------------------
// Scene更新・描画
//-----------------------------------------
void UpdateScene()
{
    RefreshSceneRange();

    if (CurrentSceneIndex < 0 || CurrentSceneIndex >= (int)SceneRanges.size()) return;
    SceneRange& range = SceneRanges[CurrentSceneIndex];
    ObjectDataPool* pool = GetObjectDataPool();

    int cam = (range.UseCameraIndex >= 0) ? range.UseCameraIndex : GetUseCamera();
    if (cam < 0 || cam >= (int)pool->CameraPos.size) return;

    // Camera projection + view 更新（安全チェック）
    if (GetObjectClass()) {
        //Camera
        GetObjectClass()->GetComponent<Camera>(cam)->SetCameraProjection(70.0f, 800, 600);
        Vec4 pos = Vec4_Get(&pool->CameraPos, cam);
        Vec4 look = Vec4_Get(&pool->CameraLook, cam);
        GetObjectClass()->GetComponent<Camera>(cam)->SetCameraView({ pos.X,pos.Y,pos.Z,0 }, { look.X,look.Y,look.Z,0 });
    }
}

void DrawScene()
{
    if (CurrentSceneIndex < 0 || CurrentSceneIndex >= (int)SceneRanges.size()) return;
    SceneRange& range = SceneRanges[CurrentSceneIndex];
    ObjectDataPool* pool = GetObjectDataPool();

    int useCam = (range.UseCameraIndex >= 0) ? range.UseCameraIndex : GetUseCamera();
    if (useCam < 0 || useCam >= (int)pool->CameraPos.size) return;

    if (!GetGridClass() || !GetObjectClass()) return;


    GetGridClass()->SetProj(GetObjectClass()->GetComponent<Camera>(useCam)->GetProjection());
    GetGridClass()->SetView(GetObjectClass()->GetComponent<Camera>(useCam)->GetView());

    // GridBase
    GetGridClass()->SetColor({ 0,0,0,1 });

    for (int i = 0; i < 10; i++)
    {
        if (i != 5)
        {
            GetGridClass()->SetPos({ i - 5.0f, 0.0f, -5.0f }, { i - 5.0f, 0.0f, 5.0f });
            GetGridClass()->Draw();
            GetGridClass()->SetPos({ -5.0f, 0.0f, i - 5.0f }, { 5.0f, 0.0f, i - 5.0f });
            GetGridClass()->Draw();
        }
    }

    GetGridClass()->SetColor({ 1,0,0,1 });
    GetGridClass()->SetPos({ -5,0,0 }, { 5,0,0 }); GetGridClass()->Draw();
    GetGridClass()->SetColor({ 0,1,0,1 });
    GetGridClass()->SetPos({ 0,-5,0 }, { 0,5,0 }); GetGridClass()->Draw();
    GetGridClass()->SetColor({ 0,0,1,1 });
    GetGridClass()->SetPos({ 0,0,-5 }, { 0,0,5 }); GetGridClass()->Draw();

    // GridBox
    if (SceneRanges[CurrentSceneIndex].StartIndex_GridBox >= 0 && SceneRanges[CurrentSceneIndex].EndIndex_GridBox <= (int)pool->GridBoxPos.size) {
        for (int i = SceneRanges[CurrentSceneIndex].StartIndex_GridBox; i < SceneRanges[CurrentSceneIndex].EndIndex_GridBox; i++) {
            if (i < 0 || i >= (int)pool->GridBoxPos.size) continue;
            Vec4 pos = Vec4_Get(&pool->GridBoxPos, i);
            Vec4 size = Vec4_Get(&pool->GridBoxSize, i);
            Vec4 ang = Vec4_Get(&pool->GridBoxAngle, i);
            Vec4 col = Vec4_Get(&pool->GridBoxColor, i);
            GetGridClass()->SetColor({ col.X,col.Y,col.Z,col.W });
            GetGridClass()->DrawBox({ pos.X,pos.Y,pos.Z }, { size.X,size.Y,size.Z }, { ang.X,ang.Y,ang.Z });
        }
    }

    // GridPolygon
    if (SceneRanges[CurrentSceneIndex].StartIndex_GridPolygon >= 0 && SceneRanges[CurrentSceneIndex].EndIndex_GridPolygon <= (int)pool->GridPolygonPos.size) {
        for (int i = SceneRanges[CurrentSceneIndex].StartIndex_GridPolygon; i < SceneRanges[CurrentSceneIndex].EndIndex_GridPolygon; i++) {
            if (i < 0 || i >= (int)pool->GridPolygonPos.size) continue;
            Vec4 pos = Vec4_Get(&pool->GridPolygonPos, i);
            Vec4 size = Vec4_Get(&pool->GridPolygonSize, i);
            Vec4 ang = Vec4_Get(&pool->GridPolygonAngle, i);
            Vec4 col = Vec4_Get(&pool->GridPolygonColor, i);
            GetGridClass()->SetColor({ col.X,col.Y,col.Z,col.W });
            GetGridClass()->DrawGridPolygon(VecInt_Get(&pool->GridPolygonSides, i),
                { pos.X,pos.Y,pos.Z }, { size.X,size.Y,size.Z }, { ang.X,ang.Y,ang.Z });
        }
    }

    if (!GetObjectClass())
    {
        MessageBoxA(nullptr, "ObjectClassNULL", "Error", MB_OK);
    }

    //SpriteWorld
    if (SceneRanges[CurrentSceneIndex].StartIndex_SpriteWorld >= 0 && SceneRanges[CurrentSceneIndex].EndIndex_SpriteWorld <= (int)pool->SpriteWorldPos.size)
    {
        for (int i = SceneRanges[CurrentSceneIndex].StartIndex_SpriteWorld; i < SceneRanges[CurrentSceneIndex].EndIndex_SpriteWorld; i++)
        {
            if (i < 0 || i >= (int)pool->SpriteWorldPos.size) continue;
            Vec4 v4Pos = Vec4_Get(&pool->SpriteWorldPos, i);
            Vec4 v4Size = Vec4_Get(&pool->SpriteWorldSize, i);
            Vec4 v4Angle = Vec4_Get(&pool->SpriteWorldAngle, i);
            Vec4 v4Color = Vec4_Get(&pool->SpriteWorldColor, i);

            
            GetObjectClass()->GetComponent<SpriteWorld>(i)->SetColor({ v4Color.X, v4Color.Y, v4Color.Z, v4Color.W });
            GetObjectClass()->GetComponent<SpriteWorld>(i)->SetPos(v4Pos.X, v4Pos.Y, v4Pos.Z);
            GetObjectClass()->GetComponent<SpriteWorld>(i)->SetSize(v4Size.X, v4Size.Y);
            GetObjectClass()->GetComponent<SpriteWorld>(i)->SetAngle(v4Angle.X, v4Angle.Y, v4Angle.Z);

            //カメラ行列を渡す
            if (GetObjectClass()->GetComponent<Camera>(useCam)) {
                GetObjectClass()->GetComponent<SpriteWorld>(i)->SetView(GetObjectClass()->GetComponent<Camera>(useCam)->GetView());
                GetObjectClass()->GetComponent<SpriteWorld>(i)->SetProj(GetObjectClass()->GetComponent<Camera>(useCam)->GetProjection());
            }
            else
            {
                MessageBoxA(nullptr, "CameraNotFound", "SpriteWorld", MB_OK);
            }

        }
    }
    //SpriteScreen
    if (SceneRanges[CurrentSceneIndex].StartIndex_SpriteScreen >= 0 &&
        SceneRanges[CurrentSceneIndex].EndIndex_SpriteScreen <= (int)pool->SpriteScreenPos.size)
    {
        for (int i = SceneRanges[CurrentSceneIndex].StartIndex_SpriteScreen;
            i < SceneRanges[CurrentSceneIndex].EndIndex_SpriteScreen; ++i)
        {
            if (i < 0 || i >= (int)pool->SpriteScreenPos.size) continue;
            Vec4 v4Pos = Vec4_Get(&pool->SpriteScreenPos, i);
            Vec4 v4Size = Vec4_Get(&pool->SpriteScreenSize, i);
            Vec4 v4Color = Vec4_Get(&pool->SpriteScreenColor, i);
            int vIAngle = VecInt_Get(&pool->SpriteScreenAngle, i);

            GetObjectClass()->GetComponent<SpriteScreen>(i)->SetPos(v4Pos.X, v4Pos.Y);
            GetObjectClass()->GetComponent<SpriteScreen>(i)->SetSize(v4Size.X, v4Size.Y);
            GetObjectClass()->GetComponent<SpriteScreen>(i)->SetColor(v4Color.X, v4Color.Y, v4Color.Z, v4Color.W);
        }
    }
}

//-----------------------------------------
// その他
//-----------------------------------------
void ChangeScene(const char* name)
{
    int index = KeyMap_GetIndex(&SceneMap, name);
    if (index == -1) return;

    AddMessage(ConcatCStr("ChangeScene: ", name));
    CurrentSceneIndex = index;
    ActiveSceneIndex = index;
}

void NotifyAddObject(IndexType type)
{
    if (ActiveSceneIndex < 0 || ActiveSceneIndex >= (int)SceneRanges.size()) return;
    SceneRange& range = SceneRanges[ActiveSceneIndex];
    ObjectIndex* idx = GetObjectIndex();

    switch (type)
    {
    case IndexType::GridBox:
        range.EndIndex_GridBox = idx->GridBoxIndex;
        break;
    case IndexType::GridPolygon:
        range.EndIndex_GridPolygon = idx->GridPolygonIndex;
        break;
    case IndexType::Camera:
        range.EndIndex_Camera = idx->CameraIndex;
        break;
    case IndexType::GridLine:
        range.EndIndex_Grid = idx->GridLineIndex;
        break;
    }
}

const char* GetCurrentSceneName()
{
    if (CurrentSceneIndex < 0 || CurrentSceneIndex >= (int)SceneMap.size)
        return "None";
    return KeyMap_GetKey(&SceneMap, CurrentSceneIndex);
}

void SetSceneCamera(const char* s, const char* c)
{
    int si = KeyMap_GetIndex(&SceneMap, s);
    int ci = KeyMap_GetIndex(GetCameraKeyMap(), c);
    if (ci < 0 || si < 0) {
        AddMessage(ConcatCStr("SetSceneCamera failed: ", (ci < 0) ? c : s));
        return;
    }
    // シーンに割当るだけにする（UseCamera を直接変更しない）
    SceneRanges[si].UseCameraIndex = ci;
    AddMessage(ConcatCStr("SetSceneCamera: scene=", s));
}
void DeleteScene(const char* name) { /*元処理保持用ダミー*/ }
