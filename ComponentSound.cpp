#include "ComponentSound.h"
#include <algorithm>
#include <cmath>

// =========================
// ヘルパー関数
// =========================
static float Clamp(float v, float a, float b)
{
    return (v < a) ? a : (v > b) ? b : v;
}

// =========================
// 簡易3Dベクトル距離
// =========================
static float VecDistance(const XMFLOAT3& a, const XMFLOAT3& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

// =========================
// 反響データ
// =========================
struct EchoState
{
    bool Enable = false;
    float Strength = 0.25f;  // 反響度
    float DelaySec = 0.2f;   // 遅延時間
};

static EchoState g_Echo;

// =========================
// Sound クラス実装
// =========================

void Sound::Init()
{
    Mono = true;
    pos = { 0,0,0 };
    camPos = { 0,0,0 };
    camAng = { 0,0,0 };
    pan = 0.0f;


    // =========================
    // WAV 読み込みテスト
    // =========================
    // XAudio2 初期化
    static IXAudio2* xaudio = nullptr;
    if (!xaudio)
    {
        XAudio2Create(&xaudio, 0);
        xaudio->CreateMasteringVoice(&m_masterVoice);
    }


    // WAV ロード
    HANDLE hFile = CreateFileA("asset/001.wav", GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return;


    DWORD fileSize = GetFileSize(hFile, nullptr);
    BYTE* data = new BYTE[fileSize];
    DWORD readBytes;
    ReadFile(hFile, data, fileSize, &readBytes, nullptr);
    CloseHandle(hFile);


    // WAV ヘッダ解析
    WAVEFORMATEX* wf = reinterpret_cast<WAVEFORMATEX*>(data + 20);


    // 波形データ位置（簡易）
    BYTE* audioStart = data + 44;
    DWORD audioSize = fileSize - 44;


    // ソースボイス作成
    xaudio->CreateSourceVoice(&m_sourceVoice, wf);


    XAUDIO2_BUFFER buf = {};
    buf.AudioBytes = audioSize;
    buf.pAudioData = audioStart;
    buf.Flags = XAUDIO2_END_OF_STREAM;


    m_sourceVoice->SubmitSourceBuffer(&buf);
    m_sourceVoice->Start();


    m_wavData.reset(data);
}

void Sound::Update()
{
    // 3D計算（簡易）
    if (Mono)
    {
        // モノラル時はパンをカメラ中心（0）へ寄せる
        pan = 0.0f;
    }
    else
    {
        // 左右方向 : カメラ→音源ベクトルのx成分をパンとして扱う
        float dx = pos.x - camPos.x;
        float dz = pos.z - camPos.z;

        // カメラの向いている方向へ回転補正
        float yaw = camAng.y;
        float cx = cosf(yaw);
        float sx = sinf(yaw);

        // ローカル座標に変換
        float lx = dx * cx - dz * sx;
        float lz = dx * sx + dz * cx;

        // パンは -1〜1 に丸める
        pan = Clamp(lx * 0.2f, -1.0f, 1.0f);
    }

    // パンや距離減衰を実サウンドへ適用する処理をここに追加
}

void Sound::Draw()
{
    
}

void Sound::Release()
{
}

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
    pan = Clamp(p, -1.0f, 1.0f);
}

void Sound::SetCameraPos(float x, float y, float z)
{
    camPos = { x,y,z };
}

void Sound::SetCameraAngle(float x, float y, float z)
{
    camAng = { x,y,z };
}

// =========================
// 反響（簡易エコー）設定
// =========================
void Sound::SetEcho(bool enable, float strength, float delaySec)
{
    g_Echo.Enable = enable;
    g_Echo.Strength = strength;
    g_Echo.DelaySec = delaySec;

    // 実際のエフェクト適用処理（XAudio2 FX など）は
    // オーディオチャンネルごとの管理側に追加
}
