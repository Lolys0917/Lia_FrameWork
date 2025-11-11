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

#include "Manager.h"
#include "Main.h"

#include <vector>
#include <wincodec.h> // WIC

//グローバル
std::vector<ID3D11ShaderResourceView*> g_textureSRV;
KeyMap TextureMap;

#define SafeRelease(p) if(p){ (p)->Release(); (p)=nullptr; }

bool LoadTexture(const char* filename)
{
    // --- 既にロード済みならスキップ ---
    int mapSize = KeyMap_GetSize(&TextureMap);
    for (int i = 0; i < mapSize; i++) {
        const char* key = KeyMap_GetKey(&TextureMap, i);
        if (strcmp(key, filename) == 0) return true;
    }

    int TextureIndex = KeyMap_Add(&TextureMap, filename);

    IWICImagingFactory* pWIC = nullptr;
    IWICBitmapDecoder* pDecoder = nullptr;
    IWICBitmapFrameDecode* pFrame = nullptr;
    IWICFormatConverter* pConverter = nullptr;
    bool calledCoInit = false;

    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pWIC));

    // --- COMが未初期化なら初期化して再試行 ---
    if (FAILED(hr)) {
        HRESULT hrInit = CoInitialize(nullptr);
        if (SUCCEEDED(hrInit)) calledCoInit = true;

        hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&pWIC));

        if (FAILED(hr)) {
            if (calledCoInit) CoUninitialize();
            return false;
        }
    }

    // --- 画像読み込み ---
    hr = pWIC->CreateDecoderFromFilename(
        ConvertToWString(filename).c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &pDecoder);

    if (FAILED(hr)) {
        SafeRelease(pWIC);
        if (calledCoInit) CoUninitialize();
        return false;
    }

    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr)) {
        SafeRelease(pDecoder);
        SafeRelease(pWIC);
        if (calledCoInit) CoUninitialize();
        return false;
    }

    hr = pWIC->CreateFormatConverter(&pConverter);
    if (FAILED(hr)) {
        SafeRelease(pFrame);
        SafeRelease(pDecoder);
        SafeRelease(pWIC);
        if (calledCoInit) CoUninitialize();
        return false;
    }

    hr = pConverter->Initialize(
        pFrame,
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr, 0.0, WICBitmapPaletteTypeCustom);

    if (FAILED(hr)) {
        SafeRelease(pConverter);
        SafeRelease(pFrame);
        SafeRelease(pDecoder);
        SafeRelease(pWIC);
        if (calledCoInit) CoUninitialize();
        return false;
    }

    // --- ピクセルデータ取得 ---
    UINT width, height;
    pConverter->GetSize(&width, &height);
    std::vector<BYTE> pixels(width * height * 4);
    pConverter->CopyPixels(nullptr, width * 4, (UINT)pixels.size(), pixels.data());

    // --- DirectXテクスチャ作成 ---
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
    if (FAILED(hr) || !texture) {
        MessageBoxA(nullptr, "CreateTexture2D failed in LoadTexture()", "Error", MB_OK);
        SafeRelease(pConverter);
        SafeRelease(pFrame);
        SafeRelease(pDecoder);
        SafeRelease(pWIC);
        if (calledCoInit) CoUninitialize();
        return false;
    }

    ID3D11ShaderResourceView* srv = nullptr;
    hr = GetDevice()->CreateShaderResourceView(texture, nullptr, &srv);
    if (FAILED(hr)) {
        MessageBoxA(nullptr, "CreateShaderResourceView failed in LoadTexture()", "Error", MB_OK);
        SafeRelease(texture);
        SafeRelease(pConverter);
        SafeRelease(pFrame);
        SafeRelease(pDecoder);
        SafeRelease(pWIC);
        if (calledCoInit) CoUninitialize();
        return false;
    }

    // --- 保存 ---
    g_textureSRV.push_back(srv);
    SafeRelease(texture);
    SafeRelease(pConverter);
    SafeRelease(pFrame);
    SafeRelease(pDecoder);
    SafeRelease(pWIC);
    if (calledCoInit) CoUninitialize();

    return true;
}