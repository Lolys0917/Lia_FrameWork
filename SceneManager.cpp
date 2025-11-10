#include "Manager.h"
#include <vector>

// Scene範囲構造体
typedef struct {
    int StartIndex_Grid, EndIndex_Grid;
    int StartIndex_GridBox, EndIndex_GridBox;
    int StartIndex_GridPolygon, EndIndex_GridPolygon;
    int StartIndex_Camera, EndIndex_Camera;
    int UseCameraIndex;
} SceneRange;


static std::vector<SceneRange> SceneRanges;
static KeyMap SceneMap;
static int CurrentSceneIndex = -1;

int SceneCount = 0;
int SceneEndFlag = 0;


//-------------------------------------
void AddScene(const char* name) {
    KeyMap_Add(&SceneMap, name);

    SceneRange range{};
    range.StartIndex_Camera = GetObjectIndex()->CameraIndex;
    range.EndIndex_Camera = GetObjectIndex()->CameraIndex;
    range.UseCameraIndex = -1;

    range.StartIndex_GridBox = GetObjectIndex()->GridBoxIndex;
    range.EndIndex_GridBox = GetObjectIndex()->GridBoxIndex;
    range.StartIndex_GridPolygon = GetObjectIndex()->GridPolygonIndex;
    range.EndIndex_GridPolygon = GetObjectIndex()->GridPolygonIndex;
    range.StartIndex_Grid = GetObjectIndex()->GridLineIndex;
    range.EndIndex_Grid = GetObjectIndex()->GridLineIndex;

    SceneRanges.push_back(range);
    SceneCount++;
    SceneEndFlag = 0;
}

void SceneEndPoint() {
    if (SceneCount <= 0) return;
    auto& r = SceneRanges.back();
    r.EndIndex_Camera = GetObjectIndex()->CameraIndex;
    r.EndIndex_GridBox = GetObjectIndex()->GridBoxIndex;
    r.EndIndex_GridPolygon = GetObjectIndex()->GridPolygonIndex;
    r.EndIndex_Grid = GetObjectIndex()->GridLineIndex;
    SceneEndFlag = 1;
}

void InitScene(const char* name) {
    int index = KeyMap_GetIndex(&SceneMap, name);
    if (index == -1) {
        AddMessage("\nerror : InitScene failed (scene not found)\n");
        return;
    }

    SceneRange& range = SceneRanges[index];
    CurrentSceneIndex = index;

    // カメラ再設定
    int useCam = range.UseCameraIndex >= 0 ? range.UseCameraIndex : GetUseCamera();
    if (useCam >= 0 && useCam < GetObjectIndex()->CameraIndex) {
        Vec4 vec4Pos = Vec4_Get(&CameraPosVec4, useCam);
        Vec4 vec4Look = Vec4_Get(&CameraLookVec4, useCam);
        XMFLOAT4 CamPos = { vec4Pos.X, vec4Pos.Y, vec4Pos.Z, 0.0f };
        XMFLOAT4 CamLook = { vec4Look.X, vec4Look.Y, vec4Look.Z, 0.0f };
        GetObjectClass()->GetComponent<Camera>(useCam)->SetCameraView(CamPos, CamLook);
    }

    // シーン範囲内オブジェクトを再初期化
    for (int i = range.StartIndex_GridBox; i < range.EndIndex_GridBox; i++) {
        // 位置や色などを再設定して復帰
        Vec4 col = Vec4_Get(&GridBoxColorVec4, i);
        GetGridClass()->SetColor({col.X, col.Y, col.Z, col.W});
    }
    for (int i = range.StartIndex_GridPolygon; i < range.EndIndex_GridPolygon; i++) {
        // 必要があれば再初期化
        Vec4 col = Vec4_Get(&GridPolygonColorVec4, i);
        GetGridClass()->SetColor({ col.X, col.Y, col.Z, col.W });
    }

    AddMessage(ConcatCStr("InitScene(): ", name));
}
void DeleteScene(const char* name) {
    int index = KeyMap_GetIndex(&SceneMap, name);
    if (index == -1) {
        AddMessage("\nerror : DeleteScene failed (scene not found)\n");
        return;
    }

    // 削除対象シーン
    SceneRange& range = SceneRanges[index];

    // --- Vec4 / Int 内のデータを削除（安全な削除処理） ---
    auto erase_range = [&](auto& vec, int start, int end) {
        if (start >= 0 && end <= (int)vec.size && start < end) {
            int count = end - start;
            memmove(&vec.data[start], &vec.data[end],
                (vec.size - end) * sizeof(vec.data[0]));
            vec.size -= count;
        }
        };

    erase_range(GridBoxPosVec4, range.StartIndex_GridBox, range.EndIndex_GridBox);
    erase_range(GridBoxColorVec4, range.StartIndex_GridBox, range.EndIndex_GridBox);
    erase_range(GridPolygonPosVec4, range.StartIndex_GridPolygon, range.EndIndex_GridPolygon);
    erase_range(GridPolygonColorVec4, range.StartIndex_GridPolygon, range.EndIndex_GridPolygon);

    // KeyMap内も削除
    const char* keyName = KeyMap_GetKey(&SceneMap, index);
    if (keyName) free((void*)keyName);

    // SceneMapのキー削除
    for (size_t i = index; i + 1 < SceneMap.size; i++)
        SceneMap.keys[i] = SceneMap.keys[i + 1];
    SceneMap.size--;

    // SceneRange削除
    SceneRanges.erase(SceneRanges.begin() + index);

    AddMessage(ConcatCStr("DeleteScene(): ", name));

    // カレント修正
    if (CurrentSceneIndex >= (int)SceneRanges.size())
        CurrentSceneIndex = (int)SceneRanges.size() - 1;
}

void CopyScene(const char* srcScene, const char* newScene) {
    int srcIndex = KeyMap_GetIndex(&SceneMap, srcScene);
    if (srcIndex == -1) {
        AddMessage(ConcatCStr("error : CopyScene failed (source not found): ", srcScene));
        return;
    }

    // Scene名登録
    int newSceneIndex = KeyMap_Add(&SceneMap, newScene);

    // 元シーン範囲
    const SceneRange& src = SceneRanges[srcIndex];
    SceneRange newRange = src; // 範囲情報をコピー

    // --- Vec4/Int データのコピー ---
    auto copy_range = [&](auto& vec, int start, int end) {
        if (end <= start) return;
        int count = end - start;
        for (int i = 0; i < count; i++) {
            Vec4_PushBack(&vec, Vec4_Get(&vec, start + i));
        }
        };

    auto copy_int_range = [&](auto& vec, int start, int end) {
        if (end <= start) return;
        int count = end - start;
        for (int i = 0; i < count; i++) {
            VecInt_PushBack(&vec, VecInt_Get(&vec, start + i));
        }
        };

    // コピー対象
    int oldGridBoxEnd = GetObjectIndex()->GridBoxIndex;
    int oldGridPolygonEnd = GetObjectIndex()->GridPolygonIndex;
    int oldGridEnd = GetObjectIndex()->GridLineIndex;

    // GridBox
    newRange.StartIndex_GridBox = GetObjectIndex()->GridBoxIndex;
    copy_range(GridBoxPosVec4, src.StartIndex_GridBox, src.EndIndex_GridBox);
    copy_range(GridBoxSizeVec4, src.StartIndex_GridBox, src.EndIndex_GridBox);
    copy_range(GridBoxAngleVec4, src.StartIndex_GridBox, src.EndIndex_GridBox);
    copy_range(GridBoxColorVec4, src.StartIndex_GridBox, src.EndIndex_GridBox);
    newRange.EndIndex_GridBox = GetObjectIndex()->GridBoxIndex = (int)GridBoxPosVec4.size;

    // GridPolygon
    newRange.StartIndex_GridPolygon = GetObjectIndex()->GridPolygonIndex;
    copy_range(GridPolygonPosVec4, src.StartIndex_GridPolygon, src.EndIndex_GridPolygon);
    copy_range(GridPolygonSizeVec4, src.StartIndex_GridPolygon, src.EndIndex_GridPolygon);
    copy_range(GridPolygonAngleVec4, src.StartIndex_GridPolygon, src.EndIndex_GridPolygon);
    copy_range(GridPolygonColorVec4, src.StartIndex_GridPolygon, src.EndIndex_GridPolygon);
    copy_int_range(GridPolygonSides, src.StartIndex_GridPolygon, src.EndIndex_GridPolygon);
    newRange.EndIndex_GridPolygon = GetObjectIndex()->GridPolygonIndex = (int)GridPolygonPosVec4.size;

    // Grid (シンプルコピー)
    newRange.StartIndex_Grid = GetObjectIndex()->GridLineIndex;
    copy_range(GridColorVec4, src.StartIndex_Grid, src.EndIndex_Grid);
    copy_range(GridStartVec4, src.StartIndex_Grid, src.EndIndex_Grid);
    copy_range(GridEndVec4, src.StartIndex_Grid, src.EndIndex_Grid);
    newRange.EndIndex_Grid = GetObjectIndex()->GridLineIndex = (int)GridColorVec4.size;

    // カメラ設定
    newRange.UseCameraIndex = src.UseCameraIndex;

    // シーン範囲を追加
    SceneRanges.push_back(newRange);
    SceneCount++;

    AddMessage(ConcatCStr("CopyScene() success: ", newScene));
}

void ChangeScene(const char* name) {
    int index = KeyMap_GetIndex(&SceneMap, name);
    if (index == -1) {
        AddMessage(ConcatCStr("error : ChangeScene - Scene not found: ", name));
        return;
    }

    // 現在のScene終了処理（※Deleteしない）
    AddMessage(ConcatCStr("Exiting Scene: ", GetCurrentSceneName()));

    // 新Sceneへ切替
    CurrentSceneIndex = index;
}

void SetSceneCamera(const char* sceneName, const char* cameraName) {
    int sceneIdx = KeyMap_GetIndex(&SceneMap, sceneName);
    int camIdx = KeyMap_GetIndex(GetCameraKeyMap(), cameraName);

    if (sceneIdx == -1 || camIdx == -1) {
        AddMessage("\nerror : SetSceneCamera failed (invalid scene or camera)\n");
        return;
    }

    SceneRanges[sceneIdx].UseCameraIndex = camIdx;
}

void UpdateScene() {
    if (CurrentSceneIndex < 0 || CurrentSceneIndex >= (int)SceneRanges.size())
        return;

    SceneRange& range = SceneRanges[CurrentSceneIndex];

    int useCam = range.UseCameraIndex;
    if (useCam >= 0)
    {
        Vec4 vec4Pos = Vec4_Get(&CameraPosVec4, useCam);
        Vec4 vec4Look = Vec4_Get(&CameraLookVec4, useCam);

        XMFLOAT4 CamPos = { vec4Pos.X, vec4Pos.Y, vec4Pos.Z, 0.0f };
        XMFLOAT4 CamLook = { vec4Look.X, vec4Look.Y, vec4Look.Z, 0.0f };

        GetObjectClass()->GetComponent<Camera>(useCam)->SetCameraView(CamPos, CamLook);
    }

    // --- Scene単位で更新 ---
    // 
    // Grid=============
    // 
    // 現在のシーンに対応するグリッドなどだけを更新
    for (int i = range.StartIndex_GridBox; i < range.EndIndex_GridBox; i++) {
        // Scene専用GridBox更新処理
    }
    for (int i = range.StartIndex_GridPolygon; i < range.EndIndex_GridPolygon; i++) {
        // Scene専用GridPolygon更新処理
    }
    for (int i = range.StartIndex_Grid; i < range.EndIndex_Grid; i++) {
        // Scene専用Grid更新処理
    }
}
void DrawScene() {
    if (CurrentSceneIndex < 0 || CurrentSceneIndex >= (int)SceneRanges.size())
        return;

    SceneRange& range = SceneRanges[CurrentSceneIndex];
    int useCam = (range.UseCameraIndex >= 0) ? range.UseCameraIndex : GetUseCamera();

    GetGridClass()->SetProj(GetObjectClass()->GetComponent<Camera>(useCam)->GetProjection());
    GetGridClass()->SetView(GetObjectClass()->GetComponent<Camera>(useCam)->GetView());

    // --- Scene単位で描画 ---
    for (int i = range.StartIndex_GridBox; i < range.EndIndex_GridBox; i++) {
        Vec4 vec4Pos = Vec4_Get(&GridBoxPosVec4, i);
        Vec4 vec4Size = Vec4_Get(&GridBoxSizeVec4, i);
        Vec4 vec4Angle = Vec4_Get(&GridBoxAngleVec4, i);
        Vec4 vec4Color = Vec4_Get(&GridBoxColorVec4, i);

        GetGridClass()->SetColor({ vec4Color.X, vec4Color.Y, vec4Color.Z, vec4Color.W });
        GetGridClass()->DrawBox(
            { vec4Pos.X, vec4Pos.Y, vec4Pos.Z },
            { vec4Size.X, vec4Size.Y, vec4Size.Z },
            { vec4Angle.X, vec4Angle.Y, vec4Angle.Z }
        );
    }

    for (int i = range.StartIndex_GridPolygon; i < range.EndIndex_GridPolygon; i++) {
        Vec4 vec4Color = Vec4_Get(&GridPolygonColorVec4, i);
        Vec4 vec4Pos = Vec4_Get(&GridPolygonPosVec4, i);
        Vec4 vec4Size = Vec4_Get(&GridPolygonSizeVec4, i);
        Vec4 vec4Angle = Vec4_Get(&GridPolygonAngleVec4, i);

        GetGridClass()->SetColor({ 1, 1, 1, 1 });
        GetGridClass()->DrawGridPolygon(
            VecInt_Get(&GridPolygonSides, i),
            { vec4Pos.X, vec4Pos.Y, vec4Pos.Z },
            { vec4Size.X, vec4Size.Y, vec4Size.Z },
            { vec4Angle.X, vec4Angle.Y, vec4Angle.Z }
        );
    }
}

const char* GetCurrentSceneName()
{
    if (CurrentSceneIndex < 0 || CurrentSceneIndex >= (int)SceneMap.size)
        return "None";
    return KeyMap_GetKey(&SceneMap, CurrentSceneIndex);
}
