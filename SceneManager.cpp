// SceneManager.cpp（修正版）
// SceneRange の管理を行う。ObjectDataPool 側の完全初期化を前提に安全チェックを追加。
// Objectにインデックスを割り振り、Sceneごとに管理する仕組みを提供。

//構造メモ
// 3次元でScene,ComponentType,ComponentIndexとして管理
//

#include "Manager.h"
#include "Component.h"
#include <vector>

//static Int2Vector SceneRangeIndex;
//static Vec4Vector SceneUtilIndex;
static IntVector CameraSceneIndex;
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

	VecInt_PushBack(&CameraSceneIndex, GetUseCamera());

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
    //
    // 初期フレーム初期化
    //

    // カメラのインデックス取得
	int cam = VecInt_Get(&CameraSceneIndex, CurrentSceneIndex);
    if (cam < 0 || cam >= (int)pool->CameraPos.size) return;
    SetUseCamera(cam);
	//MessageBoxA(NULL, ConcatCStr("UseCameraIndex:", std::to_string(cam).c_str()), "Debug", MB_OK);
    for (int i = 0; i < pool->CameraInfo.size; i++)
    {
        if (!(Vec4_Get(&pool->CameraInfo, i).X == CurrentSceneIndex)) continue;

        //MessageBoxA(NULL, ConcatCStr("UseCameraIndexInLoop:", std::to_string(i).c_str()), "Debug", MB_OK);

        if (Vec4_Get(&pool->CameraInfo, i).Y == GetObjectClass()->GetComponentType<Camera>())
        {
            //MessageBoxA(NULL, ConcatCStr("UseCameraIndexGo:", std::to_string(i).c_str()), "Debug", MB_OK);
            //Camera
            GetObjectClass()->GetComponent<Camera>(i)->SetCameraProjection(70.0f, 800, 600);
            Vec4 pos = Vec4_Get(&pool->CameraPos, i);
            Vec4 look = Vec4_Get(&pool->CameraLook, i);
			GetObjectClass()->GetComponent<Camera>(i)->SetCameraView({ pos.X,pos.Y,pos.Z,0 }, { look.X,look.Y,look.Z,0 });
        }
        else
        {
			//MessageBoxA(NULL, ConcatCStr("UseCameraIndexInLoop:", std::to_string(Vec4_Get(&pool->CameraInfo, i).Y).c_str()), "Error", MB_OK);
			//MessageBoxA(NULL, ConcatCStr("UseCameraIndexInLoop:", std::to_string(GetObjectClass()->GetComponentType<Camera>()).c_str()), "Error", MB_OK);
        }
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

    //SpriteWorld
    for(int i = 0; i < pool->SpriteWorldInfo.size; i++)
    {
        if (!(Vec4_Get(&pool->SpriteWorldInfo, i).X == CurrentSceneIndex)) continue;

        if (Vec4_Get(&pool->SpriteWorldInfo, i).Y == GetObjectClass()->GetComponentType<SpriteWorld>())
        {
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
            GetObjectClass()->DrawObject<SpriteWorld>(i);
        }
    }
    //SpriteBox
    for (int i = 0; i < pool->SpriteBoxInfo.size; i++)
    {
        if (!(Vec4_Get(&pool->SpriteBoxInfo, i).X == CurrentSceneIndex)) continue;

        if (Vec4_Get(&pool->SpriteBoxInfo, i).Y == GetObjectClass()->GetComponentType<SpriteBox>())
        {
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
            GetObjectClass()->DrawObject<SpriteBox>(i);
        }
    }
    //SpriteCylinder
    for (int i = 0; i < pool->SpriteCylinderInfo.size; i++)
    {
        if (!(Vec4_Get(&pool->SpriteCylinderInfo, i).X == CurrentSceneIndex)) continue;

        if (Vec4_Get(&pool->SpriteCylinderInfo, i).Y == GetObjectClass()->GetComponentType<SpriteCylinder>())
        {
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
            GetObjectClass()->DrawObject<SpriteCylinder>(i);
        }
    }

    //SpriteScreen
    for (int i = 0; i < pool->SpriteScreenInfo.size; i++)
    {
        if (!(Vec4_Get(&pool->SpriteScreenInfo, i).X == CurrentSceneIndex)) continue;

        if (Vec4_Get(&pool->SpriteScreenInfo, i).Y == GetObjectClass()->GetComponentType<SpriteScreen>())
        {
            Vec4 v4Pos = Vec4_Get(&pool->SpriteScreenPos, i);
            Vec4 v4Size = Vec4_Get(&pool->SpriteScreenSize, i);
            Vec4 v4Color = Vec4_Get(&pool->SpriteScreenColor, i);
            int vIAngle = VecInt_Get(&pool->SpriteScreenAngle, i);

            GetObjectClass()->GetComponent<SpriteScreen>(i)->SetPos2D(v4Pos.X, v4Pos.Y);
            GetObjectClass()->GetComponent<SpriteScreen>(i)->SetSize2D(v4Size.X, v4Size.Y);
            GetObjectClass()->GetComponent<SpriteScreen>(i)->SetColor(v4Color.X, v4Color.Y, v4Color.Z, v4Color.W);

            GetObjectClass()->DrawObject<SpriteScreen>(i);
        }
    }

    //Model
    for (int i = 0; i < pool->ModelInfo.size; i++)
    {
        if (!(Vec4_Get(&pool->ModelInfo, i).X == CurrentSceneIndex)) continue;

        if (Vec4_Get(&pool->ModelInfo, i).Y == GetObjectClass()->GetComponentType<Model>())
        {
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

            GetObjectClass()->DrawObject<Model>(i);
        }
    }
    //Collision
    for (int i = 0; i < pool->ModelInfo.size; i++)
    {
        if (!(Vec4_Get(&pool->ModelInfo, i).X == CurrentSceneIndex)) continue;

        if (Vec4_Get(&pool->ModelInfo, i).Y == GetObjectClass()->GetComponentType<Collision>())
        {
            Vec4 v4Pos = Vec4_Get(&pool->CollisionPos, i);
            Vec4 v4Size = Vec4_Get(&pool->CollisionSize, i);
            Vec4 v4Angle = Vec4_Get(&pool->CollisionAngle, i);

            GetObjectClass()->GetComponent<Collision>(i)->SetOffsetPos(v4Pos.X, v4Pos.Y, v4Pos.Z);
            GetObjectClass()->GetComponent<Collision>(i)->SetOffsetSize(v4Size.X, v4Size.Y, v4Size.Z);
            GetObjectClass()->GetComponent<Collision>(i)->SetOffsetAngle(v4Angle.X, v4Angle.Y, v4Angle.Z);

            GetObjectClass()->DrawObject<Collision>(i);
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
    
	float index = Vec4_Get(&GetObjectDataPool()->CameraInfo, ci).Z;

	Vec4_Set(&GetObjectDataPool()->CameraInfo, ci, {
        (float)si,
        Vec4_Get(&GetObjectDataPool()->CameraInfo, ci).Y, 
        Vec4_Get(&GetObjectDataPool()->CameraInfo, ci).Z,
        0 });
    
	//SetUseCamera(ci);

	VecInt_Set(&CameraSceneIndex, si, ci);

    AddMessage(ConcatCStr("SetSceneCamera: scene=", s));
}
//void DeleteScene(const char* name) { /*元処理保持用ダミー*/ }
