#pragma once

#include <Windows.h>

#include <string>
#include <cstring>
#include <vector>
#include <memory>
#include <wrl.h>

#include <fstream>
#include <sstream>
#include <filesystem>

#include <chrono>
#include <set>

#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>

//Lib _________
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")


  //////////////////////
 // グローバル構造体 //
//////////////////////

// XYZW ____________________________________
typedef struct
{
	float X;
	float Y;
	float Z;
	float W;
}Vec4;

//データ保存用構造体________________________
typedef struct {
    Vec4* data;       // 実データ
    size_t size;      // 現在の要素数
    size_t capacity;  // 確保済みの容量
} Vec4Vector;
typedef struct {
    char** data;
    size_t size;      // 現在の要素数
    size_t capacity;  // 確保済みの容量
} CharVector;
typedef struct {
    int* data;
    size_t size;      // 現在の要素数
    size_t capacity;  // 確保済みの容量
} IntVector;
typedef struct {
    bool* data;
    size_t size;      // 現在の要素数
    size_t capacity;  // 確保済みの容量
} BoolVector;

//マッピング構造体
typedef struct {
    char** keys;
    size_t size;
    size_t capacity;
} KeyMap;

//雑入れ関数群
void GridDrawJudge(bool Judge);     //グリッド描画判定
void AddMessage(const char* sent);  //メッセージ追加

  //////////////////////
 // 構造体管理用関数 //
//////////////////////

// Vec4
// vector初期化
void Vec4_Init(Vec4Vector* vec);
//push_back
void Vec4_PushBack(Vec4Vector* vec, Vec4 value);
//要素の設定
void Vec4_Set(Vec4Vector* vec, size_t index, Vec4 value);
//要素を取得
Vec4 Vec4_Get(Vec4Vector* vec, size_t index);
//解放
void Vec4_Free(Vec4Vector* vec);
Vec4Vector* GetVec4Vector(int type);

// Char
void VecC_Init(CharVector* vec);
void VecC_PushBack(CharVector* vec, const char* str);
const char* VecC_Get(CharVector* vec, size_t index);
void VecC_Set(CharVector* vec, size_t index, const char* str);
void VecC_Free(CharVector* vec);
CharVector* GetVecCVector(int type);

// Int
void VecInt_Init(IntVector* vec);
void VecInt_PushBack(IntVector* vec, int str);
int VecInt_Get(IntVector* vec, size_t index);
void VecInt_Set(IntVector* vec, size_t index, int str);
void VecInt_Free(IntVector* vec);

void VecBool_Init(BoolVector* vec);
void VecBool_PushBack(BoolVector* vec, bool str);
int VecBool_Get(BoolVector* vec, size_t index);
void VecBool_Set(BoolVector* vec, size_t index, bool str);
void VecBool_Free(BoolVector* vec);

// KeyMap 関数
void KeyMap_Init(KeyMap* map);
void KeyMap_Free(KeyMap* map);
int KeyMap_EnsureCapacity(KeyMap* map);
int KeyMap_Add(KeyMap* map, const char* key);
int KeyMap_GetIndex(KeyMap* map, const char* key);
const char* KeyMap_GetKey(KeyMap* map, int index);
KeyMap* GetKeyMapByType(int type);

  ////////////////////////
 // アセット読込関数群 //
////////////////////////

void LoadModel_OBJ(          //OBJモデル読込
	const char* ModelDataName,
	const char* ModelUniqueName);
void LoadModel_FBX(         //FBXモデル読込
	const char* ModelDataName,
	const char* ModelUniqueName);
void LoadTexture(           //テクスチャ読込
	const char* TextureDataName,
	const char* TextureUniqueName);
void LoadSound(             //サウンド読込
	const char* SoundDataName,
	const char* SoundUniqueName);


  ////////////////////////
 // オブジェクト関数群 //
////////////////////////

//カメラ_____________________
//外部用関数
void AddCamera(
        const char* CameraName);
void SetCameraPos(
        const char* CameraName,
        float PosX, float PosY, float PosZ);
void SetCameraLook(
        const char* CameraName,
        float LookX, float LookY, float LookZ);
void UseCameraSet(
        const char* CameraName);
//内部用関数
void CreateCamera();
void SettingCamera();

int GetuseCamera();
int GetCameraIndex();

//2DUI ________________________
//外部用関数
void AddSpriteScreen(
        const char* TextureName,
        const char* ObjectName);
void SetSpriteScreenPos(
        const char* ObjectName,
        float PosX, float PosY);
void SetSpriteScreenSize(
        const char* ObjectName,
        float SizeX, float SizeY, float SizeZ);
void SetSpriteScreenAngle(
        const char* ObjectName,
        float AngleX, float AngleY);
void SetSpriteScreenColor(
        const char* ObjectName,
        float R, float G, float B, float A);
//内部用関数
void CreateSpriteScreen();
void SettingSpriteScreen();

//3DSprite_____________________
//外部用関数
void AddSpriteWorld(
    const char* TextureName,
    const char* ObjectName);
void SetSpriteWorldPos(
    const char* ObjectName,
    float PosX, float PosY, float PosZ);
void SetSpriteWorldSize(
    const char* ObjectName,
    float SizeX, float SizeY, float SizeZ);
void SetSpriteWorldAngle(
    const char* ObjectName,
    float AngleX, float AngleY, float PosZ);
void SetSpriteWorldColor(
    const char* ObjectName,
    float R, float G, float B, float A);
//内部用関数
void CreateSpriteWorld();
void SettingSpriteWorld();

//Model _______________________
//外部用関数
void AddModel(
        const char* TextureName,
        const char* ObjectName);
void SetModelPos(
        const char* ObjectName,
        float PosX, float PosY, float PosZ);
void SetModelSize(
        const char* ObjectName,
        float SizeX, float SizeY, float SizeZ);
void SetModelAngle(
        const char* ObjectName,
        float AngleX, float AngleY, float AngleZ);
//内部用関数
void CreateModel();
void SettingModel();

//Speaker&Sound ________________
//外部用関数
void AddSpeaker(
        const char* SoundName,
        const char* ObjectName);
void SetSpeakerPos(
        const char* ObjectName,
        float PosX, float PosY, float PosZ);

void SetSoundVolume(    //音量の設定
        const char* ObjectName,
        float Volume);
void SoundStart(        //音の再生
        const char* ObjectName);
void SoundReStart(      //音の再開
        const char* ObjectName);
void SoundStop(         //音の停止
        const char* ObjectName);
void SoundPause(        //音の一時停止
        const char* ObjectName);
//内部用関数
void CreateSpeaker();
void SettingSpeaker();

  //////////////////
 // 全体共通処理 //
//////////////////
void InitDo();
void UpdateDo();
void DrawDo();
void ReleaseDo();