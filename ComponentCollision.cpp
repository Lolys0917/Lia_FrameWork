#include "ComponentCollision.h"

void Collision::Update()
{
	m_worldPos.x = GetPosition().x + m_offPos.x;
	m_worldPos.y = GetPosition().y + m_offPos.y;
	m_worldPos.z = GetPosition().z + m_offPos.z;

	m_worldSize.x = GetSize().x + m_offSize.x;
	m_worldSize.y = GetSize().y + m_offSize.y;
	m_worldSize.z = GetSize().z + m_offSize.z;

	m_worldAngle.x = GetAngle().x + m_offAngle.x;
	m_worldAngle.y = GetAngle().y + m_offAngle.y;
	m_worldAngle.z = GetAngle().z + m_offAngle.z;
}