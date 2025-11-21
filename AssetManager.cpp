// アセットの読込
// アセットの保存
// アセットの暗号化
// アセットの復号化
// アセットの管理
// を行うプログラム
//
// アセットはソフト出力時に自動でパッケージ化され、暗号化される。
// ファイルの重複を避けるため、同名ファイルは読み込めないものとする。
//
// アセットのパスは相対パスで指定する。
// アセットのメモリはプログラム終了時に自動で解放される。
// Manager.h経由でAssetManagerへアクセスし、
// アセット操作関数を呼び出す。それにより
// Manager.h経由でObjectManagerへアクセスし、
// アセットのメモリを取得することが出来る。
// __________________________________________

#include "Main.h"
#include "AssetLoad.h"
#include "Manager.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <wincodec.h> // WIC
#include <fstream>
#include <sstream>
#include <algorithm>

#define SafeRelease(p) if(p){ (p)->Release(); (p)=nullptr; }

//グローバル_____________________
static std::vector<ID3D11ShaderResourceView*> g_textureSRV;        //テクスチャ保存用SRV
static std::vector<std::vector<ModelVertex>> g_modelVertex;        //Obj保存用SRV
static ID3D11SamplerState* g_samplerState;                         //デフォルトサンプラーステート
//キーマップ
static KeyMap TextureMap;
static KeyMap ModelMap;

ID3D11ShaderResourceView* GetTextureSRV(const char* filename)
{
    if (!filename) return nullptr;
    // すでに登録済みならそのSRVを返す
    int index = KeyMap_GetIndex(&TextureMap, filename);
    if (index >= 0 && index < (int)g_textureSRV.size()) {
        return g_textureSRV[index];
    }

    // pkgから読み込み
    if (!AL_LoadFromPackageByName(filename)) {
        MessageBoxA(nullptr, ("Texture not found: " + std::string(filename)).c_str(), "AssetManager", MB_OK);
        return nullptr;
    }

    // KeyMapが更新されているはずなので再取得
    index = KeyMap_GetIndex(&TextureMap, filename);
    if (index < 0 || index >= (int)g_textureSRV.size()) {
        MessageBoxA(nullptr, "GetOrLoadTextureSRV: invalid index after load", "Error", MB_OK);
        return nullptr;
    }

    return g_textureSRV[index];
}
// ================================================================
// FBX / OBJ 取得
// ================================================================
const std::vector<ModelVertex>* GetModelVertices(const char* name)
{
    int index = KeyMap_GetIndex(&ModelMap, name);
    if (index < 0 || index >= (int)g_modelVertex.size()) return nullptr;
    return &g_modelVertex[index];
}


// ================================================================
// Texture メモリロード
// ================================================================
bool IN_LoadTexture_Memory(const char* name, const unsigned char* data, size_t size)
{
    if (!data || size == 0) return false;

    if (!GetDevice())
    {
        MessageBoxA(nullptr, "Device is NULL in IN_LoadTexture_Memory", "Error", MB_OK);
        return false;
    }

    int TextureIndex = KeyMap_Add(&TextureMap, name);
    if ((int)g_textureSRV.size() <= TextureIndex)
        g_textureSRV.resize(TextureIndex + 1, nullptr);

    IWICImagingFactory* pWIC = nullptr;
    IWICStream* pStream = nullptr;
    IWICBitmapDecoder* pDecoder = nullptr;
    IWICBitmapFrameDecode* pFrame = nullptr;
    IWICFormatConverter* pConverter = nullptr;
    bool calledCoInit = false;

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWIC));
    if (FAILED(hr)) {
        HRESULT hrInit = CoInitialize(nullptr);
        if (SUCCEEDED(hrInit)) calledCoInit = true;
        hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWIC));
        if (FAILED(hr)) {
            if (calledCoInit) CoUninitialize();
            return false;
        }
    }

    hr = pWIC->CreateStream(&pStream);
    if (FAILED(hr)) return false;
    hr = pStream->InitializeFromMemory((WICInProcPointer)data, (DWORD)size);
    if (FAILED(hr)) return false;

    hr = pWIC->CreateDecoderFromStream(pStream, nullptr, WICDecodeMetadataCacheOnLoad, &pDecoder);
    if (FAILED(hr)) return false;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr)) return false;

    hr = pWIC->CreateFormatConverter(&pConverter);
    if (FAILED(hr)) return false;
    hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return false;

    UINT width, height;
    pConverter->GetSize(&width, &height);
    std::vector<BYTE> pixels(width * height * 4);
    pConverter->CopyPixels(nullptr, width * 4, (UINT)pixels.size(), pixels.data());

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pixels.data();
    initData.SysMemPitch = width * 4;

    ID3D11Texture2D* texture = nullptr;
    hr = GetDevice()->CreateTexture2D(&desc, &initData, &texture);
    if (FAILED(hr)) return false;

    ID3D11ShaderResourceView* srv = nullptr;
    hr = GetDevice()->CreateShaderResourceView(texture, nullptr, &srv);
    if (FAILED(hr)) { texture->Release(); return false; }

    g_textureSRV[TextureIndex] = srv;

    SafeRelease(texture);
    SafeRelease(pConverter);
    SafeRelease(pFrame);
    SafeRelease(pDecoder);
    SafeRelease(pStream);
    SafeRelease(pWIC);
    if (calledCoInit) CoUninitialize();

    return true;
}

// ================================================================
// Obj / FBX メモリロード（Assimp利用）
// ================================================================
static bool LoadModel_Assimp_FromMemory(const char* name, const unsigned char* data, size_t size, bool isFBX)
{
    if (!data || size == 0) return false;

    // 既にロード済みか？
    int mapSize = KeyMap_GetSize(&ModelMap);
    for (int i = 0; i < mapSize; ++i) {
        const char* key = KeyMap_GetKey(&ModelMap, i);
        if (strcmp(key, name) == 0) return true;
    }

    int ModelIndex = KeyMap_Add(&ModelMap, name);
    if ((int)g_modelVertex.size() <= ModelIndex)
        g_modelVertex.resize(ModelIndex + 1);

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFileFromMemory(
        data,
        size,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ConvertToLeftHanded,
        isFBX ? "fbx" : "obj"
    );

    if (!scene || !scene->HasMeshes()) {
        std::string err = importer.GetErrorString();
        MessageBoxA(nullptr, ("Assimp: " + err).c_str(), "LoadModel_Memory Error", MB_OK);
        return false;
    }

    std::vector<ModelVertex> outVerts;

    for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi)
    {
        aiMesh* mesh = scene->mMeshes[mi];
        if (!mesh) continue;

        bool hasNormals = mesh->HasNormals();
        bool hasTexCoords = mesh->HasTextureCoords(0);

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
        {
            aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices < 3) continue;
            for (unsigned int j = 0; j < face.mNumIndices; ++j)
            {
                unsigned int vi = face.mIndices[j];
                ModelVertex v;
                v.pos = DirectX::XMFLOAT3(mesh->mVertices[vi].x, mesh->mVertices[vi].y, mesh->mVertices[vi].z);
                v.normal = hasNormals ? DirectX::XMFLOAT3(mesh->mNormals[vi].x, mesh->mNormals[vi].y, mesh->mNormals[vi].z)
                    : DirectX::XMFLOAT3(0, 0, 0);
                if (hasTexCoords)
                    v.uv = DirectX::XMFLOAT2(mesh->mTextureCoords[0][vi].x, mesh->mTextureCoords[0][vi].y);
                else
                    v.uv = DirectX::XMFLOAT2(0, 0);
                outVerts.push_back(v);
            }
        }
    }

    g_modelVertex[ModelIndex] = std::move(outVerts);
    return true;
}

// ================================================================
// FBX / OBJ wrapper
// ================================================================
bool IN_LoadFBX_Memory(const char* name, const unsigned char* data, size_t size)
{
    return LoadModel_Assimp_FromMemory(name, data, size, true);
}

bool IN_LoadModelObj_Memory(const char* name, const unsigned char* data, size_t size)
{
    return LoadModel_Assimp_FromMemory(name, data, size, false);
}

// ================================================================
// WAV メモリロード
// ================================================================
struct WavData {
    std::vector<BYTE> buffer;
    WAVEFORMATEX format = {};
};

static std::vector<WavData> g_wavData;
static KeyMap WavMap;

const WavData* GetWavData(const char* name)
{
    int index = KeyMap_GetIndex(&WavMap, name);
    if (index < 0 || index >= (int)g_wavData.size()) return nullptr;
    return &g_wavData[index];
}

bool IN_LoadWav_Memory(const char* name, const unsigned char* data, size_t size)
{
    if (!data || size == 0) return false;

    int WavIndex = KeyMap_Add(&WavMap, name);
    if ((int)g_wavData.size() <= WavIndex)
        g_wavData.resize(WavIndex + 1);

    const BYTE* ptr = data;
    // RIFFチャンク確認
    if (size < 44 || strncmp((const char*)ptr, "RIFF", 4) != 0 || strncmp((const char*)(ptr + 8), "WAVE", 4) != 0)
        return false;

    const BYTE* fmtChunk = nullptr;
    const BYTE* dataChunk = nullptr;
    size_t dataSize = 0;

    size_t pos = 12;
    while (pos + 8 < size) {
        const char* chunkId = (const char*)(ptr + pos);
        uint32_t chunkSize = *(uint32_t*)(ptr + pos + 4);
        if (strncmp(chunkId, "fmt ", 4) == 0) fmtChunk = ptr + pos + 8;
        if (strncmp(chunkId, "data", 4) == 0) { dataChunk = ptr + pos + 8; dataSize = chunkSize; }
        pos += 8 + chunkSize;
    }

    if (!fmtChunk || !dataChunk) return false;

    WAVEFORMATEX fmt = {};
    fmt.wFormatTag = *(uint16_t*)(fmtChunk + 0);
    fmt.nChannels = *(uint16_t*)(fmtChunk + 2);
    fmt.nSamplesPerSec = *(uint32_t*)(fmtChunk + 4);
    fmt.nAvgBytesPerSec = *(uint32_t*)(fmtChunk + 8);
    fmt.nBlockAlign = *(uint16_t*)(fmtChunk + 12);
    fmt.wBitsPerSample = *(uint16_t*)(fmtChunk + 14);

    g_wavData[WavIndex].buffer.assign(dataChunk, dataChunk + dataSize);
    g_wavData[WavIndex].format = fmt;

    return true;
}
