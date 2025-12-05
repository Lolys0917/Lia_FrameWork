//=====================================================================
// MotionManager.cpp   （完全版：パッケージ化対応 / クラス不使用）
//=====================================================================
#include <vector>
#include <string>
#include <unordered_map>
#include <windows.h>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <assimp/scene.h>

#include "AssetLoad.h"
#include "Manager.h"

//=====================================================================
// ★ Motion データ構造（クラスなし、純粋な構造体）
//=====================================================================
struct Motion {
    std::string name;          // モーション名（KeyMap用）
    double startTime;          // 開始時刻（秒 or ticks）
    double endTime;            // 終了時刻
    double ticksPerSecond;     // 必須（FBX/OBJ）
    const aiAnimation* anim;   // Assimp のアニメーション元
};

//=====================================================================
// ★ Motion パッケージ（ModelMotion.pkg）へ保存する構造
//=====================================================================
struct MotionPackageEntry {
    Motion motion;
};
static std::vector<MotionPackageEntry> g_MotionPackage;

//=====================================================================
// ★ KeyMap：MotionName → インデックス
//=====================================================================
static KeyMap g_MotionMap;   // MotionName → index

//=====================================================================
// ★ 重複チェック
//=====================================================================
static int FindDuplicate(const Motion& m)
{
    int sz = (int)g_MotionPackage.size();
    for (int i = 0; i < sz; i++)
    {
        const Motion& x = g_MotionPackage[i].motion;

        if (x.anim == m.anim &&
            fabs(x.startTime - m.startTime) < 0.0001 &&
            fabs(x.endTime - m.endTime) < 0.0001)
        {
            return i;
        }
    }
    return -1;
}

//=====================================================================
// ★ パッケージへ Motion を追加
//=====================================================================
int RegisterMotion(const Motion& m)
{
    int dup = FindDuplicate(m);
    if (dup >= 0)
        return dup;

    // パッケージ登録
    MotionPackageEntry entry;
    entry.motion = m;

    int newIndex = (int)g_MotionPackage.size();
    g_MotionPackage.push_back(entry);

    // KeyMap へ登録
    KeyMap_Add(&g_MotionMap, m.name.c_str());

    return newIndex;
}

//=====================================================================
// ★ Debug 表示
//=====================================================================
void DebugPrintMotions()
{
    std::ostringstream oss;

    oss << "Motion Count = " << g_MotionPackage.size() << "\n\n";

    for (size_t i = 0; i < g_MotionPackage.size(); i++)
    {
        const Motion& m = g_MotionPackage[i].motion;
        oss << "[" << i << "] "
            << m.name
            << "  Start=" << m.startTime
            << "  End=" << m.endTime
            << "  TPS=" << m.ticksPerSecond
            << "\n";
    }

    MessageBoxA(NULL, oss.str().c_str(), "MotionManager Debug", MB_OK);
}

//=====================================================================
// ★ FBX / OBJ 内の全モーション自動登録
//=====================================================================
void AddMotionsFromScene(const aiScene* scene, const char* modelName)
{
    if (!scene || scene->mNumAnimations == 0) return;

    for (unsigned int i = 0; i < scene->mNumAnimations; i++)
    {
        const aiAnimation* anim = scene->mAnimations[i];
        if (!anim) continue;

        Motion m;
        m.anim = anim;
        m.startTime = 0.0;
        m.endTime = anim->mDuration;
        m.ticksPerSecond = anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 30.0;

        // モーション名決定
        if (anim->mName.length > 0)
            m.name = anim->mName.C_Str();
        else
            m.name = std::string(modelName) + "_Anim" + std::to_string(i);

        RegisterMotion(m);
    }

    DebugPrintMotions();
}

//=====================================================================
// ★ ユーザー指定区間を Motion として登録
//=====================================================================
int AddMotionRange(
    const aiScene* scene,
    int animIndex,
    double start,
    double end,
    const std::string& tag
)
{
    if (!scene || animIndex < 0 || animIndex >= (int)scene->mNumAnimations)
        return -1;

    const aiAnimation* anim = scene->mAnimations[animIndex];

    Motion m;
    m.anim = anim;
    m.startTime = start;
    m.endTime = end;
    m.ticksPerSecond = anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 30;

    // 名前にタグをつける
    if (anim->mName.length > 0)
        m.name = anim->mName.C_Str();
    else
        m.name = "AnimRange";

    m.name += "_" + tag;

    int idx = RegisterMotion(m);

    DebugPrintMotions();
    return idx;
}

//=====================================================================
// ★ インデックスから Motion を取得
//=====================================================================
const Motion* GetMotion(int index)
{
    if (index < 0 || index >= (int)g_MotionPackage.size()) return nullptr;
    return &g_MotionPackage[index].motion;
}

//=====================================================================
// ★ 名前から Motion を取得
//=====================================================================
int FindMotionIndex(const char* name)
{
    return KeyMap_GetIndex(&g_MotionMap, name);
}

//=====================================================================
// ★ 外部使用 API：Model などから呼ばれる
//=====================================================================
extern "C"
{
    // モデルロード後に呼び、FBX に入っているアニメーションを自動登録
    __declspec(dllexport)
        void MM_AddSceneMotions(const aiScene* scene, const char* modelName)
    {
        AddMotionsFromScene(scene, modelName);
    }

    // 登録済み Motion の数
    __declspec(dllexport)
        int MM_GetMotionCount()
    {
        return (int)g_MotionPackage.size();
    }

    // Motion 名
    __declspec(dllexport)
        const char* MM_GetMotionName(int index)
    {
        if (index < 0 || index >= (int)g_MotionPackage.size()) return "";
        return g_MotionPackage[index].motion.name.c_str();
    }

    // Motion 構造体を取得
    __declspec(dllexport)
        const Motion* MM_GetMotion(int index)
    {
        return GetMotion(index);
    }
}
