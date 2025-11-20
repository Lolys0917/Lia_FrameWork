#pragma once
#include "Manager.h"
#include "Component.h"
#include <xaudio2.h>

using namespace DirectX;

class Speaker : public Component
{
public:
	using Component::Component;

	void Init()override;
	void Update()override;
	void Draw()override;
	void Release()override;

private:
	XMFLOAT3 pos;
	float pan;
};