#include "Manager.h"
#include "ComponentCamera.h"


static Grid* grid = nullptr;
static Object* object = nullptr;

static ObjectIndex ObjectIdx;

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

// KeyMap宣言 __________________
KeyMap CameraMap;
KeyMap ModelMap;
KeyMap TextureMap;
KeyMap World2dMap;
KeyMap UIMap;
KeyMap BoxColliderMap;
KeyMap GridBoxMap;
KeyMap GridPolygonMap;

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

void AddCamera(const char* name) {
    Vec4_PushBack(&CameraPosVec4, { 0,0,0,0 });
    Vec4_PushBack(&CameraLookVec4, { 0,0,1,0 });
    KeyMap_Add(&CameraMap, name);
    CameraIndex++;
}
void SetCameraPos(const char* name, float x, float y, float z) {
    Vec4_Set(&CameraPosVec4, KeyMap_GetIndex(&CameraMap, name), { x,y,z,0 });
}
void SetCameraLook(const char* name, float x, float y, float z) {
    Vec4_Set(&CameraLookVec4, KeyMap_GetIndex(&CameraMap, name), { x,y,z,0 });
}
void CreateCamera()
{
    while (CameraOldIdx < CameraIndex)
    {
        object->AddComponent<Camera>();

        CameraOldIdx++;
    }
}
void UseCameraSet(const char* name) {
    UseCamera = KeyMap_GetIndex(&CameraMap, name);
}
void SettingCameraOnce() {
    for (int i = 0; i < CameraIndex; i++)
        AddMessage("Camera initialized");
}

//============================
// Grid
//============================
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

//多角形グリッド
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

//============================
// 基本ライフサイクル
//============================
void InitDo() {
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

    grid = new Grid();
    grid->Init();
    object = new Object();
    object->Init();
}
void UpdateDo() {
    //カメラ_________
    CreateCamera();

    object->Update();
    UpdateScene();
}
void DrawDo() {
    DrawGridBase();
    object->Draw();
    DrawScene();
}
void ReleaseDo() {
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

//取得用関数 __________________________

ObjectDataPool* GetObjectDataPool() {
    return &g_ObjectPool;
}
void OutObjectIndex(ObjectIndex* out) {
	out->CameraIndex            = ObjectIdx.CameraIndex;
	out->SpriteWorldIndex       = ObjectIdx.SpriteWorldIndex;
	out->SpriteScreenIndex      = ObjectIdx.SpriteScreenIndex;
	out->ModelIndex             = ObjectIdx.ModelIndex;
	out->BoxColliderIndex       = ObjectIdx.BoxColliderIndex;
	out->SphereColliderIndex    = ObjectIdx.SphereColliderIndex;
	out->CapsuleColliderIndex   = ObjectIdx.CapsuleColliderIndex;
	out->GridLineIndex          = ObjectIdx.GridLineIndex;
	out->GridBoxIndex           = ObjectIdx.GridBoxIndex;
	out->GridPolygonIndex       = ObjectIdx.GridPolygonIndex;
	out->GridSphereIndex        = ObjectIdx.GridSphereIndex;
	out->GridCapsuleIndex       = ObjectIdx.GridCapsuleIndex;
	out->EffectIndex            = ObjectIdx.EffectIndex;
}
ObjectIndex* GetObjectIndex() {
    return &ObjectIdx;
}
int GetUseCamera() {
    return UseCamera;
}
Object* GetObjectClass() {
    return object;
}
Grid* GetGridClass() {
    return grid;
}
KeyMap* GetCameraKeyMap() {
    return &CameraMap;
}
