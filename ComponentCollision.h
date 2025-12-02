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

    // ---- Offset 設定 ----
    void SetBasePos(float x, float y, float z) { m_basePos = { x,y,z }; }
    void SetBaseSize(float x, float y, float z) { m_baseSize = { x,y,z }; }
    void SetBaseAngle(float x, float y, float z) { m_baseAngle = { x,y,z }; }
    void SetOffsetPos(float x, float y, float z) { m_offPos = { x,y,z }; }
    void SetOffsetSize(float x, float y, float z) { m_offSize = { x,y,z }; }
    void SetOffsetAngle(float x, float y, float z) { m_offAngle = { x,y,z }; }

    // ---- 判定用に外部から使用 ----
    XMFLOAT3 GetWorldPos()   const { return m_worldPos; }
    XMFLOAT3 GetWorldSize()  const { return m_worldSize; }
    XMFLOAT3 GetWorldAngle() const { return m_worldAngle; }
    CollisionType GetType() const { return m_type; }

    void SetCollisionType(CollisionType type) { m_type = type; }

private:

    // Base（親の値）
    XMFLOAT3 m_basePos = { 0,0,0 };
    XMFLOAT3 m_baseSize = { 1,1,1 };
    XMFLOAT3 m_baseAngle = { 0,0,0 };

    // Offset（ユーザー指定）
    XMFLOAT3 m_offPos = { 0,0,0 };
    XMFLOAT3 m_offSize = { 1,1,1 };
    XMFLOAT3 m_offAngle = { 0,0,0 };

    // 判定用 World 値（外部関数が使用する）
    XMFLOAT3 m_worldPos = { 0,0,0 };
    XMFLOAT3 m_worldSize = { 1,1,1 };
    XMFLOAT3 m_worldAngle = { 0,0,0 };

    CollisionType m_type = CollisionType::CollisionBox;
};