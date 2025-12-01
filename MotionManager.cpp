//=====================================================================
// MotionManager.cpp   （クラスなし / データ＋関数のみ）
//=====================================================================
#include <vector>
#include <string>
#include <windows.h>
#include <sstream>
#include <algorithm>
#include <assimp/scene.h>

// ------------------------------
// Motion データ（クラスではない）
// ------------------------------
struct Motion {
    std::string name;
    double startTime;
    double endTime;
    const aiAnimation* source; // Assimp の元データ
};

// ------------------------------
static std::vector<Motion> g_Motions;
// ------------------------------


// =============================================================
// モーションを MessageBox で確認
// =============================================================
void DebugPrintMotions()
{
    std::ostringstream oss;

    oss << "Registered Motions: " << g_Motions.size() << "\n";

    for (size_t i = 0; i < g_Motions.size(); i++) {
        const Motion& m = g_Motions[i];
        oss << "[" << i << "] " << m.name
            << "  Start: " << m.startTime
            << "  End: " << m.endTime
            << "\n";
    }

    MessageBoxA(NULL, oss.str().c_str(), "MotionManager", MB_OK);
}



// =============================================================
// 重複モーションの判定（非常に簡易版）
// =============================================================
int FindDuplicateMotion(const Motion& m)
{
    for (size_t i = 0; i < g_Motions.size(); i++) {
        const Motion& x = g_Motions[i];

        if (x.source == m.source &&
            fabs(x.startTime - m.startTime) < 0.0001 &&
            fabs(x.endTime - m.endTime) < 0.0001)
        {
            return (int)i; // 同じモーションがすでに存在
        }
    }
    return -1;
}



// =============================================================
// ★ モデルに含まれるモーションをすべて追加（統合関数）
//   → モデル読み込み時にこれだけ呼べば OK
// =============================================================
void AddMotionsFromScene(const aiScene* scene)
{
    if (!scene || scene->mNumAnimations == 0) return;

    for (unsigned int i = 0; i < scene->mNumAnimations; i++)
    {
        const aiAnimation* anim = scene->mAnimations[i];
        if (!anim) continue;

        Motion m;
        m.name = anim->mName.C_Str();
        if (m.name.empty()) {
            m.name = "Anim" + std::to_string(i);
        }

        m.startTime = 0.0;
        m.endTime = anim->mDuration;
        m.source = anim;

        // すでに同じモーションがあればスキップ
        int dup = FindDuplicateMotion(m);
        if (dup >= 0) continue;

        g_Motions.push_back(m);
    }

    DebugPrintMotions();
}



// =============================================================
// ユーザーが指定した範囲を一つのモーションとして登録
// =============================================================
int AddMotionByRange(int animIndex, double start, double end, const aiScene* scene)
{
    if (!scene) return -1;
    if (animIndex < 0 || animIndex >= (int)scene->mNumAnimations) return -1;

    const aiAnimation* anim = scene->mAnimations[animIndex];

    Motion m;
    m.name = anim->mName.C_Str();
    if (m.name.empty()) m.name = "AnimRange";

    m.name += "_range_" + std::to_string((int)start) + "_" + std::to_string((int)end);
    m.startTime = start;
    m.endTime = end;
    m.source = anim;

    int dup = FindDuplicateMotion(m);
    if (dup >= 0) return dup;

    g_Motions.push_back(m);

    DebugPrintMotions();
    return (int)g_Motions.size() - 1;
}



// =============================================================
// モーション再生（Blend 付き）
// ここでは「インデックス指定で再生できる」だけを実装
// =============================================================
void PlayMotion(int index, float blendIn, float blendOut)
{
    if (index < 0 || index >= (int)g_Motions.size()) return;

    const Motion& m = g_Motions[index];

    std::ostringstream oss;
    oss << "Play Motion: " << m.name
        << "\nStart: " << m.startTime
        << " End: " << m.endTime
        << "\nBlendIn: " << blendIn
        << " BlendOut: " << blendOut;

    MessageBoxA(NULL, oss.str().c_str(), "PlayMotion", MB_OK);

    // 実際のボーン更新処理は Model 側で実装する想定
}
