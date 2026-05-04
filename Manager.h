//Manager関連を統括するヘッダーファイル
//Manager.h
// |  Manager関連の定義
// |  SceneManager.cpp
// |  ObjectManager.cpp
// |  UtilManager.cpp
// |  AssetManager.cpp
// |  AudioManager.cpp
// |  InputManager.cpp
// |  EffectManager.cpp
// |  ShaderManager.cpp
// __________________________________________

#pragma once

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

// Include________________________
#include "Grid.h"
#include "Object.h"
#include "Component.h"

#include <string>
#include <vector>

#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXMathMatrix.inl>
#include <wrl.h>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#pragma comment (lib, "d3dcompiler.lib")

Object* GetObjectClass();

// 構造体定義_____________________

// Vec4構造体
typedef struct { float X, Y, Z, W; } Vec4;
// Char2構造体
typedef struct { const char* First; const char* End; } Char2;
typedef struct { int One; int Second; } Int2;
// ベクター型構造体
typedef struct { Vec4*  data; size_t size; size_t capacity; } Vec4Vector;
typedef struct { Char2* data; size_t size; size_t capacity; } Char2Vector;
typedef struct { Int2*  data; size_t size; size_t capacity; } Int2Vector;
typedef struct { char** data; size_t size; size_t capacity; } CharVector;
typedef struct { int*   data; size_t size; size_t capacity; } IntVector;
typedef struct { float* data; size_t size; size_t capacity; } FloatVector;
typedef struct { bool*  data; size_t size; size_t capacity; } BoolVector;
typedef struct { char** keys; size_t size; size_t capacity; } KeyMap;
// ObjectIndex構造体
typedef struct {
	int CameraIndex;        //Camera_________
    int SpriteWorldIndex;	//Sprite_________
    int SpriteScreenIndex;
    int SpriteBoxIndex;
    int SpriteCylinderIndex;
	int ModelIndex;			//Model__________
	int CollisionIndex; 	//Collider_______
	int GridIndex;      //Grid___________
	int EffectIndex;		//Effect_________
} ObjectIndex;
enum GridType
{
    Grid_Line,
    Grid_Box,
    Grid_Polygon,
    Grid_GridPolygon,
};
enum LightType
{
    PointLight,
    SpotLight,
    DirectionalLight,
};
enum SoundEffect
{
    Delay,
    Reverb,
    Compressor,
    Limiter,
    Gate,
};
enum Input
{
    Key_0,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,
    Key_A,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,
    Key_SPACE,
    Key_TAB,
    Key_CTRL,
    Key_SHIFT,
    Key_ENTER,
    Key_BACKSPACE,
    Key_ALT,
    Key_ESC,
    Key_PGUP,
    Key_PGDN,
    Key_HOME,
    Key_END,
    Key_SHIFT_LEFT,
    Key_SHIFT_RIGHT,
    Key_CTRL_LEFT,
    Key_CTRL_RIGHT,
    Key_F1,
    Key_F2,
    Key_F3,
    Key_F4,
    Key_F5,
    Key_F6,
    Key_F7,
    Key_F8,
    Key_F9,
    Key_F10,
    Key_F11,
    Key_F12,
    Mouse_LEFT,
    Mouse_RIGHT,
    Mouse_CENTER,
    Mouse_M1,
    Mosue_M2,
    Mouse_M3,
    Mouse_M4,
    Mouse_M5,
    Pad_A,
    Pad_B,
    Pad_X,
    Pad_Y,
    Pad_L1,
    Pad_L2,
    Pad_L3,
    Pad_R1,
    Pad_R2,
    Pad_R3,
    Pad_LS_LEFT,
    Pad_LS_RIGHT,
    Pad_LS_UP,
    Pad_LS_DOWN,
    Pad_RS_LEFT,
    Pad_RS_RIGHT,
    Pad_RS_UP,
    Pad_RS_DOWN,
    Pad_D_LEFT,
    Pad_D_RIGHT,
    Pad_D_UP,
    Pad_D_DOWN,
};
//モデル用マトリクスバッファ
struct MatrixBuffer
{
    XMMATRIX mvp;
    XMFLOAT4 diffuseColor;
    XMFLOAT4 useTexture;
};
//モデル用頂点情報
struct ModelVertex
{
    XMFLOAT3 pos;
    XMFLOAT3 normal;
    XMFLOAT2 uv;
};
//モデル用サブメッシュ情報
struct ModelSubmeshInfo {
    std::vector<ModelVertex> verts;
    std::vector<unsigned int> idx;

    int materialIndex = -1;
    XMFLOAT4 materialDiffuse = { 1,1,1,1 };

    // マテリアルに記載されていた diffuse テクスチャ名（psd/tga/png など）
    std::string diffuseTexName;

    // ★ 実際に読み込まれた DirectX テクスチャを保持する SRV
    ID3D11ShaderResourceView* textureSRV = nullptr;
};
struct PackageEntry {
    std::string name;         // key in package (relative path like "Texture/test.png" or "Texture/test.psd")
    uint64_t offset = 0;
    uint64_t size = 0;
    std::vector<uint8_t> data; // runtime に取り出すまで空でも可
};
struct Package {
    std::string ext; // 小文字拡張子 (no dot)
    KeyMap keymap;
    std::vector<PackageEntry> entries;
    std::string pkgPath;      // path to .pkg file
    std::ifstream pkgStream;  // lazy-opened for reading entry data
};
struct MotionRange {
    std::string name;
    double startTime;
    double endTime;
};

struct MotionChannel_Bone {
    std::string boneName;
    std::vector<double> times;
    std::vector<aiVectorKey> posKeys;
    std::vector<aiQuatKey> rotKeys;
    std::vector<aiVectorKey> scaleKeys;
};
struct MotionPackage {
    std::string name;

    double duration = 0;
    double ticksPerSecond = 30;

    std::vector<MotionRange> motions;   // "Take001" + range motion

    std::vector<MotionChannel_Bone> boneChannels; // FBX/OBJ 共通チャンネル化

    // OBJ にモーフアニメ等があれば追加
    // std::vector<MotionChannel_Vertex> vertexChannels;
};
struct WavData {
    std::vector<BYTE> buffer;
    WAVEFORMATEX format = {};
};
//API&インデックス共通用
struct ComponentType
{
    const int CT_Camera         = GetObjectClass()->GetComponentType<class Camera>();
    const int CT_Grid           = GetObjectClass()->GetComponentType<class Grid>();
    const int CT_SpriteScreen   = GetObjectClass()->GetComponentType<class SpriteScreen>();
    const int CT_SpriteWorld    = GetObjectClass()->GetComponentType<class SpriteWorld>();
    const int CT_SpriteBox      = GetObjectClass()->GetComponentType<class SpriteBox>();
    const int CT_SpriteCylinder = GetObjectClass()->GetComponentType<class SpriteCylinder>();
    const int CT_Sound          = GetObjectClass()->GetComponentType<class Sound>();
    const int CT_Model          = GetObjectClass()->GetComponentType<class Model>();
    const int CT_Collision      = GetObjectClass()->GetComponentType<class Collision>();
};
//-----------------------------------------
// Vec4管理用データプール構造体
struct ObjectDataPool {
    // Camera
	Vec4Vector CameraInfo;
    Vec4Vector CameraPos;
    Vec4Vector CameraLook;
    // UI
    Vec4Vector UITBLR;
    Vec4Vector UIAngle;
    Vec4Vector UIColor;
    // SpriteWorld
    Vec4Vector SpriteWorldInfo;
    Vec4Vector SpriteWorldPos;
    Vec4Vector SpriteWorldSize;
    Vec4Vector SpriteWorldAngle;
    Vec4Vector SpriteWorldColor;
    // SpriteScreen
	Vec4Vector SpriteScreenInfo;
    Vec4Vector SpriteScreenPos;
    Vec4Vector SpriteScreenSize;
    Vec4Vector SpriteScreenColor;
    IntVector  SpriteScreenAngle;
    //SpriteBox
	Vec4Vector SpriteBoxInfo;
    Vec4Vector SpriteBoxPos;
    Vec4Vector SpriteBoxSize;
    Vec4Vector SpriteBoxAngle;
    Vec4Vector SpriteBoxColor;
    //SpriteCylinder
	Vec4Vector SpriteCylinderInfo;
    Vec4Vector SpriteCylinderPos;
    Vec4Vector SpriteCylinderSize;
    Vec4Vector SpriteCylinderAngle;
    Vec4Vector SpriteCylinderColor;
    IntVector  SpriteCylinderSegment;
    // Model
	Vec4Vector ModelInfo;
    Vec4Vector ModelPos;
    Vec4Vector ModelSize;
    Vec4Vector ModelAngle;
    BoolVector ModelUseTexture;
    // Collision
	Vec4Vector CollisionInfo;
    Vec4Vector CollisionPos;
    Vec4Vector CollisionSize;
    Vec4Vector CollisionAngle;
    IntVector  CollisionType;
    BoolVector CollisionHit;
    // Grid(Box / Polygon)
    Vec4Vector GridInfo;
    Vec4Vector GridPos;
    Vec4Vector GridSize;
    Vec4Vector GridAngle;
    Vec4Vector GridColor;
    IntVector  GridSides;
    IntVector  GridTypeIndex;
    // Int / Bool / Char Vec
    CharVector TexturePath;
    CharVector ModelPath;
    IntVector NumberOfScenes;
    IntVector ModelType;
    BoolVector BillboardW2d;
    // KeyMaps
    KeyMap CameraMap;
    KeyMap ModelMap;
    KeyMap ModelFileMap;
    KeyMap ModelTextureMap;
    KeyMap TextureMap;
    KeyMap SpriteWorldMap;
    KeyMap SpriteScreenMap;
    KeyMap SpriteBoxMap;
    KeyMap SpriteCylinderMap;
    KeyMap UIMap;
    KeyMap CollisionMap;
    KeyMap CollisionTagMap;
    KeyMap CollisionParentMap;
    KeyMap GridMap;
    KeyMap SpriteWorldTexturePathMap;
    KeyMap SpriteScreenTexturePathMap;
    KeyMap SpriteBoxTopTexturePathMap;
    KeyMap SpriteBoxBottomTexturePathMap;
    KeyMap SpriteBoxFrontTexturePathMap;
    KeyMap SpriteBoxRearTexturePathMap;
    KeyMap SpriteBoxLeftTexturePathMap;
    KeyMap SpriteBoxRightTexturePathMap;
    KeyMap SpriteCylinderTopTexturePathMap;
    KeyMap SpriteCylinderBottomTexturePathMap;
    KeyMap SpriteCylinderSideTexturePathMap;

    KeyMap g_StructKeys;
    Vec4Vector g_SendValues;
    Vec4Vector g_RecvValues;
};
ObjectDataPool* GetObjectDataPool();


  ///////////////////
 // ObjectManager //
///////////////////
//↓API用関数 //////////////////////
//
//|| 管理   ||_______________________
void InitDo();
void UpdateDo();
void DrawDo();
void ReleaseDo();
///////////////////////////////////

void OutObjectIndex(ObjectIndex* out);
ObjectIndex* GetObjectIndex();
Grid* GetGridClass();

KeyMap* GetCameraKeyMap();


  //////////////////
 // SceneManager //
//////////////////
void AddScene(const char* name);
//void SceneEndPoint();
void ChangeScene(const char* name);
void InitScene(const char* name);
void DeleteScene(const char* name);
void CopyScene(const char* src, const char* dest);
void SetSceneCamera(const char* scene, const char* camera);
void UpdateScene();
void DrawScene();
const char* GetCurrentSceneName();
//void NotifyAddObject(IndexType type);

int GetCurrentSceneIndex();

  //////////////////
 // AssetManager //
//////////////////
const std::vector<ModelVertex>* GetModelVertices(const char* modelName);
ID3D11ShaderResourceView* GetTextureSRV(const char* textureName);
const WavData* GetWavData(const char* name);
bool RegisterAndLoadFileToPackage(const std::string& filepath);

bool IN_LoadTexture_Memory(const char* name, const unsigned char* data, size_t size);
bool IN_LoadFBX_Memory(const char* name, const unsigned char* data, size_t size);
bool IN_LoadModelObj_Memory(const char* name, const unsigned char* data, size_t size);
bool IN_LoadWav_Memory(const char* name, const unsigned char* data, size_t size);

void AL_Init();
void AL_Shutdown();
bool AL_RegisterAssetToBatch(const char* filepath);
bool AL_SaveAllPackages(const char* outFolder);
bool AL_LoadPackageIndex(const char* ext, const char* pkgFilePath);
bool AL_LoadFromPackageByIndex(const char* ext, int index);
int AL_GetIndexFromPackage(const char* ext, const char* name);
int AL_GetPackageCount(); // number of distinct extensions/packages
const char* AL_GetPackageExt(int pkgIdx);
int AL_GetPackageEntryCount(const char* ext);
const char* AL_GetPackageEntryName(const char* ext, int index);
bool AL_LoadFromPackageByName(const char* name);
int AL_GetModelMeshCount(const char* modelName);
const std::vector<ModelVertex>* AL_GetModelMeshVertices(const char* modelName, int meshIdx);
const std::vector<unsigned int>* AL_GetModelMeshIndices(const char* modelName, int meshIdx);
XMFLOAT4 AL_GetModelMeshMaterialDiffuse(const char* modelName, int meshIdx);
const char* AL_GetModelMeshTextureName(const char* modelName, int meshIdx);
static ID3D11ShaderResourceView* TryResolveAndLoadTextureSRV(const std::string& rawTex, const std::string& modelPath);
int AL_RegisterFolderRecursive(const char* folder);
void AL_LoadAllPackages(const char* folder);
static bool TryLoadAlternativeImageExtensionsForEntry(const PackageEntry& e, Package& pkg);
static ID3D11ShaderResourceView* TryResolveAndLoadTextureSRV(
    const std::string& rawTex,
    std::string& outResolvedKey,
    const std::string& modelPath);
  ///////////////////
 // ShaderManager //
///////////////////

void InitShaderDefault();
void ShaderManager_Init();

void Set2DShaderVS(const char* ShaderName);
void Set2DShaderPS(const char* ShaderName);
void Set3DShaderVS(const char* ShaderName);
void Set3DShaderPS(const char* ShaderName);

void AddVertexShader(const char* shaderName, const char* shaderCode);
void AddPixelShader(const char* shaderName, const char* shaderCode);

void ShaderManager_Update();

int GetVertexShaderIndex(const char* shaderName);
int GetPixelShaderIndex(const char* shaderName);

ID3D11VertexShader* GetVertexShader2D();
ID3D11PixelShader*  GetPixelShader2D();
ID3D11VertexShader* GetVertexShader3D();
ID3D11PixelShader*  GetPixelShader3D();
ID3D11VertexShader* GetVertexShader3DGrid();
ID3D11PixelShader*  GetPixelShader3DGrid();

ID3DBlob* GetCurrent2DVSBlob();
ID3DBlob* GetCurrent3DVSBlob();
ID3DBlob* GetCurrent3DGridVSBlob();

  //////////////////
 // UtilManager  //
//////////////////
//|| ユーティリティ ||________________
void AddMessage(const char* sent);
std::wstring ConvertToWString(const char* str);
const char* ConcatCStr(const char* str1, const char* str2);
void ConcatCStrFree(const char* str);
//|| Vec4 系 ||_______________________
void Vec4_Init(Vec4Vector* vec);
void Vec4_PushBack(Vec4Vector* vec, Vec4 value);
void Vec4_Set(Vec4Vector* vec, size_t index, Vec4 value);
Vec4 Vec4_Get(Vec4Vector* vec, size_t index);
void Vec4_Free(Vec4Vector* vec);
//|| Char2 系 ||______________________
void Char2_Init(Char2Vector* vec);
void Char2_PushBack(Char2Vector* vec, Char2 str);
void Char2_Set(Char2Vector* vec, size_t index, Char2 str);
Char2 Char2_Get(Char2Vector* vec, size_t index);
int Char2_GetIndex(Char2Vector* vec, const char* FirstName);
void Char2_Free(Char2Vector* vec);
//|| Int2 ||__________________________
void Int2_Init(Int2Vector* vec);
void Int2_PushBack(Int2Vector* vec, Int2 str);
void Int2_Set(Int2Vector* vec, size_t index, Int2 str);
Int2 Int2_Get(Int2Vector* vec, size_t index);
int  Int2_GetIndex(Int2Vector* vec, int OneIndex);
void Int2_Free(Int2Vector* vec);
//|| Char 系 ||_______________________
void VecC_Init(CharVector* vec);
void VecC_PushBack(CharVector* vec, const char* str);
void VecC_Set(CharVector* vec, size_t index, const char* str);
const char* VecC_Get(CharVector* vec, size_t index);
void VecC_Free(CharVector* vec);
//|| Int 系 ||________________________
void VecInt_Init(IntVector* vec);
void VecInt_PushBack(IntVector* vec, int value);
void VecInt_Set(IntVector* vec, size_t index, int value);
int VecInt_Get(IntVector* vec, size_t index);
void VecInt_Free(IntVector* vec);
//|| Bool 系 ||_______________________
void VecBool_Init(BoolVector* vec);
void VecBool_PushBack(BoolVector* vec, bool value);
void VecBool_Set(BoolVector* vec, size_t index, bool value);
bool VecBool_Get(BoolVector* vec, size_t index);
void VecBool_Free(BoolVector* vec);
//|| KeyMap 系 ||______________________
void KeyMap_Init(KeyMap* map);
int KeyMap_Add(KeyMap* map, const char* key);
int KeyMap_GetIndex(KeyMap* map, const char* key);
const char* KeyMap_GetKey(KeyMap* map, int index);
int KeyMap_GetSize(KeyMap* map);
void KeyMap_SetKey(KeyMap* map, size_t index, const char* key);
void KeyMap_Free(KeyMap* map);

  //////////////////
 // InputManager //
//////////////////
void InitInput();
void UpdateInput();
void ReleaseInput();
int GetKeyBoardNum();
int GetMouseNum();
int GetControllerNum();
bool GetInputState(Input input, int index);
//API用
bool Input_GetKey(Input input);
bool Input_GetPad(Input input, int index);

//
// MotionManager //
//
/// 初期化 / 終了
void MM_Init();
void MM_Shutdown();

/// シーンをパッケージ化（key: モデル名やパスをキーに使う）
/// 戻り値: true=登録成功（既に存在していれば true を返す）
bool MM_LoadFromScene(const char* key, const aiScene* scene);

/// 指定パッケージのインデックス取得 (存在しない場合 -1)
int MM_GetIndex(const char* key);

/// インデックスからパッケージ取得 (内部ポインタ、変更不可)
MotionPackage* MM_GetPackage(int index);

/// デバッグ表示（MessageBox）
void MM_DebugPrint();

/// アニメーション（aiAnimation）から自動でモーション区間候補を検出する
/// 戻り値: (start,end) ペアの vector を [start0,end0,start1,end1,...] の形で返す
std::vector<double> MM_DetectMotionSegmentsFromAnimation(const aiAnimation* anim);

/// 指定パッケージにユーザー指定の区間を追加しインデックスを返す（重複検出して既存があればそれを返す）
int MM_AddMotionRange(int packageIndex, const std::string& name, double startTime, double endTime);

/// パッケージ内のモーション数取得
int MM_GetMotionCount(int packageIndex);

/// パッケージ内のモーション情報取得（nullptr なら無効）
const MotionRange* MM_GetMotionRange(int packageIndex, int motionIndex);

  ////////////////////
 // NetworkManager //
////////////////////
bool GetNetStart();
std::string GetLocalIP();
void InitNet(int SetSC);
void SetIP(const char* SetIP);
void UpdateNet();
void Net_AddSendStruct(const char* name, Vec4 init);
void Net_SetSendStruct(const char* name, Vec4 value);
Vec4 Net_GetRecvStruct(const char* name);

void UDPInit(int SetSC);
void UDPUpdate();