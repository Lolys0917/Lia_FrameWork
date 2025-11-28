#include "ComponentCollision.h"

void Collision::Update()
{
	m_worldPos.x = m_basePos.x + m_offPos.x;
	m_worldPos.y = m_basePos.y + m_offPos.y;
	m_worldPos.z = m_basePos.z + m_offPos.z;

	m_worldSize.x = m_baseSize.x + m_offSize.x;
	m_worldSize.y = m_baseSize.y + m_offSize.y;
	m_worldSize.z = m_baseSize.z + m_offSize.z;

	m_worldAngle.x = m_baseAngle.x + m_offAngle.x;
	m_worldAngle.y = m_baseAngle.y + m_offAngle.y;
	m_worldAngle.z = m_baseAngle.z + m_offAngle.z;
}