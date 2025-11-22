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
#include "Component.h"

#include <string>
#include <vector>

#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXMathMatrix.inl>
#include <wrl.h>

#pragma comment (lib, "d3dcompiler.lib")

// Define_________________________

// 構造体定義_____________________
// Vec4構造体
typedef struct { float X, Y, Z, W; } Vec4;
// Char2構造体
typedef struct { const char* First; const char* End; } Char2;
// ベクター型構造体
typedef struct { Vec4*  data; size_t size; size_t capacity; } Vec4Vector;
typedef struct { Char2* data; size_t size; size_t capacity; } Char2Vector;
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
void AddCamera(const char* name);                                                   //カメラの追加
void SetCameraPos(const char* name, float x, float y, float z);                     //カメラの座標設定
void SetCameraLook(const char* name, float x, float y, float z);                    //カメラの注視点設定
void UseCameraSet(const char* name);                                                //使用するカメラの設定
int GetUseCamera();                                                                 //使用するカメラのインデックスを取得
void SetUseCamera(int index);                                                       //使用するカメラをインデックスで指定
//void SettingCameraOnce();
//|| SpriteWorld ||__________________
void AddSpriteWorld(const char* name, const char* pathName);                        //板ポリ追加テクスチャ指定
void SetSpriteWorldPos(const char* name, float x, float y, float z);                //板ポリ座標設定
void SetSpriteWorldSize(const char* name, float x, float y, float z);               //板ポリサイズ設定
void SetSpriteWorldAngle(const char* name, float x, float y, float z);              //板ポリ角度設定
void SetSpriteWorldColor(const char* name, float r, float g, float b, float a);     //板ポリ乗算色設定
//|| SpriteScreen ||_________________                                               //
void AddSpriteScreen(const char* name, const char* pathName);                       //UIの追加テクスチャ指定
void SetSpriteScreenPos(const char* name, float x, float y);                        //UI座標設定
void SetSpriteScreenSize(const char* name, float x, float y);                       //UIサイズ設定
void SetSpriteScreenAngle(const char* name, float angle);                           //UI角度設定
void SetSpriteScreenColor(const char* name, float r, float g, float b, float a);    //UI色設定
//|| SpriteBox ||____________________                                               //
void AddSpriteBox(const char* name, const char* pathName);                          //箱形の追加テクスチャ指定※全体
void SetSpriteBoxPos(const char* name, float x, float y, float z);                  //箱形の座標設定
void SetSpriteBoxSize(const char* name, float x, float y, float z);                 //箱形のサイズ設定
void SetSpriteBoxAngle(const char* name, float x, float y, float z);                //箱形の角度設定
void SetSpriteBoxColor(const char* name, float r, float g, float b, float a);       //箱形の色設定※乗算
void SetSpriteBoxTextureTop(const char* name, const char* pathName);                //箱形のテクスチャ設定上面
void SetSpriteBoxTextureBottom(const char* name, const char* pathName);             //箱形のテクスチャ設定底面
void SetSpriteBoxTextureFront(const char* name, const char* pathName);              //箱形のテクスチャ設定前面
void SetSpriteBoxTextureRear(const char* name, const char* pathName);               //箱形のテクスチャ設定後面
void SetSpriteBoxTextureLeft(const char* name, const char* pathName);               //箱形のテクスチャ設定左面
void SetSpriteBoxTextureRight(const char* name, const char* pathName);              //箱形のテクスチャ設定右面
void SetSpriteBoxTexture(const char* name, const char* pathName);                   //箱形のテクスチャ設定全体
//|| SpriteCylinder ||_______________                                               //
void AddSpriteCylinder(const char* name, const char* pathName);                     //円柱の追加テクスチャ指定
void SetSpriteCylinderPos(const char* name, float x, float y, float z);             //円柱の座標設定
void SetSpriteCylinderSize(const char* name, float x, float y, float z);            //円柱のサイズ設定
void SetSpriteCylinderAngle(const char* name, float x, float y, float z);           //円柱の角度設定
void SetSpriteCylinderColor(const char* name, float r, float g, float b, float a);  //円柱の色設定※乗算
void SetSpriteCylinderSegment(const char* name, int sengment);                      //円柱の角数設定
void SetSpriteCylinderTextureTop(const char* name, const char* pathName);           //円柱のテクスチャ設定上面
void SetSpriteCylinderTextureBottom(const char* name, const char* pathName);        //円柱のテクスチャ設定底面
void SetSpriteCylinderTextureSide(const char* name, const char* pathName);          //円柱のテクスチャ設定周面
//|| Grid   ||_______________________                                               //
// Grid Line                                                                        //
void AddGridLine(const char* name);                                                 //グリッドの追加
void SetGridLinePos(const char* name, float Start, float End);                      //グリッドの描画範囲指定
void SetGridLineColor(const char* name, float r, float g, float b, float a);        //グリッドの色設定
// Grid Box                                                                         //
void AddGridBox(const char* name);                                                  //箱形グリッドの追加
void SetGridBoxPos(const char* name, float x, float y, float z);                    //箱形グリッドの座標設定
void SetGridBoxSize(const char* name, float x, float y, float z);                   //箱形グリッドのサイズ設定
void SetGridBoxColor(const char* name, float R, float G, float B, float A);         //箱形グリッドの色設定
// Grid Polygon                                                                     //
void AddGridPolygon(const char* name);                                              //多角グリッドの追加
void SetGridPolygonPos(const char* name, float x, float y, float z);                //多角グリッドの座標設定
void SetGridPolygonSize(const char* name, float x, float y, float z);               //多角グリッドのサイズ設定
void SetGridPolygonAngle(const char* name, float x, float y, float z);              //多角グリッドの角度設定
void SetGridPolygonColor(const char* name, float R, float G, float B, float A);     //多角グリッドの色設定
void SetGridPolygonSides(const char* name, int sides);                              //多角グリッドの角数設定
//|| Sound ||_______________________ 
//World
void AddSpeaker(const char* name, const char* pathName);                            //スピーカーの追加音源指定
void SetSpeakerPos(const char* name, float x, float y, float z);                    //スピーカーの座標設定
void SetSpeakerSound(const char* name, const char* pathName);                       //スピーカーの音源設定
//Sound
void AddSound(const char* name, const char* pathName);                              //サウンドの追加音源指定
void SetSoundPan(const char* name, float pan);                                      //サウンドのパン設定
//SoundEffect
void SetSFxDelay(const char* name, int ms, int attenuation);                        //サウンドディレイ
void SetSFxReverb(const char* name, int ms, int attenuation, int Range);            //サウンドリバーブ
void SetSFxCompressor(const char* name, int Retio);                                 //サウンドコンプレッサー
void SetSFxLimiter(const char* name, int Max);                                      //サウンドリミッター
void SetSFxGate(const char* name, int min);                                         //サウンドゲート
//|| Light ||_________________________                                              //
void AddLight(const char* name, LightType LT);                                      //ライトの追加ライトタイプ指定
void SetLightPos(const char* name, float x, float y, float z);                      //ライトの座標設定
void SetLightAngle(const char* name, float x, float y, float z);                    //ライトの角度設定
void SetLightRange(const char* name, float range);                                  //ライトの範囲設定
void SetLightLength(const char* name, float length);                                //ライトの長さ設定※ライトが届く距離
void SetLightColor(const char* name, float r, float g, float b, float a);           //ライトの色設定
void SetLightAttenuation(const char* name, float attenuation);                      //ライトの減衰度設定
//|| Model ||_________________________
void AddModel(const char* name, const char* pathName);                              //モデルの追加
void SetModelPos(const char* name, float x, float y, float z);                      //モデルの座標設定
void SetModelSize(const char* name, float x, float y, float z);                     //モデルのサイズ設定
void SetModelAngle(const char* name, float x, float y, float z);                    //モデルの角度設定
void SetModelMotion(const char* name, const char* pathName, int Attack);            //モデルのモーション設定移行速度設定

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
const std::vector<ModelVertex>* GetModelVertex(const char* modelName);
ID3D11ShaderResourceView* GetTextureSRV(const char* textureName);

bool IN_LoadTexture_Memory(const char* name, const unsigned char* data, size_t size);
bool IN_LoadFBX_Memory(const char* name, const unsigned char* data, size_t size);
bool IN_LoadModelObj_Memory(const char* name, const unsigned char* data, size_t size);
bool IN_LoadWav_Memory(const char* name, const unsigned char* data, size_t size);

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