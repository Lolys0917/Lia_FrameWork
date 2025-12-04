#pragma once

#include "Manager.h"
#include "Object.h"
#include "Component.h"

#include <DirectXMath.h>

using namespace DirectX;

enum CollisionType
{
    CollisionBox,
    CollisionSphere,
    CollisionCylinder
};

class Collision : public Component
{
public:
    using Component::Component;

    void Update() override;

    //ベースの値はComponent基準Get/Set ??

    // ---- Offset 設定 ----
    void SetOffsetPos(float x, float y, float z) { m_offPos = { x,y,z }; }
    void SetOffsetSize(float x, float y, float z) { m_offSize = { x,y,z }; }
    void SetOffsetAngle(float x, float y, float z) { m_offAngle = { x,y,z }; }

    // ---- 判定用に外部から使用 ----
    XMFLOAT3 GetWorldPos()   const { return m_worldPos; }
    XMFLOAT3 GetWorldSize()  const { return m_worldSize; }
    XMFLOAT3 GetWorldAngle() const { return m_worldAngle; }
    CollisionType GetType() const { return m_type; }

    void SetLayer(int layer) { m_layer = layer; }
    void SetMask(int maskBit) { m_mask = maskBit; }

    int GetLayer() { return m_layer; }
    int GetMask() { return m_mask; }

    void SetCollisionType(CollisionType type) { m_type = type; }

private:
    // Offset（ユーザー指定）
    XMFLOAT3 m_offPos = { 0,0,0 };
    XMFLOAT3 m_offSize = { 1,1,1 };
    XMFLOAT3 m_offAngle = { 0,0,0 };

    // 判定用 World 値（外部関数が使用する）
    XMFLOAT3 m_worldPos = { 0,0,0 };
    XMFLOAT3 m_worldSize = { 1,1,1 };
    XMFLOAT3 m_worldAngle = { 0,0,0 };

    CollisionType m_type = CollisionType::CollisionBox;

    int m_layer = 0;
    int m_mask = 0xFFFF;    //相手レイヤーとのびっとマスク
};