// SceneManager.cpp（修正版）
// SceneRange の管理を行う。ObjectDataPool 側の完全初期化を前提に安全チェックを追加。
// Objectにインデックスを割り振り、Sceneごとに管理する仕組みを提供。

//構造メモ
// 3次元でScene,ComponentType,ComponentIndexとして管理
//

#include "Manager.h"
#include "Component.h"
#include <vector>

static Int2Vector SceneRangeIndex;
static Vec4Vector SceneUtilIndex;
static KeyMap SceneMap;
static int CurrentSceneIndex = -1;
//static int ActiveSceneIndex = -1;
//void SettingScene();
//void SceneEndPoint();

//-----------------------------------------
// Scene操作
//-----------------------------------------
void AddScene(const char* name)
{
    KeyMap_Add(&SceneMap, name);
    int newIndex = KeyMap_GetIndex(&SceneMap, name);
    CurrentSceneIndex = newIndex;
    //ActiveSceneIndex = newIndex; // 追加された瞬間アaクティブ化

    ObjectIndex* idx = GetObjectIndex();
}

int GetCurrentSceneIndex()
{
    return CurrentSceneIndex;
}
//-----------------------------------------
// Scene初期化
//-----------------------------------------
void InitScene(const char* name)
{

}

//-----------------------------------------
// Sceneコピー（簡易）
/* 元の処理を維持。コピー時にPool 内 Vec を push する処理は
   ObjectManager 側の API と合わせて呼ぶ実装が望ましいが、
   簡略版として SceneRange の複製で対応。 */
void CopyScene(const char* srcScene, const char* newScene)
{

}

//
//アップデート関連に関するメモ
//
// ループ時に判定文でCurrentSceneと同じインデックスを所有しているかを判定する
// つまりオブジェクトを全判定して更新をするのか否かを判定することができる
// Vec4VectorにおいてSceneIndex, ComponentType, ComponentIndex, NULLといった形を持つ
//

//-----------------------------------------
// Scene更新・描画
//-----------------------------------------
void UpdateScene()
{
    //RefreshSceneRange();

    //if (CurrentSceneIndex < 0) return;
    ////SceneRange range = SceneRange_Get(&SRVec, CurrentSceneIndex);
    ObjectDataPool* pool = GetObjectDataPool();

    int cam = /*(range.UseCameraIndex >= 0) ? range.UseCameraIndex : */GetUseCamera();
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
    //if (CurrentSceneIndex < 0) return;
    ////SceneRange range = SceneRange_Get(&SRVec, CurrentSceneIndex);
    ObjectDataPool* pool = GetObjectDataPool();

    int useCam = GetUseCamera();
    //if (useCam < 0 || useCam >= (int)pool->CameraPos.size) return;

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

    for (int i = 0; i < pool->GridInfo.size; i++)
    {
        if (!(Vec4_Get(&pool->GridInfo, i).X == CurrentSceneIndex)) continue;

        //MessageBoxA(NULL, ConcatCStr("CurrentSceneData:", std::to_string(pool->GridInfo.data->X).c_str()), "Debug", MB_OK);
        //MessageBoxA(NULL, ConcatCStr("CurrentSceneIndex:", std::to_string(CurrentSceneIndex).c_str()), "Debug", MB_OK);

        if (Vec4_Get(&pool->GridInfo, i).Y == GetObjectClass()->GetComponentType<Grid>())
        {
			//MessageBoxA(nullptr, "DrawGrid", "Info", MB_OK);
            
			//カメラ行列を渡す
            GetObjectClass()->GetComponent<Grid>(i)->SetProj(GetObjectClass()->GetComponent<Camera>(useCam)->GetProjection());
            GetObjectClass()->GetComponent<Grid>(i)->SetView(GetObjectClass()->GetComponent<Camera>(useCam)->GetView());
            //グリッドの種類を指定
            GridType GridTypeIndex = static_cast<GridType>(VecInt_Get(&pool->GridTypeIndex, i));
			GetObjectClass()->GetComponent<Grid>(i)->SetGridType(GridTypeIndex);
            //数値設定
			Vec4 v4Pos = Vec4_Get(&pool->GridPos, i);
			Vec4 v4Size = Vec4_Get(&pool->GridSize, i);
			Vec4 v4Ang = Vec4_Get(&pool->GridAngle, i);
			Vec4 v4Col = Vec4_Get(&pool->GridColor, i);
			int sides = (int)VecInt_Get(&pool->GridSides, i);
			GetObjectClass()->GetComponent<Grid>(i)->SetColor({ v4Col.X,v4Col.Y,v4Col.Z,v4Col.W });
			GetObjectClass()->GetComponent<Grid>(i)->SetPosition(v4Pos.X, v4Pos.Y, v4Pos.Z);
			GetObjectClass()->GetComponent<Grid>(i)->SetSize(v4Size.X, v4Size.Y, v4Size.Z);
			GetObjectClass()->GetComponent<Grid>(i)->SetAngle(v4Ang.X, v4Ang.Y, v4Ang.Z);
			GetObjectClass()->GetComponent<Grid>(i)->SetSides(sides);
			//描画
            GetObjectClass()->DrawObject<Grid>(i);
        }
    }



   //  //GridBox
   // if (SceneRange_Get(&SRVec, CurrentSceneIndex).StartIndex_Grid >= 0 && SceneRange_Get(&SRVec, CurrentSceneIndex).EndIndex_Grid <= (int)pool->GridPos.size) {
   //     for (int i = SceneRange_Get(&SRVec, CurrentSceneIndex).StartIndex_Grid; i < SceneRange_Get(&SRVec, CurrentSceneIndex).EndIndex_Grid; i++) {
   //         if (i < 0 || i >= (int)pool->GridPos.size) continue;
   //         Vec4 pos = Vec4_Get(&pool->GridPos, i);
   //         Vec4 size = Vec4_Get(&pool->GridSize, i); 
   //         Vec4 ang = Vec4_Get(&pool->GridAngle, i);
   //         Vec4 col = Vec4_Get(&pool->GridColor, i);
			//int sides = (int)VecInt_Get(&pool->GridSides, i);
			//int gridTypeIndex = (int)VecInt_Get(&pool->GridTypeIndex, i);

   //         GetObjectClass()->GetComponent<Grid>(i)->SetColor({ col.X,col.Y,col.Z,col.W });
			//GetObjectClass()->GetComponent<Grid>(i)->SetSides(sides);
			//GetObjectClass()->GetComponent<Grid>(i)->SetGridType((GridType)gridTypeIndex);
   //         GetObjectClass()->GetComponent<Grid>(i)->SetPosition(pos.X, pos.Y, pos.Z);
   //         GetObjectClass()->GetComponent<Grid>(i)->SetSize(size.X, size.Y, size.Z);
   //         GetObjectClass()->GetComponent<Grid>(i)->SetAngle(ang.X, ang.Y, ang.Z);
   //         //GetObjectClass()->GetComponent<Grid>(i)->SetBox({ pos.X,pos.Y,pos.Z }, { size.X,size.Y,size.Z }, { ang.X,ang.Y,ang.Z });
   //         //GetGridClass()->Draw();
   //         //カメラ行列を渡す
   //         if (GetObjectClass()->GetComponent<Camera>(useCam)) {
   //             GetObjectClass()->GetComponent<Grid>(i)->SetView(GetObjectClass()->GetComponent<Camera>(useCam)->GetView());
   //             GetObjectClass()->GetComponent<Grid>(i)->SetProj(GetObjectClass()->GetComponent<Camera>(useCam)->GetProjection());
   //         }
   //         else
   //         {
   //             MessageBoxA(nullptr, "CameraNotFound", "Grid", MB_OK);
   //         }
			////GetObjectClass()->DrawComponent<Grid>(SceneRange_Get(&SRVec, CurrentSceneIndex).StartIndex_Grid, SceneRange_Get(&SRVec, CurrentSceneIndex).EndIndex_Grid);
   //         GetObjectClass()->DrawObject<Grid>(i);
   //     }
   // }

   // if (!GetObjectClass())
   // {
   //     MessageBoxA(nullptr, "ObjectClassNULL", "Error", MB_OK);
   // }

   // //SpriteWorld
   // if (SceneRange_Get(&SRVec, CurrentSceneIndex).StartIndex_SpriteWorld >= 0 && SceneRange_Get(&SRVec, CurrentSceneIndex).EndIndex_SpriteWorld <= (int)pool->SpriteWorldPos.size)
   // {
   //     for (int i = SceneRange_Get(&SRVec, CurrentSceneIndex).StartIndex_SpriteWorld; i < SceneRange_Get(&SRVec, CurrentSceneIndex).EndIndex_SpriteWorld; i++)
   //     {
   //         if (i < 0 || i >= (int)pool->SpriteWorldPos.size) continue;
   //         Vec4 v4Pos = Vec4_Get(&pool->SpriteWorldPos, i);
   //         Vec4 v4Size = Vec4_Get(&pool->SpriteWorldSize, i);
   //         Vec4 v4Angle = Vec4_Get(&pool->SpriteWorldAngle, i);
   //         Vec4 v4Color = Vec4_Get(&pool->SpriteWorldColor, i);

   //         
   //         GetObjectClass()->GetComponent<SpriteWorld>(i)->SetColor({ v4Color.X, v4Color.Y, v4Color.Z, v4Color.W });
   //         GetObjectClass()->GetComponent<SpriteWorld>(i)->SetPosition(v4Pos.X, v4Pos.Y, v4Pos.Z);
   //         GetObjectClass()->GetComponent<SpriteWorld>(i)->SetSize(v4Size.X, v4Size.Y, 0);
   //         GetObjectClass()->GetComponent<SpriteWorld>(i)->SetAngle(v4Angle.X, v4Angle.Y, v4Angle.Z);

   //         //カメラ行列を渡す
   //         if (GetObjectClass()->GetComponent<Camera>(useCam)) {
   //             GetObjectClass()->GetComponent<SpriteWorld>(i)->SetView(GetObjectClass()->GetComponent<Camera>(useCam)->GetView());
   //             GetObjectClass()->GetComponent<SpriteWorld>(i)->SetProj(GetObjectClass()->GetComponent<Camera>(useCam)->GetProjection());
   //         }
   //         else
   //         {
   //             MessageBoxA(nullptr, "CameraNotFound", "SpriteWorld", MB_OK);
   //         }
   //         GetObjectClass()->DrawObject<SpriteWorld>(i);
   //     }
   // }
   // //SpriteBox
   // if (SceneRange_Get(&SRVec, CurrentSceneIndex).StartIndex_SpriteBox >= 0 &&
   //     SceneRange_Get(&SRVec, CurrentSceneIndex).EndIndex_SpriteBox <= (int)pool->SpriteBoxPos.size)
   // {
   //     for (int i = SceneRange_Get(&SRVec, CurrentSceneIndex).StartIndex_SpriteBox;
   //         i < SceneRange_Get(&SRVec, CurrentSceneIndex).EndIndex_SpriteBox; ++i)
   //     {
   //         if (i < 0 || i >= (int)pool->SpriteBoxPos.size) continue;
   //         Vec4 v4Pos = Vec4_Get(&pool->SpriteBoxPos, i);
   //         Vec4 v4Size = Vec4_Get(&pool->SpriteBoxSize, i);
   //         Vec4 v4Color = Vec4_Get(&pool->SpriteBoxColor, i);
   //         Vec4 v4Angle = Vec4_Get(&pool->SpriteBoxAngle, i);

   //         GetObjectClass()->GetComponent<SpriteBox>(i)->SetPosition(v4Pos.X, v4Pos.Y, v4Pos.Z);
   //         GetObjectClass()->GetComponent<SpriteBox>(i)->SetSize(v4Size.X, v4Size.Y, v4Size.Z);
   //         GetObjectClass()->GetComponent<SpriteBox>(i)->SetAngle(v4Angle.X, v4Angle.Y, v4Angle.Z);
   //         GetObjectClass()->GetComponent<SpriteBox>(i)->SetColor(v4Color.X, v4Color.Y, v4Color.Z, v4Color.W);

   //         //カメラ行列を渡す
   //         if (GetObjectClass()->GetComponent<Camera>(useCam)) {
   //             GetObjectClass()->GetComponent<SpriteBox>(i)->SetView(GetObjectClass()->GetComponent<Camera>(useCam)->GetView());
   //             GetObjectClass()->GetComponent<SpriteBox>(i)->SetProj(GetObjectClass()->GetComponent<Camera>(useCam)->GetProjection());
   //         }
   //         else
   //         {
   //             MessageBoxA(nullptr, "CameraNotFound", "SpriteWorld", MB_OK);
   //         }
   //         GetObjectClass()->DrawObject<SpriteBox>(i);
   //     }
   // }
   // //SpriteCylinder
   // if (SceneRange_Get(&SRVec, CurrentSceneIndex).StartIndex_SpriteCylinder >= 0 &&
   //     SceneRange_Get(&SRVec, CurrentSceneIndex).EndIndex_SpriteCylinder <= (int)pool->SpriteCylinderPos.size)
   // {
   //     for (int i = SceneRange_Get(&SRVec, CurrentSceneIndex).StartIndex_SpriteCylinder;
   //         i < SceneRange_Get(&SRVec, CurrentSceneIndex).EndIndex_SpriteCylinder; ++i)
   //     {
   //         if (i < 0 || i >= (int)pool->SpriteCylinderPos.size) continue;
   //         Vec4 v4Pos = Vec4_Get(&pool->SpriteCylinderPos, i);
   //         Vec4 v4Size = Vec4_Get(&pool->SpriteCylinderSize, i);
   //         Vec4 v4Color = Vec4_Get(&pool->SpriteCylinderColor, i);
   //         Vec4 v4Angle = Vec4_Get(&pool->SpriteCylinderAngle, i);

   //         GetObjectClass()->GetComponent<SpriteCylinder>(i)->SetPosition(v4Pos.X, v4Pos.Y, v4Pos.Z);
   //         GetObjectClass()->GetComponent<SpriteCylinder>(i)->SetSize(v4Size.X, v4Size.Y, v4Size.Z);
   //         GetObjectClass()->GetComponent<SpriteCylinder>(i)->SetAngle(v4Angle.X, v4Angle.Y, v4Angle.Z);
   //         GetObjectClass()->GetComponent<SpriteCylinder>(i)->SetColor(v4Color.X, v4Color.Y, v4Color.Z, v4Color.W);

   //         //カメラ行列を渡す
   //         if (GetObjectClass()->GetComponent<Camera>(useCam)) {
   //             GetObjectClass()->GetComponent<SpriteCylinder>(i)->SetView(GetObjectClass()->GetComponent<Camera>(useCam)->GetView());
   //             GetObjectClass()->GetComponent<SpriteCylinder>(i)->SetProj(GetObjectClass()->GetComponent<Camera>(useCam)->GetProjection());
   //         }
   //         else
   //         {
   //             MessageBoxA(nullptr, "CameraNotFound", "SpriteWorld", MB_OK);
   //         }
   //         GetObjectClass()->DrawObject<SpriteCylinder>(i);
   //     }
   // }
   // //SpriteScreen
   // if (SceneRange_Get(&SRVec, CurrentSceneIndex).StartIndex_SpriteScreen >= 0 &&
   //     SceneRange_Get(&SRVec, CurrentSceneIndex).EndIndex_SpriteScreen <= (int)pool->SpriteScreenPos.size)
   // {
   //     for (int i = SceneRange_Get(&SRVec, CurrentSceneIndex).StartIndex_SpriteScreen;
   //         i < SceneRange_Get(&SRVec, CurrentSceneIndex).EndIndex_SpriteScreen; ++i)
   //     {
   //         if (i < 0 || i >= (int)pool->SpriteScreenPos.size) continue;
   //         Vec4 v4Pos = Vec4_Get(&pool->SpriteScreenPos, i);
   //         Vec4 v4Size = Vec4_Get(&pool->SpriteScreenSize, i);
   //         Vec4 v4Color = Vec4_Get(&pool->SpriteScreenColor, i);
   //         int vIAngle = VecInt_Get(&pool->SpriteScreenAngle, i);

   //         GetObjectClass()->GetComponent<SpriteScreen>(i)->SetPos2D(v4Pos.X, v4Pos.Y);
   //         GetObjectClass()->GetComponent<SpriteScreen>(i)->SetSize2D(v4Size.X, v4Size.Y);
   //         GetObjectClass()->GetComponent<SpriteScreen>(i)->SetColor(v4Color.X, v4Color.Y, v4Color.Z, v4Color.W);
   //     
   //         GetObjectClass()->DrawObject<SpriteScreen>(i);
   //     }
   // }
   // //Model
   // if (SceneRange_Get(&SRVec, CurrentSceneIndex).StartIndex_Model >= 0 &&
   //     SceneRange_Get(&SRVec, CurrentSceneIndex).EndIndex_Model <= (int)pool->ModelPos.size)
   // {
   //     for (int i = SceneRange_Get(&SRVec, CurrentSceneIndex).StartIndex_Model;
   //         i < SceneRange_Get(&SRVec, CurrentSceneIndex).EndIndex_Model; ++i)
   //     {
   //         if (i < 0 || i >= (int)pool->ModelPos.size) continue;
   //         Vec4 v4Pos = Vec4_Get(&pool->ModelPos, i);
   //         Vec4 v4Size = Vec4_Get(&pool->ModelSize, i);
   //         Vec4 v4Angle = Vec4_Get(&pool->ModelAngle, i);

   //         GetObjectClass()->GetComponent<Model>(i)->SetPosition(v4Pos.X, v4Pos.Y, v4Pos.Z);
   //         GetObjectClass()->GetComponent<Model>(i)->SetSize(v4Size.X, v4Size.Y, v4Size.Z);
   //         GetObjectClass()->GetComponent<Model>(i)->SetAngle(v4Angle.X, v4Angle.Y, v4Angle.Z);

   //         //カメラ行列を渡す
   //         if (GetObjectClass()->GetComponent<Camera>(useCam)) {
   //             GetObjectClass()->GetComponent<Model>(i)->SetView(GetObjectClass()->GetComponent<Camera>(useCam)->GetView());
   //             GetObjectClass()->GetComponent<Model>(i)->SetProj(GetObjectClass()->GetComponent<Camera>(useCam)->GetProjection());
   //         }
   //         else
   //         {
   //             MessageBoxA(nullptr, "CameraNotFound", "Model", MB_OK);
   //         }

   //         GetObjectClass()->DrawObject<Model>(i);
   //     }
   // }
   // //Collision
   // if (SceneRange_Get(&SRVec, CurrentSceneIndex).StartIndex_Collision >= 0 &&
   //     SceneRange_Get(&SRVec, CurrentSceneIndex).EndIndex_Collision <= (int)pool->CollisionPos.size)
   // {
   //     for (int i = SceneRange_Get(&SRVec, CurrentSceneIndex).StartIndex_Collision;
   //         i < SceneRange_Get(&SRVec, CurrentSceneIndex).EndIndex_Collision; ++i)
   //     {
   //         if (i < 0 || i >= (int)pool->CollisionPos.size) continue;
   //         Vec4 v4Pos = Vec4_Get(&pool->CollisionPos, i);
   //         Vec4 v4Size = Vec4_Get(&pool->CollisionSize, i);
   //         Vec4 v4Angle = Vec4_Get(&pool->CollisionAngle, i);

   //         GetObjectClass()->GetComponent<Collision>(i)->SetOffsetPos(v4Pos.X, v4Pos.Y, v4Pos.Z);
   //         GetObjectClass()->GetComponent<Collision>(i)->SetOffsetSize(v4Size.X, v4Size.Y, v4Size.Z);
   //         GetObjectClass()->GetComponent<Collision>(i)->SetOffsetAngle(v4Angle.X, v4Angle.Y, v4Angle.Z);

   //         GetObjectClass()->DrawObject<Collision>(i);
   //     }
   // }
}

//-----------------------------------------
// その他
//-----------------------------------------
void ChangeScene(const char* name)
{
    int index = KeyMap_GetIndex(&SceneMap, name);
    if (index == -1) return;
//
//    AddMessage(ConcatCStr("ChangeScene: ", name));
    CurrentSceneIndex = index;
//    ActiveSceneIndex = index;
}

void SetSceneCamera(const char* s, const char* c)
{
    int si = KeyMap_GetIndex(&SceneMap, s);
    int ci = KeyMap_GetIndex(GetCameraKeyMap(), c);
    if (ci < 0 || si < 0) {
        AddMessage(ConcatCStr("SetSceneCamera failed: ", (ci < 0) ? c : s));
        return;
    }
    
    
    AddMessage(ConcatCStr("SetSceneCamera: scene=", s));
}
//void DeleteScene(const char* name) { /*元処理保持用ダミー*/ }
