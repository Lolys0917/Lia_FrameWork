#pragma once
#include "Manager.h"
#include <string>
#include <vector>
#include <DirectXMath.h>
// 初期化 / 終了
void AL_Init();
void AL_Shutdown();

// Startupフェーズ（StartUp内）で呼んでファイルを登録する
// -> 登録した順番でパッケージにバイナリをためる
// 戻り値: true=登録成功(既に登録済みならtrueを返す)
bool AL_RegisterAssetToBatch(const char* filepath);

// バッチを.pkgとして書き出す（拡張子によるファイル毎に1つ作る）
// outFolder 例: "saved/Package/"
bool AL_SaveAllPackages(const char* outFolder);

// 実行時：パッケージを読み込み（ヘッダ＋テーブルを読み、メタだけ保持）
// -> 実行時の Load / Extract 用。extは例 "png" "fbx"（拡張子）
bool AL_LoadPackageIndex(const char* ext, const char* pkgFilePath);

// pkg内からインデックス指定でアセットをゲームメモリに読み込む
// nameOrIndex: 文字列指定なら KeyMap から index を取り、 index >= 0 で読み込み
// 例えば: AL_LoadFromPackageByName("Texture.png") あるいは AL_LoadFromPackageByIndex("png", 2)
// 戻り値: true=ロード成功（既存のIN_LoadTexture / IN_LoadFBX を呼ぶ実装）
bool AL_LoadFromPackageByName(const char* name); // キーマップに登録済みの名前を使う
bool AL_LoadFromPackageByIndex(const char* ext, int index);

// パッケージ内での index を取得（KeyMap 経由）
// return: index (>=0) or -1
int AL_GetIndexFromPackage(const char* ext, const char* name);

// pkg メタ情報を列挙したい場合の補助
int AL_GetPackageCount(); // number of distinct extensions/packages
const char* AL_GetPackageExt(int pkgIdx);
int AL_GetPackageEntryCount(const char* ext);
const char* AL_GetPackageEntryName(const char* ext, int index);