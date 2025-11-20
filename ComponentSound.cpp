#include "ComponentSound.h"
#include <iostream>
#include <algorithm>

// --- 外部（Manager など）から取得する前提のオブジェクト ---
extern IXAudio2* g_xaudio;
extern IXAudio2MasteringVoice* g_master;
extern X3DAUDIO_HANDLE g_x3dHandle;

// スピーカーの数（例：ステレオ）
static const UINT32 g_speakerCount = 2;

// --- 初期化 ---
void Sound::Init()
{
    Mono = false;
    pos = { 0,0,0 };
    camPos = { 0,0,0 };
    camAng = { 0,0,0 };
    pan = 0.0f;

    // WAV 読み込みや SourceVoice 生成などは省略
    // sourceVoice などのメンバがある前提

    // Listener と Emitter の初期化
    ZeroMemory(&listener, sizeof(listener));
    ZeroMemory(&emitter, sizeof(emitter));

    listener.OrientFront = XMFLOAT3(0, 0, 1);
    listener.OrientTop = XMFLOAT3(0, 1, 0);

    emitter.ChannelCount = 1;  // モノラル音源
    emitter.CurveDistanceScaler = 1.0f;
    emitter.DopplerScaler = 1.0f;

    dspSettings.SrcChannelCount = 1;
    dspSettings.DstChannelCount = g_speakerCount;
    dspSettings.pMatrixCoefficients = matrixCoefficients;
}

// --- 更新 ---
void Sound::Update()
{
    // Listener を更新（カメラ）
    listener.Position = camPos;
    listener.OrientFront = XMFLOAT3(
        cosf(camAng.y), 0, sinf(camAng.y)
    );
    listener.OrientTop = XMFLOAT3(0, 1, 0);

    if (Mono)
    {
        // カメラと同位置にする → モノラルになる
        emitter.Position = camPos;

        // --------------------
        // ★ パン処理 (Mono 時)
        // --------------------
        float left = (pan <= 0) ? 1.0f : 1.0f - pan;
        float right = (pan >= 0) ? 1.0f : 1.0f + pan;

        matrixCoefficients[0] = left;
        matrixCoefficients[1] = right;

        sourceVoice->SetOutputMatrix(
            g_master, 1, g_speakerCount, matrixCoefficients
        );
    }
    else
    {
        // --------------------
        // ★ Stereo 3D 音源
        // --------------------
        emitter.Position = pos;

        X3DAudioCalculate(
            g_x3dHandle,
            &listener,
            &emitter,
            X3DAUDIO_CALCULATE_MATRIX |
            X3DAUDIO_CALCULATE_DOPPLER |
            X3DAUDIO_CALCULATE_LPF_DIRECT,
            &dspSettings
        );

        // パンの補正を掛ける（Mono でも Stereo でも同じ式）
        float leftPan = (pan <= 0) ? 1.0f : 1.0f - pan;
        float rightPan = (pan >= 0) ? 1.0f : 1.0f + pan;

        dspSettings.pMatrixCoefficients[0] *= leftPan;
        dspSettings.pMatrixCoefficients[1] *= rightPan;

        // 計算結果を適用
        sourceVoice->SetFrequencyRatio(dspSettings.DopplerFactor);
        sourceVoice->SetOutputMatrix(
            g_master, 1, g_speakerCount, dspSettings.pMatrixCoefficients
        );
    }
}

void Sound::Draw()
{

}

void Sound::Release()
{
    if (sourceVoice)
    {
        sourceVoice->Stop();
        sourceVoice->DestroyVoice();
        sourceVoice = nullptr;
    }1
}

// --- Setter 群 ---
void Sound::SetMono(bool mono)
{
    Mono = mono;
}

void Sound::SetPos(float x, float y, float z)
{
    pos = { x,y,z };
}

void Sound::SetPan(float p)
{
    pan = std::clamp(p, -1.0f, 1.0f);
}

void Sound::SetCameraPos(float x, float y, float z)
{
    camPos = { x,y,z };
}

void Sound::SetCameraAngle(float x, float y, float z)
{
    camAng = { x,y,z };
}
