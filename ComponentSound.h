#pragma once
#include "Component.h"
#include "Manager.h"
#include <xaudio2.h>

struct WavData;

class Sound : public Component
{
public:
    using Component::Component;

    // Component
    void Init() override;
    void Update() override;
    void Release() override;

    // Audio
    void SetAudioName(const char* name);
    void Play();
	void Stop();
	void ReStart();
private:
    // shared audio systemÅiébíËÅj
    static IXAudio2* s_XAudio;
    static IXAudio2MasteringVoice* s_MasterVoice;
    static bool s_AudioInitialized;

    // per sound
    IXAudio2SourceVoice* m_SourceVoice = nullptr;
    const WavData* m_WavData = nullptr;

	bool m_IsPlaying = false;
	UINT32 m_StopOffsetBytes = 0;
};