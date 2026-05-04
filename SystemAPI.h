#pragma once

//API_SETTING ___________________
#ifdef SYSTEM_EXPORTS
#define SYSTEM_API __declspec(dllexport)
#else
#define SYSTEM_API __declspec(dllimport)
#endif

  //////////////////
 // DebugManager //
//////////////////
void InitDebugManager();
void DrawMessageBox();
extern "C" SYSTEM_API void MessageBoxText(const char* text, const char* caption);
extern "C" SYSTEM_API void AddCamera(const char* name);                                                   //カメラの追加
extern "C" SYSTEM_API void SetCameraPos(const char* name, float x, float y, float z);                     //カメラの座標設定
extern "C" SYSTEM_API void SetCameraLook(const char* name, float x, float y, float z);                    //カメラの注視点設定
extern "C" SYSTEM_API void UseCameraSet(const char* name);                                                //使用するカメラの設定
extern "C" SYSTEM_API int  GetUseCamera();                                                                 //使用するカメラのインデックスを取得
extern "C" SYSTEM_API void SetUseCamera(int index);                                                       //使用するカメラをインデックスで指定
//|| SpriteWorld ||__________________
extern "C" SYSTEM_API void AddSpriteWorld(const char* name, const char* pathName);                        //板ポリ追加テクスチャ指定
extern "C" SYSTEM_API void SetSpriteWorldPos(const char* name, float x, float y, float z);                //板ポリ座標設定
extern "C" SYSTEM_API void SetSpriteWorldSize(const char* name, float x, float y, float z);               //板ポリサイズ設定
extern "C" SYSTEM_API void SetSpriteWorldAngle(const char* name, float x, float y, float z);              //板ポリ角度設定
extern "C" SYSTEM_API void SetSpriteWorldColor(const char* name, float r, float g, float b, float a);     //板ポリ乗算色設定
//|| SpriteScreen ||_________________                                               //
extern "C" SYSTEM_API void AddSpriteScreen(const char* name, const char* pathName);                       //UIの追加テクスチャ指定
extern "C" SYSTEM_API void SetSpriteScreenPos(const char* name, float x, float y);                        //UI座標設定
extern "C" SYSTEM_API void SetSpriteScreenSize(const char* name, float x, float y);                       //UIサイズ設定
extern "C" SYSTEM_API void SetSpriteScreenAngle(const char* name, float angle);                           //UI角度設定
extern "C" SYSTEM_API void SetSpriteScreenColor(const char* name, float r, float g, float b, float a);    //UI色設定
//|| SpriteBox ||____________________                                               //
extern "C" SYSTEM_API void AddSpriteBox(const char* name, const char* pathName);                          //箱形の追加テクスチャ指定※全体
extern "C" SYSTEM_API void SetSpriteBoxPos(const char* name, float x, float y, float z);                  //箱形の座標設定
extern "C" SYSTEM_API void SetSpriteBoxSize(const char* name, float x, float y, float z);                 //箱形のサイズ設定
extern "C" SYSTEM_API void SetSpriteBoxAngle(const char* name, float x, float y, float z);                //箱形の角度設定
extern "C" SYSTEM_API void SetSpriteBoxColor(const char* name, float r, float g, float b, float a);       //箱形の色設定※乗算
extern "C" SYSTEM_API void SetSpriteBoxTextureTop(const char* name, const char* pathName);                //箱形のテクスチャ設定上面
extern "C" SYSTEM_API void SetSpriteBoxTextureBottom(const char* name, const char* pathName);             //箱形のテクスチャ設定底面
extern "C" SYSTEM_API void SetSpriteBoxTextureFront(const char* name, const char* pathName);              //箱形のテクスチャ設定前面
extern "C" SYSTEM_API void SetSpriteBoxTextureRear(const char* name, const char* pathName);               //箱形のテクスチャ設定後面
extern "C" SYSTEM_API void SetSpriteBoxTextureLeft(const char* name, const char* pathName);               //箱形のテクスチャ設定左面
extern "C" SYSTEM_API void SetSpriteBoxTextureRight(const char* name, const char* pathName);              //箱形のテクスチャ設定右面
extern "C" SYSTEM_API void SetSpriteBoxTexture(const char* name, const char* pathName);                   //箱形のテクスチャ設定全体
//|| SpriteCylinder ||_______________                                               //
extern "C" SYSTEM_API void AddSpriteCylinder(const char* name, const char* pathName);                     //円柱の追加テクスチャ指定
extern "C" SYSTEM_API void SetSpriteCylinderPos(const char* name, float x, float y, float z);             //円柱の座標設定
extern "C" SYSTEM_API void SetSpriteCylinderSize(const char* name, float x, float y, float z);            //円柱のサイズ設定
extern "C" SYSTEM_API void SetSpriteCylinderAngle(const char* name, float x, float y, float z);           //円柱の角度設定
extern "C" SYSTEM_API void SetSpriteCylinderColor(const char* name, float r, float g, float b, float a);  //円柱の色設定※乗算
extern "C" SYSTEM_API void SetSpriteCylinderSegment(const char* name, int sengment);                      //円柱の角数設定
extern "C" SYSTEM_API void SetSpriteCylinderTextureTop(const char* name, const char* pathName);           //円柱のテクスチャ設定上面
extern "C" SYSTEM_API void SetSpriteCylinderTextureBottom(const char* name, const char* pathName);        //円柱のテクスチャ設定底面
extern "C" SYSTEM_API void SetSpriteCylinderTextureSide(const char* name, const char* pathName);          //円柱のテクスチャ設定周面
//|| Grid   ||_______________________                                               //
// Grid
extern "C" SYSTEM_API void AddGrid(const char* name, GridType type);                                                  //箱形グリッドの追加
extern "C" SYSTEM_API void SetGridPos(const char* name, float x, float y, float z);                    //箱形グリッドの座標設定
extern "C" SYSTEM_API void SetGridSize(const char* name, float x, float y, float z);                   //箱形グリッドのサイズ設定
extern "C" SYSTEM_API void SetGridColor(const char* name, float R, float G, float B, float A);         //箱形グリッドの色設定
extern "C" SYSTEM_API void SetGridSides(const char* name, int sides);                              //多角グリッドの角数設定
//|| Sound ||_______________________ 
//World
extern "C" SYSTEM_API void AddSpeaker(const char* name, const char* pathName);                            //スピーカーの追加音源指定
extern "C" SYSTEM_API void SetSpeakerPos(const char* name, float x, float y, float z);                    //スピーカーの座標設定
extern "C" SYSTEM_API void SetSpeakerSound(const char* name, const char* pathName);                       //スピーカーの音源設定
//Sound
extern "C" SYSTEM_API void AddSound(const char* name, const char* pathName);                              //サウンドの追加音源指定
extern "C" SYSTEM_API void SetSoundPan(const char* name, float pan);                                      //サウンドのパン設定
//SoundEffect
extern "C" SYSTEM_API void SetSFxDelay(const char* name, int ms, int attenuation);                        //サウンドディレイ
extern "C" SYSTEM_API void SetSFxReverb(const char* name, int ms, int attenuation, int Range);            //サウンドリバーブ
extern "C" SYSTEM_API void SetSFxCompressor(const char* name, int Retio);                                 //サウンドコンプレッサー
extern "C" SYSTEM_API void SetSFxLimiter(const char* name, int Max);                                      //サウンドリミッター
extern "C" SYSTEM_API void SetSFxGate(const char* name, int min);                                         //サウンドゲート
//|| Light ||_________________________                                              //
extern "C" SYSTEM_API void AddLight(const char* name, LightType LT);                                      //ライトの追加ライトタイプ指定
extern "C" SYSTEM_API void SetLightPos(const char* name, float x, float y, float z);                      //ライトの座標設定
extern "C" SYSTEM_API void SetLightAngle(const char* name, float x, float y, float z);                    //ライトの角度設定
extern "C" SYSTEM_API void SetLightRange(const char* name, float range);                                  //ライトの範囲設定
extern "C" SYSTEM_API void SetLightLength(const char* name, float length);                                //ライトの長さ設定※ライトが届く距離
extern "C" SYSTEM_API void SetLightColor(const char* name, float r, float g, float b, float a);           //ライトの色設定
extern "C" SYSTEM_API void SetLightAttenuation(const char* name, float attenuation);                      //ライトの減衰度設定
//|| Model ||_________________________
extern "C" SYSTEM_API void AddModel(const char* name, const char* pathName);                              //モデルの追加
extern "C" SYSTEM_API void SetModelPos(const char* name, float x, float y, float z);                      //モデルの座標設定
extern "C" SYSTEM_API void SetModelSize(const char* name, float x, float y, float z);                     //モデルのサイズ設定
extern "C" SYSTEM_API void SetModelAngle(const char* name, float x, float y, float z);                    //モデルの角度設定
extern "C" SYSTEM_API void SetModelMotion(const char* name, const char* pathName, int Attack);            //モデルのモーション設定移行速度設定
extern "C" SYSTEM_API void ModelTexture(const char* name, const char* pathName);
//|| Collision ||_____________________
extern "C" SYSTEM_API void AddCollision(const char* name, const char* tag);
extern "C" SYSTEM_API bool HitToTag(const char* name, const char* tag);
extern "C" SYSTEM_API bool HitToName(const char* name1, const char* name2);
extern "C" SYSTEM_API void SetCollisionParent(const char* name, const char* parent);
extern "C" SYSTEM_API void SetCollisionPos(const char* name, float x, float y, float z);
extern "C" SYSTEM_API void SetCollisionSize(const char* name, float x, float y, float z);
extern "C" SYSTEM_API void SetCollisionAngle(const char* name, float x, float y, float z);
//void SetCollisionType(const char* name, CollisionType type);
