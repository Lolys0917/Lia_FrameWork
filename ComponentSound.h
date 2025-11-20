#pragma once
#include "Manager.h"
#include "Component.h"
#include <xaudio2.h>
#include <x3daudio.h>

using namespace DirectX;

class Sound : public Component
{
public:
	using Component::Component;

	void Init()override;
	void Update()override;
	void Draw()override;
	void Release()override;

	void SetMono(bool mono);
	void SetPos(float x, float y, float z);
	void SetPan(float pan);
	void SetCameraPos(float x, float y,  float z);
	void SetCameraAngle(float x, float y,  float z);
private:
	bool Mono;
	XMFLOAT3 pos;
	XMFLOAT3 camPos;
	XMFLOAT3 camAng;
	float pan;

	IXAudio2SourceVoice* sourceVoice = nullptr;

	X3DAUDIO_LISTENER listener;
	X3DAUDIO_EMITTER emitter;
	X3DAUDIO_DSP_SETTINGS dspSettings;

	float matrixCoefficients[2];
};