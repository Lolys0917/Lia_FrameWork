// AssetManager.cpp
// Re-generated AssetManager with PSD / fallback texture resolution and package system.
// Keep function signatures intact as declared in Manager.h

#include "Manager.h" // KeyMap 関連、API定義
#include "Main.h"    // GetDevice(), GetContext(), AddMessage() など既存のユーティリティ
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <wincodec.h> // WIC
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <mutex>
#include <vector>
#include <map>

#pragma comment(lib, "windowscodecs.lib")

using namespace DirectX;
namespace fs = std::filesystem;

#define SafeRelease(p) if(p){ (p)->Release(); (p) = nullptr; }

// ------------------------------
// 内部データ構造
// ------------------------------
struct PackageEntry;
struct Package;

static std::mutex g_packageMutex;
static std::vector<Package> g_packages;

// グローバルアセット格納（既存コードベースを踏襲）
static std::vector<ID3D11ShaderResourceView*> g_textureSRV;        // texture SRV per KeyMap index (TextureMap)
static std::vector<std::vector<ModelVertex>> g_modelVertex;        // models (per ModelMap index)
static std::vector<std::vector<unsigned int>> g_modelIndices;      // corresponding indices
static std::vector<ID3D11ShaderResourceView*> g_modelTextureSRV;  // model-level diffuse texture (if any)
static std::vector<std::vector<std::string>> g_modelMaterialNames; // material texture names for reference
static std::vector<std::vector<ModelSubmeshInfo>> g_modelSubmeshes; // per model index

static ID3D11SamplerState* g_samplerState = nullptr;

// キーマップ（名前->index）
static KeyMap TextureMap;
static KeyMap ModelMap;
static KeyMap WavMap;

//モデルUVデバッグ用
static float g_DebugUVShiftU = 0.0f;
static float g_DebugUVShiftV = 0.0f;
static float g_DebugUVScaleU = 1.0f;
static float g_DebugUVScaleV = 1.0f;
static bool  g_DebugUVFlipU = false;
static bool  g_DebugUVFlipV = false;
// ------------------------------
// helpers
// ------------------------------
static std::string ToLowerExt(const std::string& s) {
    std::string e = s;
    for (auto& c : e) c = (char)tolower(c);
    return e;
}
static std::string sanitizeExt(const std::string& ext) {
    std::string e = ext;
    if (!e.empty() && e[0] == '.') e.erase(0, 1);
    for (auto& c : e) c = (char)tolower(c);
    return e;
}
static Package* FindPackageByExt(const std::string& ext) {
    for (auto& p : g_packages) {
        if (p.ext == ext) return &p;
    }
    return nullptr;
}

static void DebugPrintAssimpAnimations(const aiScene* scene)
{
    if (!scene) return;

    if (scene->mNumAnimations == 0)
    {
        MessageBoxA(NULL, "FBX内にアニメーションはありません", "AnimationCheck", MB_OK);
        return;
    }

    std::string msg = "アニメーション数: " + std::to_string(scene->mNumAnimations) + "\n\n";

    for (unsigned int i = 0; i < scene->mNumAnimations; ++i)
    {
        const aiAnimation* anim = scene->mAnimations[i];
        if (!anim) continue;

        msg += "Animation[" + std::to_string(i) + "]\n";
        msg += "  Name: " + std::string(anim->mName.C_Str()) + "\n";
        msg += "  Duration: " + std::to_string(anim->mDuration) + "\n";
        msg += "  TicksPerSecond: " + std::to_string(anim->mTicksPerSecond) + "\n";
        msg += "  Channels: " + std::to_string(anim->mNumChannels) + "\n";
        msg += "\n";
    }

    MessageBoxA(NULL, msg.c_str(), "AnimationCheck", MB_OK);
}

// find package+index by name among loaded packages
static bool FindPackageEntryByName(const std::string& name, Package*& outPkg, int& outIndex) {
    std::lock_guard<std::mutex> lg(g_packageMutex);
    for (auto& p : g_packages) {
        int idx = KeyMap_GetIndex(&p.keymap, name.c_str());
        if (idx >= 0) { outPkg = &p; outIndex = idx; return true; }
    }
    outPkg = nullptr; outIndex = -1; return false;
}

// create directory if needed
static bool EnsureDirectoryExists(const std::string& path) {
    try {
        fs::path p(path);
        if (fs::exists(p)) return true;
        return fs::create_directories(p);
    }
    catch (...) {
        return false;
    }
}

// Make a relative key from assetRoot to file (forward slashes)
static std::string MakeRelativeKey(const fs::path& assetRoot, const fs::path& filePath) {
    try {
        fs::path absRoot = fs::weakly_canonical(assetRoot);
        fs::path absFile = fs::weakly_canonical(filePath);
        std::string rel = fs::relative(absFile, absRoot).generic_string();
        return rel;
    }
    catch (...) {
        return filePath.filename().generic_string();
    }
}

// ------------------------------
// WIC -> SRV loader
// ------------------------------
bool IN_LoadTexture_Memory(const char* name, const unsigned char* data, size_t size)
{
    if (!data || size == 0) return false;
    if (!GetDevice()) { AddMessage("IN_LoadTexture_Memory: device is null"); return false; }

    int texIndex = KeyMap_Add(&TextureMap, name);
    if ((int)g_textureSRV.size() <= texIndex) g_textureSRV.resize(texIndex + 1, nullptr);

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
            AddMessage("IN_LoadTexture_Memory: Create WIC factory failed");
            return false;
        }
    }

    hr = pWIC->CreateStream(&pStream);
    if (FAILED(hr)) { AddMessage("IN_LoadTexture_Memory: CreateStream failed"); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }
    hr = pStream->InitializeFromMemory((WICInProcPointer)data, (DWORD)size);
    if (FAILED(hr)) { AddMessage("IN_LoadTexture_Memory: Init stream failed"); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }

    hr = pWIC->CreateDecoderFromStream(pStream, nullptr, WICDecodeMetadataCacheOnLoad, &pDecoder);
    if (FAILED(hr)) { AddMessage("IN_LoadTexture_Memory: CreateDecoderFromStream failed"); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr)) { AddMessage("IN_LoadTexture_Memory: GetFrame failed"); SafeRelease(pDecoder); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }

    hr = pWIC->CreateFormatConverter(&pConverter);
    if (FAILED(hr)) { AddMessage("IN_LoadTexture_Memory: CreateFormatConverter failed"); SafeRelease(pFrame); SafeRelease(pDecoder); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }
    hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) { AddMessage("IN_LoadTexture_Memory: Converter Init failed"); SafeRelease(pConverter); SafeRelease(pFrame); SafeRelease(pDecoder); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }

    UINT width = 0, height = 0;
    pConverter->GetSize(&width, &height);
    if (width == 0 || height == 0) { AddMessage("IN_LoadTexture_Memory: invalid image size"); SafeRelease(pConverter); SafeRelease(pFrame); SafeRelease(pDecoder); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }

    std::vector<BYTE> pixels((size_t)width * (size_t)height * 4);
    hr = pConverter->CopyPixels(nullptr, width * 4, (UINT)pixels.size(), pixels.data());
    if (FAILED(hr)) { AddMessage("IN_LoadTexture_Memory: CopyPixels failed"); SafeRelease(pConverter); SafeRelease(pFrame); SafeRelease(pDecoder); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    // sRGB support may be desired; use SRGB for color textures (keeps compatible with sRGB sampling).
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = pixels.data();
    init.SysMemPitch = width * 4;

    ID3D11Texture2D* tex = nullptr;
    hr = GetDevice()->CreateTexture2D(&desc, &init, &tex);
    if (FAILED(hr)) { AddMessage("IN_LoadTexture_Memory: CreateTexture2D failed"); SafeRelease(pConverter); SafeRelease(pFrame); SafeRelease(pDecoder); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }

    ID3D11ShaderResourceView* srv = nullptr;
    hr = GetDevice()->CreateShaderResourceView(tex, nullptr, &srv);
    if (FAILED(hr)) { AddMessage("IN_LoadTexture_Memory: CreateSRV failed"); SafeRelease(tex); SafeRelease(pConverter); SafeRelease(pFrame); SafeRelease(pDecoder); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }

    g_textureSRV[texIndex] = srv;

    SafeRelease(tex);
    SafeRelease(pConverter);
    SafeRelease(pFrame);
    SafeRelease(pDecoder);
    SafeRelease(pStream);
    SafeRelease(pWIC);
    if (calledCoInit) CoUninitialize();

    return true;
}

bool IN_LoadPSD_Memory(const char* name, const unsigned char* data, size_t size)
{
    // Reuse the same logic as IN_LoadTexture_Memory but keep it separate for clarity.
    if (!data || size == 0) return false;
    if (!GetDevice()) { AddMessage("IN_LoadPSD_Memory: device is null"); return false; }

    int texIndex = KeyMap_Add(&TextureMap, name);
    if ((int)g_textureSRV.size() <= texIndex) g_textureSRV.resize(texIndex + 1, nullptr);

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
            AddMessage("IN_LoadPSD_Memory: Create WIC factory failed");
            return false;
        }
    }

    hr = pWIC->CreateStream(&pStream);
    if (FAILED(hr)) { AddMessage("IN_LoadPSD_Memory: CreateStream failed"); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }
    hr = pStream->InitializeFromMemory((WICInProcPointer)data, (DWORD)size);
    if (FAILED(hr)) { AddMessage("IN_LoadPSD_Memory: Init stream failed"); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }

    hr = pWIC->CreateDecoderFromStream(pStream, nullptr, WICDecodeMetadataCacheOnLoad, &pDecoder);
    if (FAILED(hr)) { AddMessage("IN_LoadPSD_Memory: CreateDecoderFromStream failed (PSD codec may be missing)"); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr)) { AddMessage("IN_LoadPSD_Memory: GetFrame failed"); SafeRelease(pDecoder); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }

    hr = pWIC->CreateFormatConverter(&pConverter);
    if (FAILED(hr)) { AddMessage("IN_LoadPSD_Memory: CreateFormatConverter failed"); SafeRelease(pFrame); SafeRelease(pDecoder); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }
    hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) { AddMessage("IN_LoadPSD_Memory: Converter Init failed (PSD decode may not be supported)"); SafeRelease(pConverter); SafeRelease(pFrame); SafeRelease(pDecoder); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }

    UINT width = 0, height = 0;
    pConverter->GetSize(&width, &height);
    if (width == 0 || height == 0) { AddMessage("IN_LoadPSD_Memory: invalid image size"); SafeRelease(pConverter); SafeRelease(pFrame); SafeRelease(pDecoder); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }

    std::vector<BYTE> pixels((size_t)width * (size_t)height * 4);
    hr = pConverter->CopyPixels(nullptr, width * 4, (UINT)pixels.size(), pixels.data());
    if (FAILED(hr)) { AddMessage("IN_LoadPSD_Memory: CopyPixels failed"); SafeRelease(pConverter); SafeRelease(pFrame); SafeRelease(pDecoder); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // sRGB if wanted
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = pixels.data();
    init.SysMemPitch = width * 4;

    ID3D11Texture2D* tex = nullptr;
    hr = GetDevice()->CreateTexture2D(&desc, &init, &tex);
    if (FAILED(hr)) { AddMessage("IN_LoadPSD_Memory: CreateTexture2D failed"); SafeRelease(pConverter); SafeRelease(pFrame); SafeRelease(pDecoder); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }

    ID3D11ShaderResourceView* srv = nullptr;
    hr = GetDevice()->CreateShaderResourceView(tex, nullptr, &srv);
    if (FAILED(hr)) { AddMessage("IN_LoadPSD_Memory: CreateSRV failed"); SafeRelease(tex); SafeRelease(pConverter); SafeRelease(pFrame); SafeRelease(pDecoder); SafeRelease(pStream); SafeRelease(pWIC); if (calledCoInit) CoUninitialize(); return false; }

    g_textureSRV[texIndex] = srv;

    SafeRelease(tex);
    SafeRelease(pConverter);
    SafeRelease(pFrame);
    SafeRelease(pDecoder);
    SafeRelease(pStream);
    SafeRelease(pWIC);
    if (calledCoInit) CoUninitialize();

    // Register alias under basename as IN_LoadTexture_Memory does (optional)
    try {
        fs::path key(name ? name : "");
        std::string basename = key.filename().string();
        if (!basename.empty() && basename != (name ? name : "")) {
            int baseIdx = KeyMap_GetIndex(&TextureMap, basename.c_str());
            if (baseIdx < 0) {
                int aliasIdx = KeyMap_Add(&TextureMap, basename.c_str());
                if ((int)g_textureSRV.size() <= aliasIdx) g_textureSRV.resize(aliasIdx + 1, nullptr);
                g_textureSRV[aliasIdx] = srv;
            }
        }
    }
    catch (...) { /* ignore */ }

    return true;
}

// ------------------------------
// Assimp-based model loader (memory)
// - outputs expanded vertex list (triangle list) and indices are sequential 0..N-1
// - extracts diffuse texture name (if present) and attempts to load it via TryResolveAndLoadTextureSRV / registration
// ------------------------------
static bool LoadModel_Assimp_FromMemory(const char* name, const unsigned char* data, size_t size, bool isFBX)
{
    if (!data || size == 0) return false;

    // check already loaded (legacy behavior)
    int mapSize = KeyMap_GetSize(&ModelMap);
    for (int i = 0; i < mapSize; ++i) {
        const char* key = KeyMap_GetKey(&ModelMap, i);
        if (key && strcmp(key, name) == 0) return true; // already loaded
    }

    int modelIndex = KeyMap_Add(&ModelMap, name);
    if ((int)g_modelSubmeshes.size() <= modelIndex) {
        g_modelSubmeshes.resize(modelIndex + 1);
    }
    // ensure other containers are at least consistent for compatibility
    if ((int)g_modelVertex.size() <= modelIndex) {
        g_modelVertex.resize(modelIndex + 1);
        g_modelIndices.resize(modelIndex + 1);
        g_modelTextureSRV.resize(modelIndex + 1, nullptr);
        g_modelMaterialNames.resize(modelIndex + 1);
    }
    else {
        // clear previous content if any (safety)
        g_modelSubmeshes[modelIndex].clear();
        g_modelVertex[modelIndex].clear();
        g_modelIndices[modelIndex].clear();
        g_modelMaterialNames[modelIndex].clear();
        g_modelTextureSRV[modelIndex] = nullptr;
    }

    // Assimp importer
    Assimp::Importer importer;

    // Build postprocess flags: keep ConvertToLeftHanded as before (comment/adjust if needed).
    // NOTE: If left-handed conversion produced positional/rotation problems for you,
    // you can remove aiProcess_ConvertToLeftHanded here.
    unsigned int ppFlags =
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices;
    // For OBJ typical engines need V flipped; we'll flip UV v manually below for OBJ.
    // Optionally add aiProcess_FlipUVs for OBJ too, but we handle that explicitly.
    // Keep ConvertToLeftHanded to preserve earlier behavior; comment out if you want right-hand coords.
    ppFlags |= aiProcess_ConvertToLeftHanded;

    const aiScene* scene = importer.ReadFileFromMemory(
        data, size,
        ppFlags,
        isFBX ? "fbx" : "obj"
    );

	//デバッグ: アニメーション情報を表示
    DebugPrintAssimpAnimations(scene);

    if (!scene || !scene->HasMeshes()) {
        std::string err = importer.GetErrorString();
        AddMessage(("Assimp: " + err).c_str());
        return false;
    }

    // for resolving model-local files (search next to model), use name provided
    std::string modelPathStr = name ? name : std::string();

    // helper: load embedded texture if mat path references it (path like "*0")
    auto TryLoadEmbeddedTexture = [&](const aiScene* sc, const std::string& texPath)->ID3D11ShaderResourceView* {
        if (!sc) return nullptr;

        // "*番号" のとき embedded texture
        if (texPath.size() >= 1 && texPath[0] == '*') {

            // embedded texture index after '*'
            int idx = atoi(texPath.c_str() + 1);

            // Assimp 5.x では const aiTexture* しか返らない
            const aiTexture* at = sc->GetEmbeddedTexture(texPath.c_str() + 1);
            if (!at) return nullptr;

            std::string loadName =
                std::string(modelPathStr) + "#embedded" + std::to_string(idx);

            // ==========================
            // ① mHeight == 0 → PNG / JPG
            // ==========================
            if (at->mHeight == 0)
            {
                const unsigned char* d = (const unsigned char*)at->pcData;
                size_t s = (size_t)at->mWidth; // バイナリ長

                if (IN_LoadTexture_Memory(loadName.c_str(), d, s)) {
                    int tIdx = KeyMap_GetIndex(&TextureMap, loadName.c_str());
                    if (tIdx >= 0 && tIdx < (int)g_textureSRV.size())
                        return g_textureSRV[tIdx];
                }
            }
            else
            {
                // ==========================
                // ② mHeight > 0 → raw RGBA
                // ==========================

                size_t s = (size_t)(at->mWidth * at->mHeight * 4);
                const unsigned char* d = (const unsigned char*)at->pcData;

                if (IN_LoadTexture_Memory(loadName.c_str(), d, s)) {
                    int tIdx = KeyMap_GetIndex(&TextureMap, loadName.c_str());
                    if (tIdx >= 0 && tIdx < (int)g_textureSRV.size())
                        return g_textureSRV[tIdx];
                }
            }
        }
        return nullptr;
        };

    // iterate meshes
    for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi) {
        aiMesh* mesh = scene->mMeshes[mi];
        if (!mesh) continue;

        ModelSubmeshInfo sm;
        sm.materialIndex = (mesh->mMaterialIndex >= 0) ? mesh->mMaterialIndex : -1;
        sm.verts.clear();
        sm.idx.clear();
        sm.diffuseTexName.clear();
        sm.materialDiffuse = XMFLOAT4(1, 1, 1, 1);

        bool hasNormals = mesh->HasNormals();
        bool hasTexcoords = mesh->HasTextureCoords(0);

        unsigned int nextLocalIndex = 0;
        // expand faces to explicit triangle list (duplicates vertices per face index)
        for (unsigned int fi = 0; fi < mesh->mNumFaces; ++fi) {
            aiFace& face = mesh->mFaces[fi];
            if (face.mNumIndices < 3) continue;
            for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                unsigned int vi = face.mIndices[j];
                ModelVertex v{};
                v.pos = XMFLOAT3(mesh->mVertices[vi].x, mesh->mVertices[vi].y, mesh->mVertices[vi].z);

                if (hasNormals) {
                    v.normal = XMFLOAT3(mesh->mNormals[vi].x, mesh->mNormals[vi].y, mesh->mNormals[vi].z);
                }
                else {
                    v.normal = XMFLOAT3(0, 1, 0);
                }

                if (hasTexcoords) {
                    // ベースのUV
                    float u = mesh->mTextureCoords[0][vi].x;
                    float vv = mesh->mTextureCoords[0][vi].y;

                    // 水平方向反転
                    if (g_DebugUVFlipU) u = 1.0f - u;

                    // 垂直方向反転
                    if (g_DebugUVFlipV) vv = 1.0f - vv;

                    // 手動オフセット調整（本来のUVに足す）
                    u += g_DebugUVShiftU;
                    vv += g_DebugUVShiftV;

                    // 必要なら範囲クランプ
                    if (u < 0) u += 1.0f;
                    if (u > 1) u -= 1.0f;
                    if (vv < 0) vv += 1.0f;
                    if (vv > 1) vv -= 1.0f;

                    v.uv = XMFLOAT2(u, vv);
                }
                else {
                    v.uv = XMFLOAT2(0, 0);
                }
                //v.uv = XMFLOAT2(0, 0);
                sm.verts.push_back(v);
                sm.idx.push_back(nextLocalIndex++);
            }
        }

        // material: diffuse color + diffuse texture path (if present)
        if (scene->HasMaterials() && sm.materialIndex >= 0 && sm.materialIndex < (int)scene->mNumMaterials) {
            aiMaterial* mat = scene->mMaterials[sm.materialIndex];
            if (mat) {
                aiColor4D col(1, 1, 1, 1);
                if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &col)) {
                    sm.materialDiffuse = XMFLOAT4(col.r, col.g, col.b, col.a);
                    // optional debug
                    // char dbg[256]; sprintf_s(dbg, "Mesh %u material diffuse %f,%f,%f,%f", mi, col.r, col.g, col.b, col.a); AddMessage(dbg);
                }

                if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                    aiString texPath;
                    if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                        std::string raw = texPath.C_Str();

                        // 1) If embedded texture referenced (path like "*0"), attempt direct load from embedded
                        ID3D11ShaderResourceView* embeddedSRV = TryLoadEmbeddedTexture(scene, raw);
                        if (embeddedSRV) {
                            // register key under model#embeddedN used above in TryLoadEmbeddedTexture
                            // We returned SRV already registered under a synthetic name; we push that name as candidate
                            // try to find the key we registered: search TextureMap for keys containing "#embedded" substring
                            // Instead, push basename fallback as we already registered under a special key
                            // Push a candidate that Model::SetModelPath can query later; use the modelPathStr + "#embeddedN"
                            std::string embKey;
                            // attempt to compute embKey consistent with TryLoadEmbeddedTexture's name
                            if (raw.size() >= 2 && raw[0] == '*') {
                                int idx = atoi(raw.c_str() + 1);
                                embKey = modelPathStr + std::string("#embedded") + std::to_string(idx);
                                sm.diffuseTexName = embKey;
                                g_modelMaterialNames[modelIndex].push_back(embKey);
                            }
                            else {
                                // fallback to basename
                                std::string base = fs::path(raw).filename().string();
                                if (!base.empty()) g_modelMaterialNames[modelIndex].push_back(base);
                            }
                        }
                        else {
                            // 2) Not embedded or failed: try to resolve via TryResolveAndLoadTextureSRV
                            std::string resolvedKey;
                            ID3D11ShaderResourceView* srv = TryResolveAndLoadTextureSRV(raw, resolvedKey, modelPathStr);
                            if (srv && !resolvedKey.empty()) {
                                sm.diffuseTexName = resolvedKey;
                                g_modelMaterialNames[modelIndex].push_back(resolvedKey);
                            }
                            else {
                                // fallback: push basename as candidate for later attempts
                                std::string base = fs::path(raw).filename().string();
                                if (!base.empty()) g_modelMaterialNames[modelIndex].push_back(base);
                                else g_modelMaterialNames[modelIndex].push_back(raw);
                            }
                        }
                    }
                }
            }
        }

        // Debug: show submesh name (you used this to inspect)
        // MessageBoxA(NULL, mesh->mName.C_Str(), "Loaded Submesh", MB_OK);

        // push the submesh info
        g_modelSubmeshes[modelIndex].push_back(std::move(sm));
    }

    // flatten into g_modelVertex/g_modelIndices (legacy compatibility)
    {
        std::vector<ModelVertex> flatVerts;
        std::vector<unsigned int> flatIdx;
        unsigned int base = 0;
        for (auto& sm : g_modelSubmeshes[modelIndex]) {
            for (auto& v : sm.verts) flatVerts.push_back(v);
            for (auto id : sm.idx) flatIdx.push_back(base + id);
            base = (unsigned int)flatVerts.size();
        }
        g_modelVertex[modelIndex] = std::move(flatVerts);
        g_modelIndices[modelIndex] = std::move(flatIdx);
    }

    // Try to pre-load textures found in materials (if they exist in packages or next to model)
    for (auto& candidate : g_modelMaterialNames[modelIndex]) {
        if (candidate.empty()) continue;
        // 1) already loaded?
        int tIdx = KeyMap_GetIndex(&TextureMap, candidate.c_str());
        if (tIdx >= 0 && tIdx < (int)g_textureSRV.size() && g_textureSRV[tIdx]) continue;

        // 2) try load from package by candidate name
        if (AL_LoadFromPackageByName(candidate.c_str())) continue;

        // 3) try to find in model folder (attempt register & load)
        fs::path mpath(name ? name : std::string());
        std::string modelDir = mpath.has_parent_path() ? mpath.parent_path().string() : std::string();
        if (!modelDir.empty()) {
            fs::path p = fs::path(modelDir) / fs::path(candidate);
            if (fs::exists(p)) {
                RegisterAndLoadFileToPackage(p.generic_string());
            }
        }
    }

    return true;
}

bool IN_LoadFBX_Memory(const char* name, const unsigned char* data, size_t size) { return LoadModel_Assimp_FromMemory(name, data, size, true); }
bool IN_LoadModelObj_Memory(const char* name, const unsigned char* data, size_t size) { return LoadModel_Assimp_FromMemory(name, data, size, false); }

// ------------------------------
// WAV loader
// ------------------------------
struct WavData {
    std::vector<BYTE> buffer;
    WAVEFORMATEX format = {};
};
static std::vector<WavData> g_wavData;

const WavData* GetWavData(const char* name) {
    int idx = KeyMap_GetIndex(&WavMap, name);
    if (idx < 0 || idx >= (int)g_wavData.size()) return nullptr;
    return &g_wavData[idx];
}
bool IN_LoadWav_Memory(const char* name, const unsigned char* data, size_t size) {
    if (!data || size == 0) return false;
    int WavIndex = KeyMap_Add(&WavMap, name);
    if ((int)g_wavData.size() <= WavIndex) g_wavData.resize(WavIndex + 1);
    const BYTE* ptr = data;
    if (size < 44 || strncmp((const char*)ptr, "RIFF", 4) != 0 || strncmp((const char*)(ptr + 8), "WAVE", 4) != 0) return false;
    const BYTE* fmtChunk = nullptr; const BYTE* dataChunk = nullptr; size_t dataSize = 0;
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

// ------------------------------
// Public Getters used by Model code
// ------------------------------
const std::vector<ModelVertex>* GetModelVertices(const char* modelName) {
    if (!modelName) return nullptr;
    int idx = KeyMap_GetIndex(&ModelMap, modelName);
    if (idx < 0 || idx >= (int)g_modelVertex.size()) return nullptr;
    return &g_modelVertex[idx];
}
const std::vector<unsigned int>* GetModelIndices(const char* modelName) {
    if (!modelName) return nullptr;
    int idx = KeyMap_GetIndex(&ModelMap, modelName);
    if (idx < 0 || idx >= (int)g_modelIndices.size()) return nullptr;
    return &g_modelIndices[idx];
}
ID3D11ShaderResourceView* GetModelTexture(const char* modelName) {
    if (!modelName) return nullptr;
    int idx = KeyMap_GetIndex(&ModelMap, modelName);
    if (idx < 0 || idx >= (int)g_modelTextureSRV.size()) {
        // attempt to locate material names and load first candidate from packages if available
        int mIdx = KeyMap_GetIndex(&ModelMap, modelName);
        if (mIdx < 0 || mIdx >= (int)g_modelMaterialNames.size()) return nullptr;
        auto& mats = g_modelMaterialNames[mIdx];
        for (auto& candidate : mats) {
            // if texture already loaded in TextureMap, return it
            int tIdx = KeyMap_GetIndex(&TextureMap, candidate.c_str());
            if (tIdx >= 0 && tIdx < (int)g_textureSRV.size() && g_textureSRV[tIdx]) {
                // ensure g_modelTextureSRV sized
                if ((int)g_modelTextureSRV.size() <= mIdx) g_modelTextureSRV.resize(mIdx + 1, nullptr);
                g_modelTextureSRV[mIdx] = g_textureSRV[tIdx];
                return g_modelTextureSRV[mIdx];
            }
            // otherwise try to AL_LoadFromPackageByName for the candidate (package contains it)
            if (AL_LoadFromPackageByName(candidate.c_str())) {
                int newIdx = KeyMap_GetIndex(&TextureMap, candidate.c_str());
                if (newIdx >= 0 && newIdx < (int)g_textureSRV.size() && g_textureSRV[newIdx]) {
                    if ((int)g_modelTextureSRV.size() <= mIdx) g_modelTextureSRV.resize(mIdx + 1, nullptr);
                    g_modelTextureSRV[mIdx] = g_textureSRV[newIdx];
                    return g_modelTextureSRV[mIdx];
                }
            }
        }
        return nullptr;
    }
    return g_modelTextureSRV[idx];
}

ID3D11ShaderResourceView* GetTextureSRV(const char* filename) {
    if (!filename) return nullptr;
    int index = KeyMap_GetIndex(&TextureMap, filename);
    if (index >= 0 && index < (int)g_textureSRV.size()) {
        return g_textureSRV[index];
    }
    // not registered: try load from package
    if (!AL_LoadFromPackageByName(filename)) {
        // report missing
        AddMessage(("Texture not found: " + std::string(filename)).c_str());
        return nullptr;
    }
    index = KeyMap_GetIndex(&TextureMap, filename);
    if (index < 0 || index >= (int)g_textureSRV.size()) {
        AddMessage("GetTextureSRV: invalid index after load");
        return nullptr;
    }
    return g_textureSRV[index];
}

// ------------------------------
// Package creation / management (batch time)
// ------------------------------
void AL_Init() {
    g_packages.clear();
    KeyMap_Init(&TextureMap);
    KeyMap_Init(&ModelMap);
    KeyMap_Init(&WavMap);
    // reserve minimal vectors to avoid frequent reallocation
    g_textureSRV.clear();
    g_modelVertex.clear();
    g_modelIndices.clear();
    g_modelTextureSRV.clear();
    g_modelMaterialNames.clear();
    g_modelSubmeshes.clear();
}

void AL_Shutdown() {
    // release SRVs
    for (auto srv : g_textureSRV) if (srv) srv->Release();
    g_textureSRV.clear();
    for (auto srv : g_modelTextureSRV) if (srv) srv->Release();
    g_modelTextureSRV.clear();
    g_modelVertex.clear();
    g_modelIndices.clear();
    g_modelMaterialNames.clear();
    g_modelSubmeshes.clear();

    // close package streams & free keymaps
    for (auto& p : g_packages) {
        if (p.pkgStream.is_open()) p.pkgStream.close();
        KeyMap_Free(&p.keymap);
    }
    g_packages.clear();
    KeyMap_Free(&TextureMap);
    KeyMap_Free(&ModelMap);
    KeyMap_Free(&WavMap);

    // sampler etc - if created in future, release it here
    if (g_samplerState) { g_samplerState->Release(); g_samplerState = nullptr; }
}

// Old-style RegisterAssetToBatch kept for compatibility (editor-side incremental)
bool AL_RegisterAssetToBatch(const char* filepath) {
    if (!filepath) return false;
    std::string path = filepath;
    std::string ext = fs::path(path).extension().string();
    ext = sanitizeExt(ext);
    if (ext.empty()) return false;

    Package* pkg = FindPackageByExt(ext);
    if (!pkg) {
        Package np;
        np.ext = ext;
        KeyMap_Init(&np.keymap);
        std::lock_guard<std::mutex> lg(g_packageMutex);
        g_packages.push_back(std::move(np));
        pkg = &g_packages.back();
    }

    int existing = KeyMap_GetIndex(&pkg->keymap, path.c_str());
    if (existing != -1) return true; // already registered

    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return false;
    std::streamsize size = in.tellg();
    in.seekg(0);
    PackageEntry e;
    e.name = path;
    if (size > 0) {
        e.size = (uint64_t)size;
        e.data.resize((size_t)size);
        in.read((char*)e.data.data(), size);
    }
    else e.size = 0;
    in.close();

    KeyMap_Add(&pkg->keymap, e.name.c_str());
    pkg->entries.push_back(std::move(e));
    return true;
}

bool AL_SaveAllPackages(const char* outFolder) {
    if (!outFolder) return false;
    fs::path outDir(outFolder);
    try {
        fs::create_directories(outDir);
    }
    catch (...) { return false; }

    std::lock_guard<std::mutex> lg(g_packageMutex);
    for (auto& pkg : g_packages) {
        std::string outPath = (outDir / ("asset" + pkg.ext + ".pkg")).string();
        std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) return false;

        const char magic[8] = "LIA_PKG";
        uint32_t count = (uint32_t)pkg.entries.size();
        uint64_t tableOffset = 0;
        out.write(magic, 8);
        out.write((char*)&count, sizeof(uint32_t));
        tableOffset = 8 + 4 + 8;
        out.write((char*)&tableOffset, sizeof(uint64_t));

        std::streampos tablePos = out.tellp();
        size_t tableSize = 0;
        for (auto& e : pkg.entries) {
            tableSize += sizeof(uint16_t) + e.name.size() + sizeof(uint64_t) * 2;
        }
        out.seekp(tablePos + (std::streamoff)tableSize);

        for (auto& e : pkg.entries) {
            std::streampos dataPos = out.tellp();
            e.offset = (uint64_t)dataPos;
            if (!e.data.empty()) out.write((char*)e.data.data(), e.data.size());
        }

        out.seekp(tablePos);
        for (auto& e : pkg.entries) {
            uint16_t len = (uint16_t)e.name.size();
            out.write((char*)&len, sizeof(uint16_t));
            out.write(e.name.data(), len);
            out.write((char*)&e.offset, sizeof(uint64_t));
            out.write((char*)&e.size, sizeof(uint64_t));
        }

        out.close();
    }
    return true;
}

// LoadPackageIndex reads index from a .pkg and stores entries (no data loaded)
bool AL_LoadPackageIndex(const char* ext, const char* pkgFilePath) {
    if (!ext || !pkgFilePath) return false;
    std::string sExt = ToLowerExt(ext);
    Package* pkg = nullptr;
    {
        std::lock_guard<std::mutex> lg(g_packageMutex);
        pkg = FindPackageByExt(sExt);
        if (!pkg) {
            Package np;
            np.ext = sExt;
            KeyMap_Init(&np.keymap);
            g_packages.push_back(std::move(np));
            pkg = &g_packages.back();
        }
        else {
            if (pkg->pkgStream.is_open()) pkg->pkgStream.close();
            pkg->entries.clear();
            KeyMap_Free(&pkg->keymap);
            KeyMap_Init(&pkg->keymap);
        }
    }

    pkg->pkgPath = pkgFilePath;
    std::ifstream in(pkgFilePath, std::ios::binary);
    if (!in.is_open()) return false;

    char magic[8] = {};
    in.read(magic, 8);
    uint32_t count = 0;
    uint64_t tableOffset = 0;
    in.read((char*)&count, sizeof(uint32_t));
    in.read((char*)&tableOffset, sizeof(uint64_t));
    in.seekg(tableOffset);
    for (uint32_t i = 0; i < count; ++i) {
        uint16_t nameLen = 0;
        in.read((char*)&nameLen, sizeof(uint16_t));
        std::string name(nameLen, '\0');
        in.read(name.data(), nameLen);
        uint64_t offset = 0, size = 0;
        in.read((char*)&offset, sizeof(uint64_t));
        in.read((char*)&size, sizeof(uint64_t));
        PackageEntry e;
        e.name = name;
        e.offset = offset;
        e.size = size;
        KeyMap_Add(&pkg->keymap, e.name.c_str());
        pkg->entries.push_back(std::move(e));
    }
    in.close();
    return true;
}

// Read specific package entry's raw bytes into memory and call loader
static bool ReadPackageEntryDataToMemory(PackageEntry& e, Package& pkg, std::vector<uint8_t>& outData)
{
    // ensure stream open
    if (!pkg.pkgStream.is_open()) {
        pkg.pkgStream.open(pkg.pkgPath, std::ios::binary);
        if (!pkg.pkgStream.is_open()) return false;
    }

    try {
        pkg.pkgStream.seekg((std::streamoff)e.offset);
        outData.resize((size_t)e.size);
        pkg.pkgStream.read((char*)outData.data(), (std::streamsize)e.size);
        return true;
    }
    catch (...) {
        return false;
    }
}

static bool WriteTempAndCallLoader(PackageEntry& e, Package& pkg) {
    std::vector<uint8_t> data;
    if (e.data.empty()) {
        if (!ReadPackageEntryDataToMemory(e, pkg, data)) return false;
    }
    else {
        data = e.data;
    }

    // determine ext (lowercase, no dot)
    std::string ext = fs::path(e.name).extension().string();
    if (!ext.empty() && ext[0] == '.') ext.erase(0, 1);
    ext = ToLowerExt(ext);

    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp") {
        return IN_LoadTexture_Memory(e.name.c_str(), data.data(), data.size());
    }
    else if (ext == "tga") {
        // we provide an internal tga loader implementation in this file (if present)
        // If project also supplies IN_LoadTGA_Memory externally, prefer it via extern.
        extern bool IN_LoadTGA_Memory(const char* name, const unsigned char* data, size_t size);
        // Try external first (linker will resolve if implemented), otherwise fall back to internal parser
        // (we assume extern exists in many builds — but to be safe, wrap in try/call pattern).
        // We'll attempt to call extern; if unresolved at link time, user must provide it.
        // Here call local IN_LoadTexture_Memory for safety as fallback via WIC if possible:
        if (IN_LoadTexture_Memory(e.name.c_str(), data.data(), data.size())) return true;
        // Note: if a project-specific IN_LoadTGA_Memory is available, it should be used instead by providing it in project.
        return false;
    }
    else if (ext == "obj") {
        return IN_LoadModelObj_Memory(e.name.c_str(), data.data(), data.size());
    }
    else if (ext == "fbx") {
        return IN_LoadFBX_Memory(e.name.c_str(), data.data(), data.size());
    }
    else if (ext == "wav") {
        return IN_LoadWav_Memory(e.name.c_str(), data.data(), data.size());
    }
    else {
        // unknown extension - no loader
        return false;
    }
}

static bool TryLoadAlternativeImageExtensionsForEntry(const PackageEntry& e, Package& pkg)
{
    // derive basename / stem / dir
    fs::path p(e.name);
    std::string basename = p.filename().string();
    std::string stem = p.stem().string();
    std::string dirpart = p.has_parent_path() ? p.parent_path().generic_string() : std::string();

    // prioritized alternatives (png/tga first)
    std::vector<std::string> altExts = { ".png", ".tga", ".jpg", ".jpeg", ".dds" };

    Package* foundPkg = nullptr;
    int foundIdx = -1;

    // 1) try same stem in same package if dirpart exists: dir/stem.ext
    if (!dirpart.empty()) {
        for (auto& ext : altExts) {
            std::string cand = (fs::path(dirpart) / (stem + ext)).generic_string();
            if (FindPackageEntryByName(cand, foundPkg, foundIdx)) {
                if (foundPkg && foundIdx >= 0) return WriteTempAndCallLoader(foundPkg->entries[foundIdx], *foundPkg);
            }
        }
    }

    // 2) try basename variants (stem.ext) across all packages
    for (auto& ext : altExts) {
        std::string cand = stem + ext;
        if (FindPackageEntryByName(cand, foundPkg, foundIdx)) {
            if (foundPkg && foundIdx >= 0) return WriteTempAndCallLoader(foundPkg->entries[foundIdx], *foundPkg);
        }
    }

    // 3) fallback: linear search by filename across all packages
    for (auto& pack : g_packages) {
        for (int i = 0; i < (int)pack.entries.size(); ++i) {
            fs::path ent(pack.entries[i].name);
            if (ent.filename() == basename) {
                return WriteTempAndCallLoader(pack.entries[i], pack);
            }
        }
    }

    return false;
}



// AL_LoadFromPackageByName: find entry by key and load on demand
bool AL_LoadFromPackageByName(const char* name) {
    if (!name) return false;
    Package* pkg = nullptr;
    int idx = -1;
    if (!FindPackageEntryByName(name, pkg, idx)) {
        return false;
    }
    if (!pkg) return false;
    if (idx < 0 || idx >= (int)pkg->entries.size()) return false;
    PackageEntry& e = pkg->entries[idx];
    return WriteTempAndCallLoader(e, *pkg);
}

bool IN_LoadTGA_Memory(const char* name, const unsigned char* data, size_t size)
{
    if (!name || !data || size < 18) return false;

    // TGA header (18 bytes)
    const unsigned char* ptr = data;
    uint8_t idLength = ptr[0];
    uint8_t colorMapType = ptr[1];
    uint8_t imageType = ptr[2]; // 2 = uncompressed true-color
    // we ignore color map fields for simplicity
    uint16_t width = (uint16_t)ptr[12] | ((uint16_t)ptr[13] << 8);
    uint16_t height = (uint16_t)ptr[14] | ((uint16_t)ptr[15] << 8);
    uint8_t bpp = ptr[16]; // bits per pixel
    uint8_t descriptor = ptr[17];

    if (imageType != 2) {
        AddMessage("IN_LoadTGA_Memory: unsupported TGA image type (only uncompressed true-color supported)");
        return false;
    }
    if (width == 0 || height == 0) {
        AddMessage("IN_LoadTGA_Memory: invalid TGA size");
        return false;
    }
    if (bpp != 24 && bpp != 32) {
        AddMessage("IN_LoadTGA_Memory: unsupported bpp (only 24/32 supported)");
        return false;
    }

    // compute image data start
    size_t headerSize = 18;
    size_t imageDataSize = (size_t)width * (size_t)height * (bpp / 8);
    if (idLength) headerSize += idLength;
    if (size < headerSize + imageDataSize) {
        AddMessage("IN_LoadTGA_Memory: truncated data");
        return false;
    }

    const unsigned char* imgSrc = ptr + headerSize;

    // Prepare RGBA buffer (DX wants RGBA in memory)
    std::vector<BYTE> pixels((size_t)width * (size_t)height * 4);
    bool hasAlpha = (bpp == 32);

    // TGA stores BGR(A) per pixel. Also row order may be bottom-up unless descriptor indicates top-left.
    bool originTop = (descriptor & 0x20) != 0; // bit5: top-left if set
    size_t srcRowBytes = (size_t)width * (bpp / 8);

    for (uint32_t y = 0; y < height; ++y) {
        uint32_t srcY = originTop ? y : (height - 1 - y);
        const unsigned char* rowPtr = imgSrc + (size_t)srcY * srcRowBytes;
        for (uint32_t x = 0; x < width; ++x) {
            const unsigned char* px = rowPtr + (size_t)x * (bpp / 8);
            size_t dstIndex = ((size_t)y * width + x) * 4;
            // BGR -> RGB, copy alpha if present
            pixels[dstIndex + 0] = px[2]; // R
            pixels[dstIndex + 1] = px[1]; // G
            pixels[dstIndex + 2] = px[0]; // B
            pixels[dstIndex + 3] = hasAlpha ? px[3] : 0xFF; // A
        }
    }

    // Register key
    int texIndex = KeyMap_Add(&TextureMap, name);
    if ((int)g_textureSRV.size() <= texIndex) g_textureSRV.resize(texIndex + 1, nullptr);

    if (!GetDevice()) {
        AddMessage("IN_LoadTGA_Memory: device is null");
        return false;
    }

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // not sRGB — caller can adjust if needed
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = pixels.data();
    init.SysMemPitch = width * 4;

    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = GetDevice()->CreateTexture2D(&desc, &init, &tex);
    if (FAILED(hr)) {
        AddMessage("IN_LoadTGA_Memory: CreateTexture2D failed");
        return false;
    }

    ID3D11ShaderResourceView* srv = nullptr;
    hr = GetDevice()->CreateShaderResourceView(tex, nullptr, &srv);
    if (FAILED(hr)) {
        AddMessage("IN_LoadTGA_Memory: CreateSRV failed");
        tex->Release();
        return false;
    }

    g_textureSRV[texIndex] = srv;
    SafeRelease(tex);
    return true;
}

static std::string ResolveTextureCandidateFull(const std::string& rawTexName)
{
    if (rawTexName.empty())
        return "";

    // 拡張子分解
    std::string basename = rawTexName;
    std::string ext = "";
    {
        size_t dot = rawTexName.find_last_of('.');
        if (dot != std::string::npos)
        {
            basename = rawTexName.substr(0, dot);
            ext = rawTexName.substr(dot + 1);
            for (auto& c : ext) c = (char)tolower(c);
        }
    }

    // 探索候補
    std::vector<std::string> candidates;

    // 1. 元の名前そのまま
    candidates.push_back("texture/" + rawTexName);

    // 2. PSD → TGA → PNG に置換
    if (ext != "psd")
        candidates.push_back("texture/" + basename + ".psd");
    if (ext != "tga")
        candidates.push_back("texture/" + basename + ".tga");
    if (ext != "png")
        candidates.push_back("texture/" + basename + ".png");

    // ---- ここで pkg の中に存在するかを調べる ----
    for (auto& c : candidates)
    {
        int idx = AL_GetIndexFromPackage(nullptr, c.c_str());
        if (idx >= 0)
            return c; // 見つかった！
    }

    // ---- 見つからなかった場合 ----
    // ログ用
    AddMessage(("ResolveTextureCandidateFull: not found -> " + rawTexName).c_str());
    return ""; // なし
}

bool AL_LoadFromPackageByIndex(const char* ext, int index) {
    if (!ext) return false;
    Package* pkg = FindPackageByExt(ToLowerExt(ext));
    if (!pkg) return false;
    if (index < 0 || index >= (int)pkg->entries.size()) return false;
    PackageEntry& e = pkg->entries[index];
    return WriteTempAndCallLoader(e, *pkg);
}

bool RegisterAndLoadFileToPackage(const std::string& filepath)
{
    if (filepath.empty()) return false;
    fs::path fp(filepath);
    if (!fs::exists(fp) || !fs::is_regular_file(fp)) return false;

    // Try to see if already loaded by basename or full path
    std::string basename = fp.filename().generic_string();
    if (KeyMap_GetIndex(&TextureMap, filepath.c_str()) >= 0) return true;
    if (KeyMap_GetIndex(&TextureMap, basename.c_str()) >= 0) return true;

    // Read file into memory
    std::ifstream in(fp.string(), std::ios::binary | std::ios::ate);
    if (!in.is_open()) return false;
    std::streamsize sz = in.tellg();
    in.seekg(0);
    std::vector<uint8_t> buf;
    if (sz > 0) { buf.resize((size_t)sz); in.read((char*)buf.data(), sz); }
    in.close();

    std::string ext = ToLowerExt(fp.extension().string());
    if (!ext.empty() && ext[0] == '.') ext.erase(0, 1);

    bool ok = false;
    // Decide loader by extension
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp") {
        // register with basename key and full path key (IN_LoadTexture_Memory will register basename alias if implemented)
        ok = IN_LoadTexture_Memory(basename.c_str(), buf.data(), buf.size());
        if (ok) {
            // also register under full path key to be consistent with package-key lookups
            KeyMap_Add(&TextureMap, filepath.c_str());
            int idx = KeyMap_GetIndex(&TextureMap, filepath.c_str());
            if (idx >= 0 && idx < (int)g_textureSRV.size()) {
                // ensure SRV vector sized and copy pointer from basename index if necessary
                int baseIdx = KeyMap_GetIndex(&TextureMap, basename.c_str());
                if (baseIdx >= 0 && baseIdx < (int)g_textureSRV.size()) {
                    if ((int)g_textureSRV.size() <= idx) g_textureSRV.resize(idx + 1, nullptr);
                    g_textureSRV[idx] = g_textureSRV[baseIdx];
                }
            }
        }
    }
    else if (ext == "tga") {
        extern bool IN_LoadTGA_Memory(const char* name, const unsigned char* data, size_t size);
        // If IN_LoadTGA_Memory implemented (we provided one earlier in file), call it with basename
        ok = IN_LoadTGA_Memory(basename.c_str(), buf.data(), buf.size());
        if (ok) {
            KeyMap_Add(&TextureMap, filepath.c_str());
            int idx = KeyMap_GetIndex(&TextureMap, filepath.c_str());
            int baseIdx = KeyMap_GetIndex(&TextureMap, basename.c_str());
            if (idx >= 0 && baseIdx >= 0 && baseIdx < (int)g_textureSRV.size()) {
                if ((int)g_textureSRV.size() <= idx) g_textureSRV.resize(idx + 1, nullptr);
                g_textureSRV[idx] = g_textureSRV[baseIdx];
            }
        }
    }
    else if (ext == "fbx") {
        ok = IN_LoadFBX_Memory(basename.c_str(), buf.data(), buf.size());
        if (ok) {
            // register under full path too
            KeyMap_Add(&ModelMap, filepath.c_str());
        }
    }
    else if (ext == "obj") {
        ok = IN_LoadModelObj_Memory(basename.c_str(), buf.data(), buf.size());
        if (ok) KeyMap_Add(&ModelMap, filepath.c_str());
    }
    else if (ext == "wav") {
        ok = IN_LoadWav_Memory(basename.c_str(), buf.data(), buf.size());
        if (ok) KeyMap_Add(&WavMap, filepath.c_str());
    }
    else {
        // unsupported extension for direct load
        ok = false;
    }

    // If loaded OK, also register to batch packages (so it appears in saved pkg if desired)
    if (ok) {
        AL_RegisterAssetToBatch(filepath.c_str());
    }

    return ok;
}

static std::vector<std::string> FindAlternativeTexture(const std::string& rawName)
{
    std::vector<std::string> out;
    fs::path p(rawName);

    std::string stem = p.stem().string();
    std::string ext = p.extension().string();
    for (auto& c : ext) c = ::tolower(c);

    // PSD → PNG/TGA/JPG に変換
    static const char* altExts[] = { ".png", ".tga", ".jpg", ".jpeg" };

    // 元のファイル名（raw）
    out.push_back(rawName);

    // basename
    out.push_back(p.filename().string());

    // 拡張子を差し替えた候補
    for (auto& ae : altExts)
    {
        out.push_back(stem + ae);
        out.push_back(p.filename().replace_extension(ae).string());
    }

    return out;
}
// ------------------------------
// TryResolveAndLoadTextureSRV
// - rawTex: material-provided path/name inside FBX (could be "Texture/Alicia_hair.psd" or "Alicia_hair.psd")
// - modelPath: optional model source path used to search alongside model folder
// Search order: png -> tga -> jpg -> jpeg -> psd -> dds -> raw
// Returns SRV or nullptr. Also registers texture under resolved key name in TextureMap via IN_LoadTexture_Memory or RegisterAndLoadFileToPackage.
// ------------------------------
static ID3D11ShaderResourceView* TryResolveAndLoadTextureSRV(
    const std::string& rawTex,
    std::string& outResolvedKey,
    const std::string& modelPath)
{
    outResolvedKey.clear();
    if (rawTex.empty()) return nullptr;

    std::string s = rawTex;
    // trim helpers
    auto trim = [](std::string& str) {
        while (!str.empty() && isspace((unsigned char)str.front())) str.erase(str.begin());
        while (!str.empty() && isspace((unsigned char)str.back())) str.pop_back();
        if (!str.empty() && (str.front() == '\"' || str.front() == '\'')) str.erase(str.begin());
        if (!str.empty() && (str.back() == '\"' || str.back() == '\'')) str.pop_back();
        };
    trim(s);
    if (s.empty()) return nullptr;

    fs::path rawPath(s);
    std::string basename = rawPath.filename().string();
    std::vector<std::string> candidates;

    // push raw as-is
    candidates.push_back(s);
    if (basename != s) candidates.push_back(basename);

    // If original is PSD (or other) try common image extensions
    std::string stem = rawPath.stem().string();
    std::vector<std::string> exts = { ".png", ".tga", ".jpg", ".jpeg", ".dds" };
    for (auto& e : exts) {
        std::string p = stem + e;
        if (std::find(candidates.begin(), candidates.end(), p) == candidates.end())
            candidates.push_back(p);
    }

    // Also if raw had extension, try replacing it with common exts (avoid duplication)
    if (rawPath.has_extension()) {
        std::string rawStem = rawPath.stem().string();
        for (auto& e : exts) {
            std::string p = rawStem + e;
            if (std::find(candidates.begin(), candidates.end(), p) == candidates.end())
                candidates.push_back(p);
        }
    }

    // model directory, if provided
    std::string modelDir;
    if (!modelPath.empty()) {
        fs::path mp(modelPath);
        if (mp.has_parent_path()) modelDir = mp.parent_path().string();
    }

    // Search through candidates in order
    for (auto& cand : candidates) {
        // 1) if already loaded in TextureMap
        int tIdx = KeyMap_GetIndex(&TextureMap, cand.c_str());
        if (tIdx >= 0 && tIdx < (int)g_textureSRV.size() && g_textureSRV[tIdx]) {
            outResolvedKey = cand;
            return g_textureSRV[tIdx];
        }

        // 2) try model directory + candidate file (file system)
        if (!modelDir.empty()) {
            fs::path localPath = fs::path(modelDir) / fs::path(cand);
            if (fs::exists(localPath)) {
                std::string localStr = localPath.generic_string();
                // register+load into package system (calls IN_LoadXXX_Memory)
                if (RegisterAndLoadFileToPackage(localStr)) {
                    // first try registered under the local (relative) full path
                    ID3D11ShaderResourceView* srv = GetTextureSRV(localStr.c_str());
                    if (srv) { outResolvedKey = localStr; return srv; }
                    // then try basename
                    ID3D11ShaderResourceView* srv2 = GetTextureSRV(cand.c_str());
                    if (srv2) { outResolvedKey = cand; return srv2; }
                }
            }
        }

        // 3) try load by candidate name from packages
        if (AL_LoadFromPackageByName(cand.c_str())) {
            ID3D11ShaderResourceView* srv = GetTextureSRV(cand.c_str());
            if (srv) { outResolvedKey = cand; return srv; }
        }

        // 4) lastly try direct GetTextureSRV for candidate
        ID3D11ShaderResourceView* srv = GetTextureSRV(cand.c_str());
        if (srv) { outResolvedKey = cand; return srv; }
    }

    // nothing found
    return nullptr;
}

// 既存の呼び出し互換性のためのラッパー（古いコード向け）
static ID3D11ShaderResourceView* TryResolveAndLoadTextureSRV(const std::string& rawTex, const std::string& modelPath)
{
    std::string key;
    return TryResolveAndLoadTextureSRV(rawTex, key, modelPath);
}

// ------------------------------
// Get functions for package index / entries
// ------------------------------
int AL_GetIndexFromPackage(const char* ext, const char* name) {
    if (!ext || !name) return -1;
    Package* pkg = FindPackageByExt(ToLowerExt(ext));
    if (!pkg) return -1;
    return KeyMap_GetIndex(&pkg->keymap, name);
}

int AL_GetPackageCount() { return (int)g_packages.size(); }
const char* AL_GetPackageExt(int pkgIdx) {
    if (pkgIdx < 0 || pkgIdx >= (int)g_packages.size()) return nullptr;
    return g_packages[pkgIdx].ext.c_str();
}
int AL_GetPackageEntryCount(const char* ext) {
    Package* pkg = FindPackageByExt(ToLowerExt(ext));
    if (!pkg) return 0;
    return (int)pkg->entries.size();
}
const char* AL_GetPackageEntryName(const char* ext, int index) {
    Package* pkg = FindPackageByExt(ToLowerExt(ext));
    if (!pkg) return nullptr;
    if (index < 0 || index >= (int)pkg->entries.size()) return nullptr;
    return pkg->entries[index].name.c_str();
}

// ------------------------------
// Model helpers
// ------------------------------
int AL_GetModelMeshCount(const char* modelName) {
    if (!modelName) return 0;
    int idx = KeyMap_GetIndex(&ModelMap, modelName);
    if (idx < 0 || idx >= (int)g_modelSubmeshes.size()) return 0;
    return (int)g_modelSubmeshes[idx].size();
}

const std::vector<ModelVertex>* AL_GetModelMeshVertices(const char* modelName, int meshIdx) {
    if (!modelName) return nullptr;
    int idx = KeyMap_GetIndex(&ModelMap, modelName);
    if (idx < 0 || idx >= (int)g_modelSubmeshes.size()) return nullptr;
    if (meshIdx < 0 || meshIdx >= (int)g_modelSubmeshes[idx].size()) return nullptr;
    return &g_modelSubmeshes[idx][meshIdx].verts;
}

const std::vector<unsigned int>* AL_GetModelMeshIndices(const char* modelName, int meshIdx) {
    if (!modelName) return nullptr;
    int idx = KeyMap_GetIndex(&ModelMap, modelName);
    if (idx < 0 || idx >= (int)g_modelSubmeshes.size()) return nullptr;
    if (meshIdx < 0 || meshIdx >= (int)g_modelSubmeshes[idx].size()) return nullptr;
    return &g_modelSubmeshes[idx][meshIdx].idx;
}

XMFLOAT4 AL_GetModelMeshMaterialDiffuse(const char* modelName, int meshIdx) {
    XMFLOAT4 fallback = { 1,1,1,1 };
    if (!modelName) return fallback;
    int idx = KeyMap_GetIndex(&ModelMap, modelName);
    if (idx < 0 || idx >= (int)g_modelSubmeshes.size()) return fallback;
    if (meshIdx < 0 || meshIdx >= (int)g_modelSubmeshes[idx].size()) return fallback;
    return g_modelSubmeshes[idx][meshIdx].materialDiffuse;
}

const char* AL_GetModelMeshTextureName(const char* modelName, int meshIdx) {
    if (!modelName) return nullptr;
    int idx = KeyMap_GetIndex(&ModelMap, modelName);
    if (idx < 0 || idx >= (int)g_modelSubmeshes.size()) return nullptr;
    if (meshIdx < 0 || meshIdx >= (int)g_modelSubmeshes[idx].size()) return nullptr;
    return g_modelSubmeshes[idx][meshIdx].diffuseTexName.empty() ? nullptr : g_modelSubmeshes[idx][meshIdx].diffuseTexName.c_str();
}

// ------------------------------
// Build packages from asset folder (editor/encoder side).
// - Scans assetRoot recursively, groups files by extension, stores relative keys like "texture/test.png"
// - Writes saved/package/Asset<ext>.pkg per extension
// ------------------------------
bool AL_BuildPackagesFromAssetFolder(const char* assetRootC)
{
    if (!assetRootC) return false;
    fs::path assetRoot(assetRootC);
    if (!fs::exists(assetRoot) || !fs::is_directory(assetRoot)) {
        AddMessage("AL_BuildPackagesFromAssetFolder: asset root not found");
        return false;
    }

    std::map<std::string, Package> packages; // ext -> Package

    for (auto& it : fs::recursive_directory_iterator(assetRoot)) {
        try {
            if (!it.exists() || !it.is_regular_file()) continue;
        }
        catch (...) { continue; }
        fs::path fp = it.path();
        std::string ext = fp.extension().string();
        ext = sanitizeExt(ext);
        if (ext.empty()) continue;

        auto pit = packages.find(ext);
        if (pit == packages.end()) {
            Package np;
            np.ext = ext;
            KeyMap_Init(&np.keymap);
            packages.emplace(ext, std::move(np));
            pit = packages.find(ext);
        }

        PackageEntry e;
        e.name = MakeRelativeKey(assetRoot, fp); // relative key e.g. "texture/test.png"
        // Ensure key uses forward slashes
        std::replace(e.name.begin(), e.name.end(), '\\', '/');

        try {
            std::ifstream in(fp.string(), std::ios::binary | std::ios::ate);
            if (in.is_open()) {
                std::streamsize sz = in.tellg();
                in.seekg(0);
                if (sz > 0) {
                    e.size = (uint64_t)sz;
                    e.data.resize((size_t)sz);
                    in.read((char*)e.data.data(), sz);
                }
                else e.size = 0;
                in.close();
            }
            else {
                AddMessage(("AL_BuildPackages: failed to open file: " + fp.string()).c_str());
            }
        }
        catch (...) {
            AddMessage(("AL_BuildPackages: exception reading file: " + fp.string()).c_str());
        }

        Package& pkgref = pit->second;
        KeyMap_Add(&pkgref.keymap, e.name.c_str());
        pkgref.entries.push_back(std::move(e));
    }

    const std::string outDir = std::string("saved/package");
    if (!EnsureDirectoryExists(outDir)) {
        AddMessage("AL_BuildPackagesFromAssetFolder: failed to create saved/package directory");
        return false;
    }

    // write packages and register indexes into g_packages (entries have offsets assigned in file)
    {
        std::lock_guard<std::mutex> lg(g_packageMutex);
        g_packages.clear();
        for (auto& kv : packages) {
            std::string ext = kv.first;
            Package& pkg = kv.second;
            std::string outPath = outDir + "/Asset" + pkg.ext + ".pkg";
            // write package file
            std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
            if (!out.is_open()) {
                AddMessage(("AL_BuildPackagesFromAssetFolder: failed to write " + outPath).c_str());
                continue;
            }

            const char magic[8] = "LIA_PKG";
            uint32_t count = (uint32_t)pkg.entries.size();
            uint64_t tableOffset = 0;
            out.write(magic, 8);
            out.write((char*)&count, sizeof(uint32_t));
            tableOffset = 8 + 4 + 8;
            out.write((char*)&tableOffset, sizeof(uint64_t));

            std::streampos tablePos = out.tellp();
            size_t tableSize = 0;
            for (auto& e : pkg.entries) {
                tableSize += sizeof(uint16_t) + e.name.size() + sizeof(uint64_t) * 2;
            }
            out.seekp(tablePos + (std::streamoff)tableSize);

            for (auto& e : pkg.entries) {
                std::streampos dataPos = out.tellp();
                e.offset = (uint64_t)dataPos;
                if (!e.data.empty()) out.write((char*)e.data.data(), e.data.size());
            }

            out.seekp(tablePos);
            for (auto& e : pkg.entries) {
                uint16_t len = (uint16_t)e.name.size();
                out.write((char*)&len, sizeof(uint16_t));
                out.write(e.name.data(), len);
                out.write((char*)&e.offset, sizeof(uint64_t));
                out.write((char*)&e.size, sizeof(uint64_t));
            }
            out.close();

            // free entry data to save memory in builder (we are done writing)
            for (auto& e : pkg.entries) e.data.clear();

            // register index into g_packages
            Package gp;
            gp.ext = pkg.ext;
            KeyMap_Init(&gp.keymap);
            gp.entries = pkg.entries; // copies offsets + sizes
            gp.pkgPath = outPath;
            g_packages.push_back(std::move(gp));
        }
    }

    AddMessage("AL_BuildPackagesFromAssetFolder: packages built to saved/package");
    return true;
}


// ------------------------------
// AL_LoadAllPackageIndexes - load indices from saved/package directory (no data loaded)
// Note: header may define default arg; do not re-declare default here.
// ------------------------------
bool AL_LoadAllPackageIndexes(const char* savedPackageDir)
{
    if (!savedPackageDir) return false;
    try {
        fs::path dir(savedPackageDir);
        if (!fs::exists(dir) || !fs::is_directory(dir)) return false;
        for (auto& p : fs::directory_iterator(dir)) {
            if (!p.is_regular_file()) continue;
            fs::path fp = p.path();
            if (fp.extension() == ".pkg") {
                AL_LoadPackageIndex(fp.string().c_str(), fp.string().c_str()); // ext param duplicated but functions expects ext/pkgFilePath; we pass ext as filename's ext stripped
                // Actually AL_LoadPackageIndex expects (ext, pkgFilePath) in original - to preserve API call semantics we call alternative:
                // Instead call simple loader: parse ext from filename and call internal load
                // To avoid confusion, call helper that reads index file (we already have AL_LoadPackageIndex expecting ext and pkgFilePath).
                // Build ext from filename: Asset<ext>.pkg
                // We'll implement correct call below after deducing ext.
            }
        }
        // Because above loop used placeholder approach, do proper second pass:
        for (auto& p : fs::directory_iterator(dir)) {
            if (!p.is_regular_file()) continue;
            fs::path fp = p.path();
            if (fp.extension() != ".pkg") continue;
            // determine ext from filename: Asset<ext>.pkg
            std::string stem = fp.stem().string(); // Assetpng etc
            std::string ext;
            if (stem.rfind("Asset", 0) == 0) {
                ext = stem.substr(5);
            }
            else {
                ext = ToLowerExt(fp.extension().string());
                if (!ext.empty() && ext[0] == '.') ext.erase(0, 1);
            }
            AL_LoadPackageIndex(ext.c_str(), fp.string().c_str());
        }
        return true;
    }
    catch (...) {
        return false;
    }
}

// ------------------------------
// Convenience: AL_BuildAndSaveAll
// ------------------------------
bool AL_BuildAndSaveAll(const char* assetRoot, const char* outDir)
{
    if (!assetRoot) return false;
    bool ok = AL_BuildPackagesFromAssetFolder(assetRoot);
    if (!ok) return false;
    AL_LoadAllPackageIndexes(outDir ? outDir : "saved/package");
    return true;
}
int AL_RegisterFolderRecursive(const char* folder)
{
    if (!folder) return -1;
    try {
        namespace fs = std::filesystem;
        fs::path root(folder);
        if (!fs::exists(root) || !fs::is_directory(root)) {
            AddMessage(("AL_RegisterFolderRecursive: folder not found: " + std::string(folder)).c_str());
            return -1;
        }

        int count = 0;
        for (auto& entry : fs::recursive_directory_iterator(root)) {
            try {
                if (!entry.exists()) continue;
                if (!entry.is_regular_file()) continue;

                // 拡張子フィルタ（必要なら追加/削除してください）
                std::string ext = entry.path().extension().string();
                for (auto& c : ext) c = (char)tolower(c);
                if (ext.empty()) continue;

                // フルパスを AL_RegisterAssetToBatch に渡す
                std::string full = entry.path().string();
                if (AL_RegisterAssetToBatch(full.c_str())) count++;
            }
            catch (...) {
                // 個別ファイルエラーは無視して続行
            }
        }
        return count;
    }
    catch (...) {
        AddMessage("AL_RegisterFolderRecursive: exception");
        return -1;
    }
}
void AL_LoadAllPackages(const char* folder)
{
    namespace fs = std::filesystem;

    fs::path dir(folder);

    if (!fs::exists(dir) || !fs::is_directory(dir))
    {
        printf("[AssetManager] Package folder not found: %s\n", folder);
        return;
    }

    for (auto& entry : fs::directory_iterator(dir))
    {
        if (!entry.is_regular_file())
            continue;

        const fs::path& p = entry.path();
        if (p.extension() != ".pkg")
            continue;

        // pkgファイル名例： Assetpng.pkg → ext = png
        std::string filename = p.filename().string();
        std::string ext;

        // "Asset" の後の部分を取得
        if (filename.rfind("Asset", 0) == 0)
        {
            ext = filename.substr(5); // "png.pkg" 等
            size_t pos = ext.rfind(".pkg");
            if (pos != std::string::npos)
                ext = ext.substr(0, pos);
        }
        else
        {
            // fallback: ファイル名から拡張子取得
            ext = p.stem().string();
        }

        printf("[AssetManager] Auto loading package [%s] from file: %s\n",
            ext.c_str(), p.string().c_str());

        AL_LoadPackageIndex(ext.c_str(), p.string().c_str());
    }
}
