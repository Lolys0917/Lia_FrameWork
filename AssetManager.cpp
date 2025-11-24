#include "AssetLoad.h"
#include "Main.h"    // GetDevice(), GetContext(), AddMessage() など既存のユーティリティ
#include "Manager.h" // KeyMap 関連
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <wincodec.h> // WIC
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

#pragma comment(lib, "windowscodecs.lib")

using namespace DirectX;
namespace fs = std::filesystem;

#define SafeRelease(p) if(p){ (p)->Release(); (p) = nullptr; }

// ------------------------------
// 内部データ構造
// ------------------------------
struct PackageEntry {
    std::string name;
    uint64_t offset = 0;
    uint64_t size = 0;
    std::vector<uint8_t> data; // runtime に取り出すまで空でもよい
};

struct Package {
    std::string ext; // 小文字拡張子
    KeyMap keymap;
    std::vector<PackageEntry> entries;
    std::string pkgPath;
    std::ifstream pkgStream;
};

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

// ------------------------------
// helpers
// ------------------------------
static std::string ToLowerExt(const std::string& s) {
    std::string e = s;
    for (auto& c : e) c = (char)tolower(c);
    return e;
}
static Package* FindPackageByExt(const std::string& ext) {
    for (auto& p : g_packages) {
        if (p.ext == ext) return &p;
    }
    return nullptr;
}

// find package+index by name among loaded packages
static bool FindPackageEntryByName(const std::string& name, Package*& outPkg, int& outIndex) {
    for (auto& p : g_packages) {
        int idx = KeyMap_GetIndex(&p.keymap, name.c_str());
        if (idx >= 0) { outPkg = &p; outIndex = idx; return true; }
    }
    outPkg = nullptr; outIndex = -1; return false;
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
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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

// ------------------------------
// Assimp-based model loader (memory)
// - outputs expanded vertex list (triangle list) and indices are sequential 0..N-1
// - extracts diffuse texture name (if present) and attempts to load it via IN_LoadTexture_Memory
// ------------------------------
static bool LoadModel_Assimp_FromMemory(const char* name, const unsigned char* data, size_t size, bool isFBX)
{
    if (!data || size == 0) return false;

    // check already loaded
    int mapSize = KeyMap_GetSize(&ModelMap);
    for (int i = 0; i < mapSize; ++i) {
        const char* key = KeyMap_GetKey(&ModelMap, i);
        if (strcmp(key, name) == 0) return true; // already loaded
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

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFileFromMemory(
        data, size,
        aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices | aiProcess_ConvertToLeftHanded,
        isFBX ? "fbx" : "obj"
    );

    if (!scene || !scene->HasMeshes()) {
        std::string err = importer.GetErrorString();
        MessageBoxA(nullptr, ("Assimp: " + err).c_str(), "LoadModel_Memory Error", MB_OK);
        return false;
    }

    // Clear any placeholder
    g_modelSubmeshes[modelIndex].clear();

    // Iterate meshes and create submeshes
    for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi) {
        aiMesh* mesh = scene->mMeshes[mi];
        if (!mesh) continue;

        ModelSubmeshInfo sm;

        bool hasNormals = mesh->HasNormals();
        bool hasTex = mesh->HasTextureCoords(0);

        // Collect vertices and indices for this mesh (expand faces to triangle list)
        unsigned int nextLocalIndex = 0;
        std::vector<ModelVertex>& verts = sm.verts;
        std::vector<unsigned int>& idxs = sm.idx;

        for (unsigned int fi = 0; fi < mesh->mNumFaces; ++fi) {
            aiFace& face = mesh->mFaces[fi];
            if (face.mNumIndices < 3) continue;
            for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                unsigned int vi = face.mIndices[j];
                ModelVertex v{};
                v.pos = XMFLOAT3(mesh->mVertices[vi].x, mesh->mVertices[vi].y, mesh->mVertices[vi].z);
                v.normal = hasNormals ? XMFLOAT3(mesh->mNormals[vi].x, mesh->mNormals[vi].y, mesh->mNormals[vi].z) : XMFLOAT3(0, 1, 0);
                if (hasTex) v.uv = XMFLOAT2(mesh->mTextureCoords[0][vi].x, mesh->mTextureCoords[0][vi].y);
                else v.uv = XMFLOAT2(0, 0);

                verts.push_back(v);
                idxs.push_back(nextLocalIndex++);
            }
        }

        // material index
        sm.materialIndex = (mesh->mMaterialIndex >= 0) ? mesh->mMaterialIndex : -1;

        // try get material diffuse color and texture name (if material exists)
        if (scene->HasMaterials() && sm.materialIndex >= 0 && sm.materialIndex < (int)scene->mNumMaterials) {
            aiMaterial* mat = scene->mMaterials[sm.materialIndex];
            if (mat) {
                // Diffuse color
                aiColor4D col(1.0f, 1.0f, 1.0f, 1.0f);
                if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE, col)) {
                    sm.materialDiffuse = XMFLOAT4(col.r, col.g, col.b, col.a);
                }
                // Texture path (diffuse)
                if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                    aiString texPath;
                    if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                        std::string s = texPath.C_Str();
                        // normalize to filename
                        std::string filename = fs::path(s).filename().string();
                        sm.diffuseTexName = filename;
                        // Add to model-level material names list for compatibility
                        g_modelMaterialNames[modelIndex].push_back(filename);
                    }
                }
            }
        }

        // push this submesh
        g_modelSubmeshes[modelIndex].push_back(std::move(sm));
    }

    // For backward compatibility, optionally flatten into g_modelVertex/g_modelIndices
    // (some existing code may still call GetModelVertices). We'll create a single concatenated buffer.
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

    // Try to pre-load textures found in materials (if they exist in packages)
    for (auto& sm : g_modelSubmeshes[modelIndex]) {
        if (!sm.diffuseTexName.empty()) {
            int tIdx = KeyMap_GetIndex(&TextureMap, sm.diffuseTexName.c_str());
            if (tIdx < 0) {
                // try to load from package
                AL_LoadFromPackageByName(sm.diffuseTexName.c_str());
            }
            // If loaded, AddModel previously will find via GetTextureSRV later
        }
    }

    return true;
}

bool IN_LoadFBX_Memory(const char* name, const unsigned char* data, size_t size) { return LoadModel_Assimp_FromMemory(name, data, size, true); }
bool IN_LoadModelObj_Memory(const char* name, const unsigned char* data, size_t size) { return LoadModel_Assimp_FromMemory(name, data, size, false); }

// ------------------------------
// WAV loader (kept as original idea)
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
static std::string sanitizeExt(const std::string& ext) {
    std::string e = ext;
    if (!e.empty() && e[0] == '.') e.erase(0, 1);
    for (auto& c : e) c = (char)tolower(c);
    return e;
}

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
    fs::create_directories(outFolder);
    for (auto& pkg : g_packages) {
        std::string outPath = std::string(outFolder) + "/Asset" + pkg.ext + ".pkg";
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

bool AL_LoadPackageIndex(const char* ext, const char* pkgFilePath) {
    if (!ext || !pkgFilePath) return false;
    std::string sExt = ToLowerExt(ext);
    Package* pkg = FindPackageByExt(sExt);
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

    pkg->pkgPath = pkgFilePath;
    pkg->pkgStream.open(pkgFilePath, std::ios::binary);
    if (!pkg->pkgStream.is_open()) return false;

    char magic[8] = {};
    pkg->pkgStream.read(magic, 8);
    uint32_t count = 0;
    uint64_t tableOffset = 0;
    pkg->pkgStream.read((char*)&count, sizeof(uint32_t));
    pkg->pkgStream.read((char*)&tableOffset, sizeof(uint64_t));
    pkg->pkgStream.seekg(tableOffset);
    for (uint32_t i = 0; i < count; ++i) {
        uint16_t nameLen = 0;
        pkg->pkgStream.read((char*)&nameLen, sizeof(uint16_t));
        std::string name(nameLen, '\0');
        pkg->pkgStream.read(name.data(), nameLen);
        uint64_t offset = 0, size = 0;
        pkg->pkgStream.read((char*)&offset, sizeof(uint64_t));
        pkg->pkgStream.read((char*)&size, sizeof(uint64_t));
        PackageEntry e;
        e.name = name;
        e.offset = offset;
        e.size = size;
        KeyMap_Add(&pkg->keymap, e.name.c_str());
        pkg->entries.push_back(std::move(e));
    }
    return true;
}

// WriteTempAndCallLoader - we don't write temp file; instead read data into memory and dispatch
static bool WriteTempAndCallLoader(PackageEntry& e, Package& pkg) {
    // if e.data empty, read from stream
    if (e.data.empty()) {
        if (!pkg.pkgStream.is_open()) {
            pkg.pkgStream.open(pkg.pkgPath, std::ios::binary);
            if (!pkg.pkgStream.is_open()) return false;
        }
        pkg.pkgStream.seekg(e.offset);
        e.data.resize((size_t)e.size);
        pkg.pkgStream.read((char*)e.data.data(), (std::streamsize)e.size);
    }

    // determine ext
    std::string ext = fs::path(e.name).extension().string();
    if (!ext.empty() && ext[0] == '.') ext.erase(0, 1);
    ext = ToLowerExt(ext);

    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp")
        return IN_LoadTexture_Memory(e.name.c_str(), e.data.data(), e.data.size());
    else if (ext == "obj")
        return IN_LoadModelObj_Memory(e.name.c_str(), e.data.data(), e.data.size());
    else if (ext == "fbx")
        return IN_LoadFBX_Memory(e.name.c_str(), e.data.data(), e.data.size());
    else if (ext == "wav")
        return IN_LoadWav_Memory(e.name.c_str(), e.data.data(), e.data.size());
    else
        return false;
}

bool AL_LoadFromPackageByName(const char* name) {
    if (!name) return false;
    Package* pkg = nullptr;
    int idx = -1;
    if (!FindPackageEntryByName(name, pkg, idx)) 
    {
        return false; MessageBoxA(NULL, name, "ErrorByName", S_OK);
    }
    if (!pkg)
    {
        return false; MessageBoxA(NULL, name, "ErrorByName", S_OK);
    }
    if (idx < 0 || idx >= (int)pkg->entries.size()) return false;
    PackageEntry& e = pkg->entries[idx];
    return WriteTempAndCallLoader(e, *pkg);
}

bool AL_LoadFromPackageByIndex(const char* ext, int index) {
    if (!ext) return false;
    Package* pkg = FindPackageByExt(ToLowerExt(ext));
    if (!pkg) return false;
    if (index < 0 || index >= (int)pkg->entries.size()) return false;
    PackageEntry& e = pkg->entries[index];
    return WriteTempAndCallLoader(e, *pkg);
}

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

//モデル用Getter
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