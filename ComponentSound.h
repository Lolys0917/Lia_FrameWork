#pragma once
#include "Manager.h"
#include "Component.h"
#include <xaudio2.h>
#include <x3daudio.h>
#include <memory>
#include <DirectXMath.h>

using namespace DirectX;

class Sound : public Component
{
public:
    using Component::Component;

    void Init() override;
    void Update() override;
    void Draw() override;
    void Release() override;

    void SetMono(bool mono);
    void SetPos(float x, float y, float z);
    void SetPan(float pan);
    void SetCameraPos(float x, float y, float z);
    void SetCameraAngle(float x, float y, float z);

    // 反響（エコー）設定
    void SetEcho(bool enable, float strength, float delaySec);

private:
    bool Mono = true;
    XMFLOAT3 pos{ 0,0,0 };
    XMFLOAT3 camPos{ 0,0,0 };
    XMFLOAT3 camAng{ 0,0,0 };
    float pan = 0.0f;

    // XAudio2
    IXAudio2MasteringVoice* m_masterVoice = nullptr;
    IXAudio2SourceVoice* m_sourceVoice = nullptr;

    // WAV データ保持
    std::unique_ptr<unsigned char[]> m_wavData;
};