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

// Include________________________
#include "Grid.h"
#include "Object.h"

// Define_________________________

// 構造体定義_____________________
// Vec4構造体
typedef struct { float X, Y, Z, W; } Vec4;
// ベクター型構造体
typedef struct { Vec4* data;  size_t size; size_t capacity; } Vec4Vector;
typedef struct { char** data; size_t size; size_t capacity; } CharVector;
typedef struct { int* data;   size_t size; size_t capacity; } IntVector;
typedef struct { float* data; size_t size; size_t capacity; } FloatVector;
typedef struct { bool* data;  size_t size; size_t capacity; } BoolVector;
typedef struct { char** keys; size_t size; size_t capacity; } KeyMap;
// ObjectIndex構造体
typedef struct {
	int CameraIndex;        //Camera_________
    int SpriteWorldIndex;	//Sprite_________
    int SpriteScreenIndex;
    int SpriteBoxIndex;
    int SpriteCylinderIndex;
	int ModelIndex;			//Model__________
	int BoxColliderIndex;	//Collider_______
	int SphereColliderIndex;
	int CapsuleColliderIndex;
	int GridLineIndex;      //Grid___________
	int GridBoxIndex;
	int GridPolygonIndex;
	int GridSphereIndex;
	int GridCapsuleIndex;
	int EffectIndex;		//Effect_________
} ObjectIndex;
enum class IndexType {
	Camera,
	SpriteWorld,
	SpriteScreen,
	Model,
	BoxCollider,
	SphereCollider,
	CapsuleCollider,
	GridLine,
	GridBox,
	GridPolygon,
	GridSphere,
	GridCapsule,
	Effect
};
//モデル用マトリクスバッファ
struct MatrixBuffer
{
    XMMATRIX mvp;
    XMFLOAT4 diffuseColor;
    int useTexture;
    XMFLOAT3 pad;
};
//モデル用頂点情報
struct ModelVertex
{
    XMFLOAT3 pos;
    XMFLOAT2 uv;
    XMFLOAT3 normal;
};

//-----------------------------------------
// Vec4管理用データプール構造体
struct ObjectDataPool {
    // Camera
    Vec4Vector CameraPos;
    Vec4Vector CameraLook;
    // UI
    Vec4Vector UITBLR;
    Vec4Vector UIAngle;
    Vec4Vector UIColor;
    // SpriteWorld
    Vec4Vector SpriteWorldPos;
    Vec4Vector SpriteWorldSize;
    Vec4Vector SpriteWorldAngle;
    Vec4Vector SpriteWorldColor;
    // SpriteScreen
    Vec4Vector SpriteScreenPos;
    Vec4Vector SpriteScreenSize;
    Vec4Vector SpriteScreenColor;
    IntVector  SpriteScreenAngle;
    //SpriteBox
    Vec4Vector SpriteBoxPos;
    Vec4Vector SpriteBoxSize;
    Vec4Vector SpriteBoxAngle;
    Vec4Vector SpriteBoxColor;
    //SpriteCylinder
    Vec4Vector SpriteCylinderPos;
    Vec4Vector SpriteCylinderSize;
    Vec4Vector SpriteCylinderAngle;
    Vec4Vector SpriteCylinderColor;
    IntVector  SpriteCylinderSegment;
    // Model
    Vec4Vector ModelPos;
    Vec4Vector ModelSize;
    Vec4Vector ModelAngle;
    // BoxCollider
    Vec4Vector BoxColliderPos;
    Vec4Vector BoxColliderSize;
    Vec4Vector BoxColliderAngle;
    // Grid(Box / Polygon)
    Vec4Vector GridBoxPos;
    Vec4Vector GridBoxSize;
    Vec4Vector GridBoxAngle;
    Vec4Vector GridBoxColor;
    Vec4Vector GridPolygonPos;
    Vec4Vector GridPolygonSize;
    Vec4Vector GridPolygonAngle;
    Vec4Vector GridPolygonColor;
    // Int / Bool / Char Vec
    IntVector GridPolygonSides;
    CharVector TexturePath;
    CharVector ModelPath;
    IntVector NumberOfScenes;
    IntVector ModelType;
    BoolVector BillboardW2d;
    // KeyMaps
    KeyMap CameraMap;
    KeyMap ModelMap;
    KeyMap TextureMap;
    KeyMap SpriteWorldMap;
    KeyMap SpriteScreenMap;
    KeyMap SpriteBoxMap;
    KeyMap SpriteCylinderMap;
    KeyMap UIMap;
    KeyMap BoxColliderMap;
    KeyMap GridBoxMap;
    KeyMap GridPolygonMap;
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
//|| Camera ||_______________________
void AddCamera(const char* name);
void SetCameraPos(const char* name, float x, float y, float z);
void SetCameraLook(const char* name, float x, float y, float z);
void UseCameraSet(const char* name);
int GetUseCamera();
void SetUseCamera(int index);
//void SettingCameraOnce();
//|| SpriteWorld ||__________________
void AddSpriteWorld(const char* name, const char* pathName);
void SetSpriteWorldPos(const char* name, float x, float y, float z);
void SetSpriteWorldSize(const char* name, float x, float y, float z);
void SetSpriteWorldAngle(const char* name, float x, float y, float z);
void SetSpriteWorldColor(const char* name, float r, float g, float b, float a);
//|| SpriteScreen ||_________________
void AddSpriteScreen(const char* name, const char* pathName);
void SetSpriteScreenPos(const char* name, float x, float y);
void SetSpriteScreenSize(const char* name, float x, float y);
void SetSpriteScreenAngle(const char* name, float angle);
void SetSpriteScreenColor(const char* name, float r, float g, float b, float a);
//|| SpriteBox ||____________________
void AddSpriteBox(const char* name, const char* pathName);
void SetSpriteBoxPos(const char* name, float x, float y, float z);
void SetSpriteBoxSize(const char* name, float x, float y, float z);
void SetSpriteBoxAngle(const char* name, float x, float y, float z);
void SetSpriteBoxColor(const char* name, float r, float g, float b, float a);
void SetSpriteBoxTextureTop(const char* name, const char* pathName);
void SetSpriteBoxTextureBottom(const char* name, const char* pathName);
void SetSpriteBoxTextureFront(const char* name, const char* pathName);
void SetSpriteBoxTextureRear(const char* name, const char* pathName);
void SetSpriteBoxTextureLeft(const char* name, const char* pathName);
void SetSpriteBoxTextureRight(const char* name, const char* pathName);
void SetSpriteBoxTexture(const char* name, const char* pathName);
//|| SpriteCylinder ||_______________
void AddSpriteCylinder(const char* name, const char* pathName);
void SetSpriteCylinderPos(const char* name, float x, float y, float z);
void SetSpriteCylinderSize(const char* name, float x, float y, float z);
void SetSpriteCylinderAngle(const char* name, float x, float y, float z);
void SetSpriteCylinderColor(const char* name, float r, float g, float b, float a);
void SetSpriteCylinderSegment(const char* name, int sengment);
void SetSpriteCylinderTextureTop(const char* name, const char* pathName);
void SetSpriteCylinderTextureBottom(const char* name, const char* pathName);
void SetSpriteCylinderTextureSide(const char* name, const char* pathName);
//|| Grid   ||_______________________
// Grid Line

// Grid Box
void AddGridBox(const char* name);
void SetGridBoxPos(const char* name, float x, float y, float z);
void SetGridBoxSize(const char* name, float x, float y, float z);
void SetGridBoxColor(const char* name, float R, float G, float B, float A);
// Grid Polygon
void AddGridPolygon(const char* name);
void SetGridPolygonPos(const char* name, float x, float y, float z);
void SetGridPolygonColor(const char* name, float R, float G, float B, float A);
void SetGridPolygonSides(const char* name, int sides);

///////////////////////////////////

void OutObjectIndex(ObjectIndex* out);
ObjectIndex* GetObjectIndex();

Object* GetObjectClass();
Grid* GetGridClass();

KeyMap* GetCameraKeyMap();


  //////////////////
 // SceneManager //
//////////////////
void AddScene(const char* name);
void SceneEndPoint();
void ChangeScene(const char* name);
void InitScene(const char* name);
void DeleteScene(const char* name);
void CopyScene(const char* src, const char* dest);
void SetSceneCamera(const char* scene, const char* camera);
void UpdateScene();
void DrawScene();
const char* GetCurrentSceneName();
void NotifyAddObject(IndexType type);

  //////////////////
 // AssetManager //
//////////////////
std::vector<ModelVertex>* GetModelVertex(const char* modelName);
ID3D11ShaderResourceView* GetTextureSRV(const char* textureName);

bool IN_LoadTexture(const char* filename);
bool IN_LoadModelObj(const char* filename);
bool IN_LoadFBX(const char* filename);

bool IN_LoadTexture_Memory(const char* name, const unsigned char* data, size_t size);
bool IN_LoadFBX_Memory(const char* name, const unsigned char* data, size_t size);
bool IN_LoadModelObj_Memory(const char* name, const unsigned char* data, size_t size);

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