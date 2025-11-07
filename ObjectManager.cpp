#include "ObjectManager.h"

#include "Main.h"
#include "Object.h"
#include "Grid.h"
#include "SceneManager.h"

//コンポーネントインクルード
#include "ComponentCamera.h"
#include "ComponentSpriteScreen.h"
#include "ComponentSpriteWorld.h"

//定数
#define MAX_MESSAGE 256

//グローバル宣言
static const char* Message[MAX_MESSAGE];

//Vec4宣言 __________________________
//カメラ用Vec4
static Vec4Vector CameraPosVec4;
static Vec4Vector CameraLookVec4;
//UI用Vec4
static Vec4Vector UITBLRVec4;
static Vec4Vector UIAngleVec4;
static Vec4Vector UIColorVec4;
//World2d用Vec4
static Vec4Vector World2dPosVec4;
static Vec4Vector World2dSizeVec4;
static Vec4Vector World2dAngleVec4;
//Model用Vec4
static Vec4Vector ModelPosVec4;
static Vec4Vector ModelSizeVec4;
static Vec4Vector ModelAngleVec4;
//BoxCollider用Vec4
static Vec4Vector BoxColliderPosVec4;
static Vec4Vector BoxColliderSizeVec4;
static Vec4Vector BoxColliderAngleVec4;
//Grid用Vec4
static Vec4Vector GridBoxPosVec4;       //Box___
static Vec4Vector GridBoxSizeVec4;
static Vec4Vector GridBoxAngleVec4;
static Vec4Vector GridBoxColorVec4;
static Vec4Vector GridPolygonPosVec4;   //Polygon_______
static Vec4Vector GridPolygonSizeVec4;
static Vec4Vector GridPolygonAngleVec4;
static Vec4Vector GridPolygonColorVec4;

//CharVec宣言 __________________
static CharVector TexturePath;
static CharVector ModelPath;

//IntVec宣言 __________________
static IntVector NumberOfScenes;
static IntVector ModelType;
//Grid用
static IntVector GridPolygonSides;

//BoolVector宣言 _________________
static BoolVector BillboardW2d;

// vector ___________________
// Vec4
// vector初期化
void Vec4_Init(Vec4Vector* vec) {
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}
//push_back
void Vec4_PushBack(Vec4Vector* vec, Vec4 value) {
    if (vec->size >= vec->capacity) {
        size_t new_capacity = (vec->capacity == 0) ? 4 : vec->capacity * 2;
        Vec4* new_data = (Vec4*)realloc(vec->data, new_capacity * sizeof(Vec4));
        if (!new_data) {
            AddMessage("\nerror : vector_push_back/メモリの確保に失敗\n");
            return;
        }
        vec->data = new_data;
        vec->capacity = new_capacity;
    }
    vec->data[vec->size] = value;
    vec->size++;
}
//要素の設定
void Vec4_Set(Vec4Vector* vec, size_t index, Vec4 value) {
    if (index >= vec->size) {
        AddMessage("\nerror : vector_set/インデックス範囲外\n");
        return;
    }
    vec->data[index] = value;
}
//要素を取得
Vec4 Vec4_Get(Vec4Vector* vec, size_t index) {
    if (index >= vec->size) {
        AddMessage("\nerror : vector_get/インデックス範囲外\n");
        return { -1.0f,-1.0f,-1.0f,-1.0f };
    }
    return vec->data[index];
}
//解放
void Vec4_Free(Vec4Vector* vec) {
    free(vec->data);
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}
Vec4Vector* GetVec4Vector(int type) {
    switch (type) {
    case 0: return &CameraPosVec4;
    case 1: return &CameraLookVec4;
    case 2: return &UITBLRVec4;
    case 3: return &UIAngleVec4;
    case 4: return &World2dPosVec4;
    case 5: return &World2dSizeVec4;
    case 6: return &World2dAngleVec4;
    case 7: return &ModelPosVec4;
    case 8: return &ModelSizeVec4;
    case 9: return &ModelAngleVec4;
    case 10: return &BoxColliderPosVec4;
    case 11: return &BoxColliderSizeVec4;
    case 12: return &BoxColliderAngleVec4;
    default:
        AddMessage("\nerror : get_vec4vector/不明なタイプ\n");
        return NULL;
    }
}

// Char
void VecC_Init(CharVector* vec) {
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}
void VecC_PushBack(CharVector* vec, const char* str) {
    //if (!str) return;

    if (vec->size >= vec->capacity) {
        size_t new_capacity = (vec->capacity == 0) ? 4 : vec->capacity * 2;
        char** new_data = (char**)realloc(vec->data, new_capacity * sizeof(const char*));
        if (!new_data) {
            AddMessage("\nerror : charvector_push_back/メモリの確保に失敗\n");
            return;
        }
        vec->data = new_data;
        vec->capacity = new_capacity;
    }

    // コピーを確保して保存
    size_t len = strlen(str) + 1;
    char* copy = (char*)malloc(len);
    if (!copy) {
        AddMessage("\nerror : charvector_push_back/文字列コピー失敗\n");
        return;
    }
    memcpy(copy, str, len);

    vec->data[vec->size++] = copy;
}
const char* VecC_Get(CharVector* vec, size_t index) {
    if (!vec)
    {
        AddMessage("\nerror : charvector_get/ベクターがNULL\n");
        return NULL;
    }
    if (index >= vec->size) {
        AddMessage("\nerror : charvector_get/インデックス範囲外\n");
        return NULL;
    }
    return vec->data[index];
}
void VecC_Set(CharVector* vec, size_t index, const char* str) {
    if (index >= vec->size) {
        AddMessage("\nerror : charvector_set/インデックス範囲外\n");
        return;
    }
    vec->data[index] = NULL;

    size_t len = strlen(str) + 1;
    char* copy = (char*)malloc(len);
    memcpy(copy, str, len);

    vec->data[index] = copy;
}
void VecC_Free(CharVector* vec) {
    for (size_t i = 0; i < vec->size; i++) {
        free((void*)vec->data[i]);
    }
    free(vec->data);
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}
CharVector* GetVecCVector(int type) {
    switch (type) {
    case 0: return &ModelPath;
    case 1: return &TexturePath;
    default:
        AddMessage("\nerror : get_veccvector/不明なタイプ\n");
        return NULL;
    }
}

//___________________________________
// Int
void VecInt_Init(IntVector* vec) {
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}
void VecInt_PushBack(IntVector* vec, int str) {
    //if (!str) return;

    if (vec->size >= vec->capacity) {
        size_t new_capacity = (vec->capacity == 0) ? 4 : vec->capacity * 2;
        int* new_data = (int*)realloc(vec->data, new_capacity * sizeof(int));
        if (!new_data) {
            AddMessage("\nerror : intvector_push_back/メモリの確保に失敗\n");
            return;
        }
        vec->data = new_data;
        vec->capacity = new_capacity;
    }

    vec->data[vec->size++] = str;
}
int VecInt_Get(IntVector* vec, size_t index) {
    if (index >= vec->size) {
        AddMessage("\nerror : intvector_get/インデックス範囲外\n");
        return -1;
    }
    return vec->data[index];
}
void VecInt_Set(IntVector* vec, size_t index, int str) {
    if (index >= vec->size) {
        AddMessage("\nerror : intvector_set/インデックス範囲外\n");
        return;
    }
    vec->data[index] = str;
}
void VecInt_Free(IntVector* vec) {
    free(vec->data);
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

void VecBool_Init(BoolVector* vec) {
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}
void VecBool_PushBack(BoolVector* vec, bool str) {
    if (vec->size >= vec->capacity) {
        size_t new_capacity = (vec->capacity == 0) ? 4 : vec->capacity * 2;
        bool* new_data = (bool*)realloc(vec->data, new_capacity * sizeof(bool));
        if (!new_data) {
            AddMessage("\nerror : bool_vector_push_back/メモリの確保に失敗\n");
            return;
        }
        vec->data = new_data;
        vec->capacity = new_capacity;
    }

    vec->data[vec->size++] = str;
}
int VecBool_Get(BoolVector* vec, size_t index) {
    if (index >= vec->size) {
        AddMessage("\nerror : bool_vector_get/インデックス範囲外\n");
        return NULL;
    }
    return vec->data[index];
}
void VecBool_Set(BoolVector* vec, size_t index, bool str) {
    if (index >= vec->size) {
        AddMessage("\nerror : bool_vector_set/インデックス範囲外\n");
        return;
    }
    vec->data[index] = str;
}
void VecBool_Free(BoolVector* vec) {
    free(vec->data);
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

// KeyMap宣言 __________________
KeyMap CameraMap;
KeyMap ModelMap;
KeyMap TextureMap;
KeyMap World2dMap;
KeyMap UIMap;
KeyMap BoxColliderMap;
KeyMap GridBoxMap;
KeyMap GridPolygonMap;
KeyMap SceneMap;

// KeyMap 関数
void KeyMap_Init(KeyMap* map) {
    map->keys = NULL;
    map->size = 0;
    map->capacity = 0;
}
void KeyMap_Free(KeyMap* map) {
    for (size_t i = 0; i < map->size; i++) {
        free(map->keys[i]);
    }
    free(map->keys);
    map->keys = NULL;
    map->size = 0;
    map->capacity = 0;
}
int KeyMap_EnsureCapacity(KeyMap* map) {
    if (map->size >= map->capacity) {
        size_t new_capacity = (map->capacity == 0) ? 4 : map->capacity * 2;
        char** new_keys = (char**)realloc(map->keys, new_capacity * sizeof(char*));
        if (!new_keys) {
            AddMessage("\nerror : keymap_ensure_capacity/メモリの確保に失敗\n");
            return 0; // メモリ確保失敗
        }
        map->keys = new_keys;
        map->capacity = new_capacity;
    }
    return 1; // 成功
}
int KeyMap_Add(KeyMap* map, const char* key) {
    for (size_t i = 0; i < map->size; i++) {
        if (strcmp(map->keys[i], key) == 0) {
            printf("error: key '%s' already exists!\n", key);
            return -1; // 既存
        }
    }
    if (!KeyMap_EnsureCapacity(map)) return -1;

    map->keys[map->size] = _strdup(key);
    return (int)map->size++; // 登録したインデックスを返す
}
int KeyMap_GetIndex(KeyMap* map, const char* key) {
    for (size_t i = 0; i < map->size; i++) {
        if (strcmp(map->keys[i], key) == 0) {
            return (int)i; // 見つかった
        }
    }
    return -1; // 見つからなかった
}
const char* KeyMap_GetKey(KeyMap* map, int index) {
    if (index < 0 || (size_t)index >= map->size) {
        AddMessage("\nerror : keymap_getkey/インデックス範囲外\n");
        return NULL;
    }
    return map->keys[index];
}
KeyMap* GetKeyMapByType(int type) {
    switch (type) {
    case 0: return &CameraMap;
    case 1: return &ModelMap;
    case 2: return &TextureMap;
    case 3: return &World2dMap;
    case 4: return &UIMap;
    case 5: return &BoxColliderMap;
    default:
        AddMessage("\nerror : get_keymap_by_type/不明なタイプ\n");
        return NULL;
    }
}

void DebugDumpModelType() {
    char buf[256];
    for (size_t i = 0; i < ModelType.size; i++) {
        sprintf(buf, "ModelType[%zu] = %d\n", i, ModelType.data[i]);
        AddMessage(buf);
    }
}
void ConcatCStrFree(const char* str)
{
    free((void*)str);
}
const char* ConcatCStr(const char* str1, const char* str2)
{
    if (!str1 && !str2) return nullptr;
    if (!str1) return str2;
    if (!str2) return str1;
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    char* result = (char*)malloc(len1 + len2 + 1); // +1 for null terminator
    if (!result) {
        AddMessage("\nerror : concat_cstr/メモリの確保に失敗\n");
        return nullptr;
    }
    strcpy_s(result, len1 + 1, str1); // Copy first string
    strcat_s(result, len1 + len2 + 1, str2); // Concatenate second string
    return result; // 呼び出し側でfreeすること
}
void AddMessage(const char* sent)
{
    const char* MM = ConcatCStr("\n", sent);

    for (int i = 0; i < MAX_MESSAGE; i++)
    {
        if (Message[i] == "\0")
        {
            Message[i] = MM;
            break;
        }
    }
}

  //////////////////////
 // オブジェクト管理 // 
//////////////////////

Object* object;
Grid* grid;

//インデックス管理 ____________________________
//オブジェクトの数___________________
static int UseCamera;
static int CameraIndex = 0;
static int CameraOldIdx = 0;

static int UIIndex = 0;
static int UIOldIndex = 0;

static int world2dIndex = 0;
static int world2dOldIndex = 0;

static int ModelIndex = 0;
static int ModelOldIndex = 0;

static int BoxColliderIndex = 0;
static int BoxColliderOldIndex = 0;

static int GridBoxIndex = 0;
static int GridBoxOldIndex = 0;

static int GridPolygonIndex = 0;
static int GridPolygonOldIndex = 0;

int GetCameraIndex() { return CameraIndex; }
int GetUseCamera() { return UseCamera; }
int GetUIIndex() { return UIIndex; }
int GetWorld2dIndex() { return world2dIndex; }
int GetModelIndex() { return ModelIndex; }
int GetBoxColliderIndex() { return BoxColliderIndex; }

//カメラ=============
void AddCamera(const char* CameraName)
{

    //Vec4_Init(&CameraPosVec4);
    Vec4_PushBack(&CameraPosVec4, Vec4{ 0.0f,0.0f,0.0f,0.0f });

    //Vec4_Init(&CameraLookVec4);
    Vec4_PushBack(&CameraLookVec4, Vec4{ 0.0f,0.0f,0.0f,0.0f });

    KeyMap_Add(&CameraMap, CameraName);

    CameraIndex++;
}
void SetCameraPos(const char* CameraName, float posX, float posY, float posZ)
{
    Vec4_Set(&CameraPosVec4, KeyMap_GetIndex(&CameraMap, CameraName),
        Vec4{ posX, posY, posZ, 0.0f });
}
void SetCameraLook(const char* CameraName, float lookX, float lookY, float lookZ)
{
    Vec4_Set(&CameraLookVec4, KeyMap_GetIndex(&CameraMap, CameraName),
        Vec4{ lookX, lookY, lookZ, 0.0f });
}
void UseCameraSet(const char* CameraName)
{
    UseCamera = KeyMap_GetIndex(&CameraMap, CameraName);
}
void CreateCamera()
{
    while (CameraOldIdx < CameraIndex)
    {
        object->AddComponent<Camera>();

        CameraOldIdx++;
    }
}
void SettingCamera()
{
    for (int i = 0; i < CameraIndex; i++)
    {
        object->GetComponent<Camera>(i)->
            SetCameraProjection(70.0f, 800, 600);

        Vec4 vec4Pos = Vec4_Get(&CameraPosVec4, i);
        Vec4 vec4Look = Vec4_Get(&CameraLookVec4, i);

        XMFLOAT4 CamPos = { vec4Pos.X, vec4Pos.Y, vec4Pos.Z, 0.0f };
        XMFLOAT4 CamLook = { vec4Look.X, vec4Look.Y, vec4Look.Z, 0.0f };

        object->GetComponent<Camera>(i)->
            SetCameraView(CamPos, CamLook);
    }
}

//SpriteWorld =====================



//グリッド=========================

//ボックス型グリッド
void AddGridBox(const char* Name)
{
    Vec4_PushBack(&GridBoxPosVec4, Vec4{ 0.0f,0.0f,0.0f,0.0f });
    Vec4_PushBack(&GridBoxSizeVec4, Vec4{ 1.0f, 1.0f, 1.0f, 1.0f });
    Vec4_PushBack(&GridBoxAngleVec4, Vec4{ 0.0f, 0.0f, 0.0f, 0.0f });
    Vec4_PushBack(&GridBoxColorVec4, Vec4{ 1.0f, 1.0f, 1.0f, 1.0f });

    KeyMap_Add(&GridBoxMap, Name);

    GridBoxIndex++;
}
void SetGridBoxPos(const char* Name, float posX, float posY, float posZ)
{
    Vec4_Set(&GridBoxPosVec4, KeyMap_GetIndex(&GridBoxMap, Name),
        Vec4{ posX, posY, posZ, 0.0f });
}
void SetGridBoxSize(const char* Name, float sizeX, float sizeY, float sizeZ)
{
    Vec4_Set(&GridBoxSizeVec4, KeyMap_GetIndex(&GridBoxMap, Name),
        Vec4{ sizeX, sizeY, sizeZ, 0.0f });
}
void SetGridBoxAngle(const char* Name, float angleX, float angleY, float angleZ)
{
    Vec4_Set(&GridBoxAngleVec4, KeyMap_GetIndex(&GridBoxMap, Name),
        Vec4{ angleX, angleY, angleZ, 0.0f });
}
void SetGridBoxColor(const char* Name, float R, float G, float B, float A)
{
    Vec4_Set(&GridBoxColorVec4, KeyMap_GetIndex(&GridBoxMap, Name),
        Vec4{ R, G, B, A });
}
void CreateGridBox()
{
    while (GridBoxOldIndex < GridBoxIndex)
    {
        GridBoxOldIndex++;
    }
}
void SettingGridBox()
{
    for (int i = 0; i < GridBoxIndex; i++)
    {
        Vec4 vec4Pos = Vec4_Get(&GridBoxPosVec4, i);
        Vec4 vec4Size = Vec4_Get(&GridBoxSizeVec4, i);
        Vec4 vec4Angle = Vec4_Get(&GridBoxAngleVec4, i);
        Vec4 vec4Color = Vec4_Get(&GridBoxColorVec4, i);

        XMFLOAT4 GridBoxPos = { vec4Pos.X, vec4Pos.Y, vec4Pos.Z, 0.0f };
        XMFLOAT4 GridBoxSize = { vec4Size.X, vec4Size.Y, vec4Size.Z, 0.0f };
        XMFLOAT4 GridBoxAngle = { vec4Angle.X, vec4Angle.Y, vec4Angle.Z, 0.0f };
        XMFLOAT4 GridBoxColor = { vec4Color.X, vec4Color.Y, vec4Color.Z, vec4Color.W };

        grid->SetColor({ vec4Color.X, vec4Color.Y, vec4Color.Z, vec4Color.W });
        grid->DrawBox(
            { vec4Pos.X, vec4Pos.Y, vec4Pos.Z },
            { vec4Size.X, vec4Size.Y, vec4Size.Z },
            { vec4Angle.X, vec4Angle.Y, vec4Angle.Z }
            );
    }
}
void AddGridPolygon(const char* Name)
{
    Vec4_PushBack(&GridPolygonPosVec4, { 0.0f,0.0f, 0.0f, 0.0f });
    Vec4_PushBack(&GridPolygonSizeVec4, { 1.0f, 1.0f, 1.0f, 1.0f });
    Vec4_PushBack(&GridPolygonAngleVec4, { 0.0f,0.0f, 0.0f, 0.0f });
    Vec4_PushBack(&GridPolygonColorVec4, { 0.0f,0.0f, 0.0f, 0.0f });

    VecInt_PushBack(&GridPolygonSides, 4);

    KeyMap_Add(&GridPolygonMap, Name);

    GridPolygonIndex++;
}
void SetGridPolygonPos(const char* Name, float posX, float posY, float posZ)
{
    Vec4_Set(&GridPolygonPosVec4, KeyMap_GetIndex(&GridPolygonMap, Name),
        { posX, posY, posZ });
}
void SetGridPolygonSize(const char* Name, float sizeX, float sizeY, float sizeZ)
{
    Vec4_Set(&GridPolygonSizeVec4, KeyMap_GetIndex(&GridPolygonMap, Name),
        { sizeX, sizeY, sizeX });
}
void SetGridPolygonAngle(const char* Name, float angleX, float angleY, float angleZ)
{
    Vec4_Set(&GridPolygonAngleVec4, KeyMap_GetIndex(&GridPolygonMap, Name),
        { angleX, angleY, angleX });
}
void SetGridPolygonColor(const char* Name, float R, float G, float B, float A)
{
    Vec4_Set(&GridPolygonColorVec4, KeyMap_GetIndex(&GridPolygonMap, Name),
        Vec4{ R,G,B,A });
}
void SetGridPolygonSides(const char* Name, int Sides)
{
    VecInt_Set(&GridPolygonSides, KeyMap_GetIndex(&GridPolygonMap, Name), Sides);
}
void CreateGridPolygon()
{
    while (GridPolygonOldIndex < GridPolygonIndex)
    {
        GridPolygonOldIndex++;
    }
}
void SettingGridPolygon()
{
    for (int i = 0; i < GridPolygonIndex; i++)
    {
        Vec4 vec4Color = Vec4_Get(&GridPolygonColorVec4, i);

        Vec4 vec4Pos = Vec4_Get(&GridPolygonPosVec4, i);
        Vec4 vec4Size = Vec4_Get(&GridPolygonSizeVec4, i);
        Vec4 vec4Angle = Vec4_Get(&GridPolygonAngleVec4, i);

        XMFLOAT4 Color;
        Color.x = vec4Color.X;
        Color.y = vec4Color.Y;
        Color.z = vec4Color.Z;
        Color.w = vec4Color.W;

        grid->SetColor({Color});
        grid->DrawGridPolygon(
            VecInt_Get(&GridPolygonSides, i),
            { vec4Pos.X, vec4Pos.Y, vec4Pos.Z },
            { vec4Size.X, vec4Size.Y, vec4Size.Z },
            { vec4Angle.X, vec4Angle.Y,vec4Angle.Z }
            );
    }
}


void DrawGridBase()
{
    // グリッド表示 //=====
    // 補正グリッド
    grid->SetProj(object->GetComponent<Camera>(UseCamera)->GetProjection());
    grid->SetView(object->GetComponent<Camera>(UseCamera)->GetView());

    grid->SetColor({ 0.0f,0.0f,0.0f,0.0f });

    for (int i = 0; i < 10; i++)
    {
        if (!(i == 5))
        {
            grid->SetPos({ i - 5.0f, 0.0f, -5.0f }, { i - 5.0f, 0.0f, 5.0f });
            grid->Draw();

            grid->SetPos({ -5.0f, 0.0f, i - 5.0f }, { 5.0f, 0.0f, i - 5.0f });
            grid->Draw();
        }
    }
    // Xグリッド
    grid->SetColor({ 1.0f,0.0f,0.0f,1.0f });
    grid->SetPos({ -5.0f, 0.0f,0.0f }, { 5.0f, 0.0f, 0.0f });
    grid->Draw();
    // Yグリッド
    grid->SetColor({ 0.0f,1.0f,0.0f,1.0f });
    grid->SetPos({ 0.0f, -5.0f, 0.0f }, { 0.0f, 5.0f, 0.0f });
    grid->Draw();
    // Zグリッド
    grid->SetColor({ 0.0f,0.0f,1.0f,1.0f });
    grid->SetPos({ 0.0f, 0.0f, -5.0f }, { 0.0f, 0.0f, 5.0f });
    grid->Draw();
}
void DrawGridBox(XMFLOAT3 Pos, XMFLOAT3 Size, XMFLOAT3 Angle)
{
    grid->SetColor({ 1.0f, 1.0f, 0.0f, 1.0f });
    grid->DrawBox(Pos, Size, Angle);
}
  ////////////////
 // シーン管理 // 
////////////////

//シーン追加項目
//
// KeyMapを使って一次元増やす事で実現
// Scene X Type X Component
//

static int CurrentSceneIndex = -1;
static int SceneCount = 0;
static int SceneEndFlag = 0;

// 各Sceneごとの範囲を保持
typedef struct {
    int StartIndex_GridBox;
    int EndIndex_GridBox;
    int StartIndex_GridPolygon;
    int EndIndex_GridPolygon;
} SceneRange;

static std::vector<SceneRange> SceneRanges;

void AddScene(const char* name)
{
    KeyMap_Add(&SceneMap, name);

    SceneRange range{};
    range.StartIndex_GridBox = GridBoxIndex;
    range.EndIndex_GridBox = GridBoxIndex;
    range.StartIndex_GridPolygon = GridPolygonIndex;
    range.EndIndex_GridPolygon = GridPolygonIndex;
    SceneRanges.push_back(range);

    SceneCount++;
    SceneEndFlag = 0;
}

void SceneEndPoint()
{
    if (SceneCount <= 0) return;

    SceneRanges.back().EndIndex_GridBox = GridBoxIndex;
    SceneRanges.back().EndIndex_GridPolygon = GridPolygonIndex;
    SceneEndFlag = 1;
}

void ChangeScene(const char* name)
{
    int index = KeyMap_GetIndex(&SceneMap, name);
    if (index == -1) {
        AddMessage("\nerror : Scene not found\n");
        return;
    }
    CurrentSceneIndex = index;
}

void UpdateScene()
{
    if (CurrentSceneIndex < 0 || CurrentSceneIndex >= (int)SceneRanges.size())
        return;

    SceneRange& range = SceneRanges[CurrentSceneIndex];

    // 現在のシーンに対応するグリッドなどだけを更新
    for (int i = range.StartIndex_GridBox; i < range.EndIndex_GridBox; i++) {
        // Scene専用GridBox更新処理がある場合はここに
    }

    for (int i = range.StartIndex_GridPolygon; i < range.EndIndex_GridPolygon; i++) {
        // Scene専用GridPolygon更新処理がある場合はここに
    }
}

void DrawScene()
{
    if (CurrentSceneIndex < 0 || CurrentSceneIndex >= (int)SceneRanges.size())
        return;

    SceneRange& range = SceneRanges[CurrentSceneIndex];

    // --- Scene単位で描画 ---
    for (int i = range.StartIndex_GridBox; i < range.EndIndex_GridBox; i++) {
        Vec4 vec4Pos = Vec4_Get(&GridBoxPosVec4, i);
        Vec4 vec4Size = Vec4_Get(&GridBoxSizeVec4, i);
        Vec4 vec4Angle = Vec4_Get(&GridBoxAngleVec4, i);
        Vec4 vec4Color = Vec4_Get(&GridBoxColorVec4, i);

        grid->SetColor({ vec4Color.X, vec4Color.Y, vec4Color.Z, vec4Color.W });
        grid->DrawBox(
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

        grid->SetColor({ vec4Color.X, vec4Color.Y, vec4Color.Z, vec4Color.W });
        grid->DrawGridPolygon(
            VecInt_Get(&GridPolygonSides, i),
            { vec4Pos.X, vec4Pos.Y, vec4Pos.Z },
            { vec4Size.X, vec4Size.Y, vec4Size.Z },
            { vec4Angle.X, vec4Angle.Y, vec4Angle.Z }
        );
    }
}

void InitDo()
{
	//インデックス初期化
    UseCamera = 0;
    CameraIndex = 0;
    CameraOldIdx = 0;
    UIIndex = 0;
    UIOldIndex = 0;
    world2dIndex = 0;
    world2dOldIndex = 0;
    ModelIndex = 0;
    ModelOldIndex = 0;
    BoxColliderIndex = 0;
	BoxColliderOldIndex = 0;

	//Vec4初期化
	Vec4_Init(&CameraPosVec4);
    Vec4_Init(&CameraLookVec4);
    Vec4_Init(&UITBLRVec4);
    Vec4_Init(&UIAngleVec4);
    Vec4_Init(&World2dPosVec4);
    Vec4_Init(&World2dSizeVec4);
    Vec4_Init(&World2dAngleVec4);
    Vec4_Init(&ModelPosVec4);
    Vec4_Init(&ModelSizeVec4);
    Vec4_Init(&ModelAngleVec4);
    Vec4_Init(&BoxColliderPosVec4);
    Vec4_Init(&BoxColliderSizeVec4);
    Vec4_Init(&BoxColliderAngleVec4);
	//CharVec初期化
    VecC_Init(&TexturePath);
    VecC_Init(&ModelPath);
	//IntVec初期化
    VecInt_Init(&NumberOfScenes);
    VecInt_Init(&ModelType);
	//BoolVec初期化
    VecBool_Init(&BillboardW2d);
	//KeyMap初期化
    KeyMap_Init(&CameraMap);
    KeyMap_Init(&ModelMap);
    KeyMap_Init(&TextureMap);
    KeyMap_Init(&World2dMap);
    KeyMap_Init(&UIMap);
	KeyMap_Init(&BoxColliderMap);

	//クラス取得
	grid = new Grid();
	grid->Init();
	object = new Object();
	object->Init();

	AddCamera("MainCamera");
	UseCameraSet("MainCamera");
	SetCameraPos("MainCamera", 0.0f, 5.0f, -10.0f);
	SetCameraLook("MainCamera", 0.0f, 0.0f, 0.0f);

    AddScene("Scene1");

    AddGridBox("Box01");
    AddGridBox("Box02");
    SetGridBoxPos("Box02", 1, 0, 0);
    SetGridBoxColor("Box02", 1, 0, 0, 1);
    AddGridBox("Box03");
    SetGridBoxPos("Box03", 3, 0, 0);
    SetGridBoxColor("Box03", 1, 1, 0, 1);

    SceneEndPoint(); // Scene1終了

    AddScene("Scene2");

    AddGridPolygon("Polygon01");
    SetGridPolygonPos("Polygon01", 5, 0, 0);
    SetGridPolygonColor("Polygon01", 0, 1, 1, 1);
    SetGridPolygonSides("Polygon01", 6);

    SceneEndPoint(); // Scene2終了

    ChangeScene("Scene1");

}
void UpdateDo()
{
	static float camPosX = -5.0f;

	camPosX += 0.1f;

	SetCameraPos("MainCamera", camPosX, 5.0f, -5.0f);

    //カメラ_________
    CreateCamera();
    SettingCamera();

    //グリッド_______
    //CreateGridBox();
    //SettingGridBox();
    //CreateGridPolygon();
    //SettingGridPolygon();

    //grid->DrawGridPolygonGrid(10, 10, 1.2f, 6, 0.5f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});

    object->Update();

    UpdateScene();

	//MessageBoxA(NULL, "テストメッセージ", "タイトル", MB_OK);
}
void DrawDo()
{
    DrawGridBase();
    
    //DrawGridBox({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f });
    //DrawGridBox({ 2.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f });

    object->Draw();
    //MessageBoxA(NULL, "テストメッセージ", "タイトル", MB_OK);

    DrawScene();
}
void ReleaseDo()
{
	//インデックス初期化
	UseCamera = -1;
	CameraIndex = 0;
	CameraOldIdx = 0;
	UIIndex = 0;
	UIOldIndex = 0;
	world2dIndex = 0;
	world2dOldIndex = 0;
	ModelIndex = 0;
	ModelOldIndex = 0;
	BoxColliderIndex = 0;
	BoxColliderOldIndex = 0;

    //Vec4解放
    Vec4_Free(&CameraPosVec4);
    Vec4_Free(&CameraLookVec4);
    Vec4_Free(&UITBLRVec4);
    Vec4_Free(&UIAngleVec4);
    Vec4_Free(&World2dPosVec4);
    Vec4_Free(&World2dSizeVec4);
    Vec4_Free(&World2dAngleVec4);
    Vec4_Free(&ModelPosVec4);
    Vec4_Free(&ModelSizeVec4);
    Vec4_Free(&ModelAngleVec4);
    Vec4_Free(&BoxColliderPosVec4);
    Vec4_Free(&BoxColliderSizeVec4);
    Vec4_Free(&BoxColliderAngleVec4);
    //CharVec解放
    VecC_Free(&TexturePath);
    VecC_Free(&ModelPath);
    //IntVec解放
    VecInt_Free(&NumberOfScenes);
    VecInt_Free(&ModelType);
    //BoolVec解放
    VecBool_Free(&BillboardW2d);
    //KeyMap解放
    KeyMap_Free(&CameraMap);
    KeyMap_Free(&ModelMap);
    KeyMap_Free(&TextureMap);
    KeyMap_Free(&World2dMap);
    KeyMap_Free(&UIMap);
    KeyMap_Free(&BoxColliderMap);
}