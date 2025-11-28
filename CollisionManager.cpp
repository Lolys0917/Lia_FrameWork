#include "ComponentCollision.h"
#include "Manager.h"

//処理順について
// UpdateCollisionで全判定をfalse指定
// 現在の判定を取得
// APIへ移行し判定が適応される


void UpdateCollision()
{
    //毎フレーム開始時にfalse化
    //この後に判定をおこなう
    ObjectDataPool* p = GetObjectDataPool();
    for (int i = 0; i < KeyMap_GetSize(&p->CollisionMap); i++)
    {
        VecBool_Set(&p->CollisionHit, i, false);
    }
}

//計算用関数群-------------------------
// --------------------------------------
// Sphere vs Sphere
// --------------------------------------
bool HitSphereSphere(const XMFLOAT3& p1, float r1,
    const XMFLOAT3& p2, float r2)
{
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    float dz = p1.z - p2.z;
    float distSq = dx * dx + dy * dy + dz * dz;

    float r = r1 + r2;
    return distSq <= r * r;
}

// --------------------------------------
// Box vs Box (AABB)
// --------------------------------------
bool HitBoxBox(const XMFLOAT3& p1, const XMFLOAT3& s1,
    const XMFLOAT3& p2, const XMFLOAT3& s2)
{
    // AABB half-size
    float h1x = s1.x * 0.5f;
    float h1y = s1.y * 0.5f;
    float h1z = s1.z * 0.5f;

    float h2x = s2.x * 0.5f;
    float h2y = s2.y * 0.5f;
    float h2z = s2.z * 0.5f;

    return
        (fabs(p1.x - p2.x) <= (h1x + h2x)) &&
        (fabs(p1.y - p2.y) <= (h1y + h2y)) &&
        (fabs(p1.z - p2.z) <= (h1z + h2z));
}


// --------------------------------------
// Sphere vs Box
// --------------------------------------
bool HitSphereBox(const XMFLOAT3& sp, float r,
    const XMFLOAT3& bp, const XMFLOAT3& bs)
{
//    float hx = bs.x * 0.5f;
//    float hy = bs.y * 0.5f;
//    float hz = bs.z * 0.5f;
//
//    float cx = std::max(bp.x - hx, std::min(sp.x, bp.x + hx));
//    float cy = std::max(bp.y - hy, std::min(sp.y, bp.y + hy));
//    float cz = std::max(bp.z - hz, std::min(sp.z, bp.z + hz));
//
//    float dx = sp.x - cx;
//    float dy = sp.y - cy;
//    float dz = sp.z - cz;
//
//    return (dx * dx + dy * dy + dz * dz) <= r * r;
}


//判定取得------------------------------
bool HitJudgeTo(
    XMFLOAT3 pos1, XMFLOAT3 size1, XMFLOAT3 angle1, CollisionType type1,
    XMFLOAT3 pos2, XMFLOAT3 size2, XMFLOAT3 angle2, CollisionType type2)
{
    // Sphere vs Sphere
    if (type1 == CollisionType::CollisionSphere && type2 == CollisionType::CollisionSphere)
    {
        return HitSphereSphere(pos1, size1.x, pos2, size2.x);
    }

    // Box vs Box
    if (type1 == CollisionType::CollisionBox && type2 == CollisionType::CollisionBox)
    {
        return HitBoxBox(pos1, size1, pos2, size2);
    }

    // Sphere vs Box
    if (type1 == CollisionType::CollisionSphere && type2 == CollisionType::CollisionBox)
    {
        return HitSphereBox(pos1, size1.x, pos2, size2);
    }
    if (type1 == CollisionType::CollisionBox && type2 == CollisionType::CollisionSphere)
    {
        return HitSphereBox(pos2, size2.x, pos1, size1);
    }

    return false;

    // Cylinder 系（必要なら追加）
    // if(type1 == CollisionType::Cylinder || type2 == CollisionType::Cylinder)...
}

void HitJudgeAll()
{
	//全ての当たり判定を取得
    ObjectDataPool* p = GetObjectDataPool();

    for (int i = 0; i < KeyMap_GetSize(&p->CollisionMap); i++)
    {
        for (int ii = 0; ii < KeyMap_GetSize(&p->CollisionMap); ii++)
        {
            if (!(i == ii))
            {
                bool hit =
                    HitJudgeTo(
                        GetObjectClass()->GetComponent<Collision>(i)->GetWorldPos(),
                        GetObjectClass()->GetComponent<Collision>(i)->GetWorldSize(),
                        GetObjectClass()->GetComponent<Collision>(i)->GetWorldAngle(),
                        GetObjectClass()->GetComponent<Collision>(i)->GetType(),
                        GetObjectClass()->GetComponent<Collision>(ii)->GetWorldPos(),
                        GetObjectClass()->GetComponent<Collision>(ii)->GetWorldSize(),
                        GetObjectClass()->GetComponent<Collision>(ii)->GetWorldAngle(),
                        GetObjectClass()->GetComponent<Collision>(ii)->GetType()
                    );

                //VecBool_Set(GetObjectDataPool()->CollisionHit())
            }
        }
    }
}

//API用-----------------------------

bool CollisionJudgeTo(const char* Col1, const char* Col2)
{//特定の物体と当たっていた場合

}
bool CollisionJudgeAll(const char* Col)
{

}