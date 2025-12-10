// SceneManager.cpp（修正版）
// SceneRange の管理を行う。ObjectDataPool 側の完全初期化を前提に安全チェックを追加。
// Objectにインデックスを割り振り、Sceneごとに管理する仕組みを提供。

#include "Manager.h"
#include "Component.h"
#include <vector>

// Scene範囲構造体（元通り）
typedef struct {
    int StartIndex_Grid, EndIndex_Grid;
    int StartIndex_Camera, EndIndex_Camera;
    int StartIndex_SpriteWorld, EndIndex_SpriteWorld;
    int StartIndex_SpriteScreen, EndIndex_SpriteScreen;
    int StartIndex_SpriteBox, EndIndex_SpriteBox;
    int StartIndex_SpriteCylinder, EndIndex_SpriteCylinder;
    int StartIndex_Model, EndIndex_Model;
    int StartIndex_Collision, EndIndex_Collision;
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
    ActiveSceneIndex = newIndex; // 追加された瞬間アaクティブ化

    ObjectIndex* idx = GetObjectIndex();
    SceneRange range{};
    range.StartIndex_Camera = idx->CameraIndex;
    range.EndIndex_Camera = idx->CameraIndex;
    range.StartIndex_SpriteWorld = idx->SpriteWorldIndex;
    range.EndIndex_SpriteWorld = idx->SpriteWorldIndex;
    range.StartIndex_SpriteScreen = idx->SpriteScreenIndex;
    range.EndIndex_SpriteScreen = idx->SpriteScreenIndex;
    range.StartIndex_SpriteBox = idx->SpriteBoxIndex;
    range.EndIndex_SpriteBox = idx->SpriteBoxIndex;
    range.StartIndex_SpriteCylinder = idx->SpriteCylinderIndex;
    range.EndIndex_SpriteCylinder = idx->SpriteCylinderIndex;
    range.StartIndex_Grid = idx->GridIndex;
    range.EndIndex_Grid = idx->GridIndex;
    range.StartIndex_Model = idx->ModelIndex;
    range.EndIndex_Model = idx->ModelIndex;
    range.StartIndex_Collision = idx->CollisionIndex;
    range.EndIndex_Collision = idx->CollisionIndex;
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
    r.EndIndex_SpriteBox = idx->SpriteBoxIndex;
    r.EndIndex_SpriteCylinder = idx->SpriteCylinderIndex;
    r.EndIndex_Grid = idx->GridIndex;
    r.EndIndex_Model = idx->ModelIndex;
    r.Finalized = true;

    ActiveSceneIndex = -1;
}
void RefreshSceneRange()
{
    // ActiveSceneIndex が有効ならそちらを優先して更新する（オブジェクト追加中のシーンを追跡）
    int targetScene = ActiveSceneIndex;
    if (targetScene < 0 || targetScene >= (int)SceneRanges.size()) {
        // Active が無効なら Current を参照するが、原則は Active を使う
        targetScene = CurrentSceneIndex;
    }
    if (targetScene < 0 || targetScene >= (int)SceneRanges.size()) return;

    SceneRange& range = SceneRanges[targetScene];
    if (range.Finalized) return; // 確定済みは更新しない

    ObjectIndex* idx = GetObjectIndex();
    // すべての EndIndex を現在のプールインデックスに追随させる
    range.EndIndex_Camera = idx->CameraIndex;
    range.EndIndex_SpriteWorld = idx->SpriteWorldIndex;
    range.EndIndex_SpriteScreen = idx->SpriteScreenIndex;
    range.EndIndex_SpriteBox = idx->SpriteBoxIndex;
    range.EndIndex_SpriteCylinder = idx->SpriteCylinderIndex;
    range.EndIndex_Grid = idx->GridIndex;
    range.EndIndex_Model = idx->ModelIndex;
    range.EndIndex_Collision = idx->CollisionIndex;
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
        if (i < 0 || i >= (int)pool->GridPos.size) continue;
        Vec4 v4Pos = Vec4_Get(&pool->SpriteWorldPos, i);
        GetObjectClass()->GetComponent<SpriteWorld>(i)->SetPosition(v4Pos.X, v4Pos.Y, v4Pos.Z);
    }
    // Grid の初期化（ここでは色のみ復帰）
    /*for (int i = range.StartIndex_Grid; i < range.EndIndex_Grid; ++i)
    {
        if (i < 0 || i >= (int)pool->GridColor.size) continue;
        Vec4 col = Vec4_Get(&pool->GridColor, i);
        GetGridClass()->SetColor({ col.X, col.Y, col.Z, col.W });
    }*/

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
    GetGridClass()->SetGridType(GridType::Grid_Line);
    for (int i = 0; i < 10; i++)
    {
        if (i != 5)
        {
            GetGridClass()->SetPosition( i - 5.0f, 0.0f, -5.0f);
            GetGridClass()->SetSize(i - 5.0f, 0.0f, 5.0f);
            GetGridClass()->Draw();
            GetGridClass()->SetPosition( -5.0f, 0.0f, i - 5.0f );
            GetGridClass()->SetSize( 5.0f, 0.0f, i - 5.0f );
            GetGridClass()->Draw();
        }
    }

    GetGridClass()->SetColor({ 1,0,0,1 });
    GetGridClass()->SetPosition( -5,0,0 ); 
    GetGridClass()->SetSize( 5,0,0 ); 
    GetGridClass()->Draw();
    GetGridClass()->SetColor({ 0,1,0,1 });
    GetGridClass()->SetPosition( 0,-5,0 );
    GetGridClass()->SetSize( 0,5,0 );
    GetGridClass()->Draw();
    GetGridClass()->SetColor({ 0,0,1,1 });
    GetGridClass()->SetPosition( 0,0,-5 );
    GetGridClass()->SetSize( 0,0,5 );
    GetGridClass()->Draw();

     //GridBox
    if (SceneRanges[CurrentSceneIndex].StartIndex_Grid >= 0 && SceneRanges[CurrentSceneIndex].EndIndex_Grid <= (int)pool->GridPos.size) {
        for (int i = SceneRanges[CurrentSceneIndex].StartIndex_Grid; i < SceneRanges[CurrentSceneIndex].EndIndex_Grid; i++) {
            if (i < 0 || i >= (int)pool->GridPos.size) continue;
            Vec4 pos = Vec4_Get(&pool->GridPos, i);
            Vec4 size = Vec4_Get(&pool->GridSize, i); 
            Vec4 ang = Vec4_Get(&pool->GridAngle, i);
            Vec4 col = Vec4_Get(&pool->GridColor, i);
			int sides = (int)VecInt_Get(&pool->GridSides, i);
			int gridTypeIndex = (int)VecInt_Get(&pool->GridTypeIndex, i);

            GetObjectClass()->GetComponent<Grid>(i)->SetColor({ col.X,col.Y,col.Z,col.W });
			GetObjectClass()->GetComponent<Grid>(i)->SetSides(sides);
			GetObjectClass()->GetComponent<Grid>(i)->SetGridType((GridType)gridTypeIndex);
            GetObjectClass()->GetComponent<Grid>(i)->SetPosition(pos.X, pos.Y, pos.Z);
            GetObjectClass()->GetComponent<Grid>(i)->SetSize(size.X, size.Y, size.Z);
            GetObjectClass()->GetComponent<Grid>(i)->SetAngle(ang.X, ang.Y, ang.Z);
            GetObjectClass()->GetComponent<Grid>(i)->SetBox({ pos.X,pos.Y,pos.Z }, { size.X,size.Y,size.Z }, { ang.X,ang.Y,ang.Z });
            //GetGridClass()->Draw();
            //カメラ行列を渡す
            if (GetObjectClass()->GetComponent<Camera>(useCam)) {
                GetObjectClass()->GetComponent<Grid>(i)->SetView(GetObjectClass()->GetComponent<Camera>(useCam)->GetView());
                GetObjectClass()->GetComponent<Grid>(i)->SetProj(GetObjectClass()->GetComponent<Camera>(useCam)->GetProjection());
            }
            else
            {
                MessageBoxA(nullptr, "CameraNotFound", "Grid", MB_OK);
            }
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
            GetObjectClass()->GetComponent<SpriteWorld>(i)->SetPosition(v4Pos.X, v4Pos.Y, v4Pos.Z);
            GetObjectClass()->GetComponent<SpriteWorld>(i)->SetSize(v4Size.X, v4Size.Y, 0);
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
    //SpriteBox
    if (SceneRanges[CurrentSceneIndex].StartIndex_SpriteBox >= 0 &&
        SceneRanges[CurrentSceneIndex].EndIndex_SpriteBox <= (int)pool->SpriteBoxPos.size)
    {
        for (int i = SceneRanges[CurrentSceneIndex].StartIndex_SpriteBox;
            i < SceneRanges[CurrentSceneIndex].EndIndex_SpriteBox; ++i)
        {
            if (i < 0 || i >= (int)pool->SpriteBoxPos.size) continue;
            Vec4 v4Pos = Vec4_Get(&pool->SpriteBoxPos, i);
            Vec4 v4Size = Vec4_Get(&pool->SpriteBoxSize, i);
            Vec4 v4Color = Vec4_Get(&pool->SpriteBoxColor, i);
            Vec4 v4Angle = Vec4_Get(&pool->SpriteBoxAngle, i);

            GetObjectClass()->GetComponent<SpriteBox>(i)->SetPosition(v4Pos.X, v4Pos.Y, v4Pos.Z);
            GetObjectClass()->GetComponent<SpriteBox>(i)->SetSize(v4Size.X, v4Size.Y, v4Size.Z);
            GetObjectClass()->GetComponent<SpriteBox>(i)->SetAngle(v4Angle.X, v4Angle.Y, v4Angle.Z);
            GetObjectClass()->GetComponent<SpriteBox>(i)->SetColor(v4Color.X, v4Color.Y, v4Color.Z, v4Color.W);

            //カメラ行列を渡す
            if (GetObjectClass()->GetComponent<Camera>(useCam)) {
                GetObjectClass()->GetComponent<SpriteBox>(i)->SetView(GetObjectClass()->GetComponent<Camera>(useCam)->GetView());
                GetObjectClass()->GetComponent<SpriteBox>(i)->SetProj(GetObjectClass()->GetComponent<Camera>(useCam)->GetProjection());
            }
            else
            {
                MessageBoxA(nullptr, "CameraNotFound", "SpriteWorld", MB_OK);
            }
        }
    }
    //SpriteCylinder
    if (SceneRanges[CurrentSceneIndex].StartIndex_SpriteCylinder >= 0 &&
        SceneRanges[CurrentSceneIndex].EndIndex_SpriteCylinder <= (int)pool->SpriteCylinderPos.size)
    {
        for (int i = SceneRanges[CurrentSceneIndex].StartIndex_SpriteCylinder;
            i < SceneRanges[CurrentSceneIndex].EndIndex_SpriteCylinder; ++i)
        {
            if (i < 0 || i >= (int)pool->SpriteCylinderPos.size) continue;
            Vec4 v4Pos = Vec4_Get(&pool->SpriteCylinderPos, i);
            Vec4 v4Size = Vec4_Get(&pool->SpriteCylinderSize, i);
            Vec4 v4Color = Vec4_Get(&pool->SpriteCylinderColor, i);
            Vec4 v4Angle = Vec4_Get(&pool->SpriteCylinderAngle, i);

            GetObjectClass()->GetComponent<SpriteCylinder>(i)->SetPosition(v4Pos.X, v4Pos.Y, v4Pos.Z);
            GetObjectClass()->GetComponent<SpriteCylinder>(i)->SetSize(v4Size.X, v4Size.Y, v4Size.Z);
            GetObjectClass()->GetComponent<SpriteCylinder>(i)->SetAngle(v4Angle.X, v4Angle.Y, v4Angle.Z);
            GetObjectClass()->GetComponent<SpriteCylinder>(i)->SetColor(v4Color.X, v4Color.Y, v4Color.Z, v4Color.W);

            //カメラ行列を渡す
            if (GetObjectClass()->GetComponent<Camera>(useCam)) {
                GetObjectClass()->GetComponent<SpriteCylinder>(i)->SetView(GetObjectClass()->GetComponent<Camera>(useCam)->GetView());
                GetObjectClass()->GetComponent<SpriteCylinder>(i)->SetProj(GetObjectClass()->GetComponent<Camera>(useCam)->GetProjection());
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

            GetObjectClass()->GetComponent<SpriteScreen>(i)->SetPos2D(v4Pos.X, v4Pos.Y);
            GetObjectClass()->GetComponent<SpriteScreen>(i)->SetSize2D(v4Size.X, v4Size.Y);
            GetObjectClass()->GetComponent<SpriteScreen>(i)->SetColor(v4Color.X, v4Color.Y, v4Color.Z, v4Color.W);
        }
    }
    //Model
    if (SceneRanges[CurrentSceneIndex].StartIndex_Model >= 0 &&
        SceneRanges[CurrentSceneIndex].EndIndex_Model <= (int)pool->ModelPos.size)
    {
        for (int i = SceneRanges[CurrentSceneIndex].StartIndex_Model;
            i < SceneRanges[CurrentSceneIndex].EndIndex_Model; ++i)
        {
            if (i < 0 || i >= (int)pool->ModelPos.size) continue;
            Vec4 v4Pos = Vec4_Get(&pool->ModelPos, i);
            Vec4 v4Size = Vec4_Get(&pool->ModelSize, i);
            Vec4 v4Angle = Vec4_Get(&pool->ModelAngle, i);

            GetObjectClass()->GetComponent<Model>(i)->SetPosition(v4Pos.X, v4Pos.Y, v4Pos.Z);
            GetObjectClass()->GetComponent<Model>(i)->SetSize(v4Size.X, v4Size.Y, v4Size.Z);
            GetObjectClass()->GetComponent<Model>(i)->SetAngle(v4Angle.X, v4Angle.Y, v4Angle.Z);

            //カメラ行列を渡す
            if (GetObjectClass()->GetComponent<Camera>(useCam)) {
                GetObjectClass()->GetComponent<Model>(i)->SetView(GetObjectClass()->GetComponent<Camera>(useCam)->GetView());
                GetObjectClass()->GetComponent<Model>(i)->SetProj(GetObjectClass()->GetComponent<Camera>(useCam)->GetProjection());
            }
            else
            {
                MessageBoxA(nullptr, "CameraNotFound", "Model", MB_OK);
            }
        }
    }
    //Collision
    if (SceneRanges[CurrentSceneIndex].StartIndex_Collision >= 0 &&
        SceneRanges[CurrentSceneIndex].EndIndex_Collision <= (int)pool->CollisionPos.size)
    {
        for (int i = SceneRanges[CurrentSceneIndex].StartIndex_Collision;
            i < SceneRanges[CurrentSceneIndex].EndIndex_Collision; ++i)
        {
            if (i < 0 || i >= (int)pool->CollisionPos.size) continue;
            Vec4 v4Pos = Vec4_Get(&pool->CollisionPos, i);
            Vec4 v4Size = Vec4_Get(&pool->CollisionSize, i);
            Vec4 v4Angle = Vec4_Get(&pool->CollisionAngle, i);

            GetObjectClass()->GetComponent<Collision>(i)->SetOffsetPos(v4Pos.X, v4Pos.Y, v4Pos.Z);
            GetObjectClass()->GetComponent<Collision>(i)->SetOffsetSize(v4Size.X, v4Size.Y, v4Size.Z);
            GetObjectClass()->GetComponent<Collision>(i)->SetOffsetAngle(v4Angle.X, v4Angle.Y, v4Angle.Z);

            //

            //カメラ行列を渡す
            if (GetObjectClass()->GetComponent<Camera>(useCam)) {
                GetObjectClass()->GetComponent<Model>(i)->SetView(GetObjectClass()->GetComponent<Camera>(useCam)->GetView());
                GetObjectClass()->GetComponent<Model>(i)->SetProj(GetObjectClass()->GetComponent<Camera>(useCam)->GetProjection());
            }
            else
            {
                MessageBoxA(nullptr, "CameraNotFound", "Collision", MB_OK);
            }
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
    case IndexType::Grid:
        range.EndIndex_Grid = idx->GridIndex;
        break;
    case IndexType::Camera:
        range.EndIndex_Camera = idx->CameraIndex;
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
