#include "ComponentSound.h"
#include <cassert>

// =====================================================
// static
// =====================================================
IXAudio2* Sound::s_XAudio = nullptr;
IXAudio2MasteringVoice* Sound::s_MasterVoice = nullptr;
bool Sound::s_AudioInitialized = false;

// =====================================================
// Component
// =====================================================
void Sound::Init()
{
    // XAudio2 初期化（最初の Sound だけ）
    if (!s_AudioInitialized)
    {
        HRESULT hr = XAudio2Create(&s_XAudio, 0);
        assert(SUCCEEDED(hr));

        hr = s_XAudio->CreateMasteringVoice(&s_MasterVoice);
        assert(SUCCEEDED(hr));

        s_AudioInitialized = true;
    }
}

void Sound::Update()
{
    //再生終了検知など

    //
}

void Sound::Release()
{
    if (m_SourceVoice)
    {
        m_SourceVoice->DestroyVoice();
        m_SourceVoice = nullptr;
    }
}

// =====================================================
// Audio
// =====================================================
void Sound::SetAudioName(const char* name)
{
    m_WavData = GetWavData(name);
    assert(m_WavData && "Sound::SetAudioName : wav not found");

    // 既に Voice があれば破棄
    if (m_SourceVoice)
    {
        m_SourceVoice->DestroyVoice();
        m_SourceVoice = nullptr;
    }

    // SourceVoice 作成
    HRESULT hr = s_XAudio->CreateSourceVoice(
        &m_SourceVoice,
        &m_WavData->format
    );
    assert(SUCCEEDED(hr));
}

void Sound::Play()
{
    if (!m_SourceVoice || !m_WavData)
        return;

    XAUDIO2_BUFFER buffer{};
    buffer.pAudioData = m_WavData->buffer.data();
    buffer.AudioBytes = static_cast<UINT32>(m_WavData->buffer.size());
    buffer.Flags = XAUDIO2_END_OF_STREAM;

    m_SourceVoice->SubmitSourceBuffer(&buffer);
    m_SourceVoice->Start();
}
