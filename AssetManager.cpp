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
std::vector<ID3D11ShaderResourceView*> g_textureSRV;        //テクスチャ保存用SRV
std::vector<std::vector<ModelVertex>> g_modelVertex;        //Obj保存用SRV
ID3D11SamplerState* g_samplerState;                         //デフォルトサンプラーステート
//キーマップ
KeyMap TextureMap;
KeyMap ModelMap;

ID3D11ShaderResourceView* GetTextureSRV(const char* filename)
{
    int index = KeyMap_GetIndex(&TextureMap, filename);

    if (index < 0 || index >= (int)g_textureSRV.size())
        return nullptr;

    return g_textureSRV[index];
}
std::vector<ModelVertex>* GetModelVertex(const char* filename)
{
    int index = KeyMap_GetIndex(&ModelMap, filename);

    if (index < 0 || index >= (int)g_modelVertex.size())
        return nullptr;

    return &g_modelVertex[index];
}

 //---------------------------------
// テクスチャ読み込み&保存
bool IN_LoadTexture(const char* filename)
{
    //既にロード済みならスキップ___________
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

    //COMが未初期化なら初期化して再試行________
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

    //画像読み込み_____________________
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

    // サンプラーステート
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = sampDesc.AddressV = sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    GetDevice()->CreateSamplerState(&sampDesc, &g_samplerState);


    return true;
}


bool IN_LoadModelObj(const char* filename)
{
    //既にロード済みならスキップ___________
    int mapSize = KeyMap_GetSize(&TextureMap);
    for (int i = 0; i < mapSize; i++) {
        const char* key = KeyMap_GetKey(&TextureMap, i);
        if (strcmp(key, filename) == 0) return true;
    }

    int ModelIndex = KeyMap_Add(&TextureMap, filename);
    std::ifstream file(ConvertToWString(filename));

    if (!file) return false;

    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT2> uvs;
    std::vector<XMFLOAT3> normals;

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v")
        {
            XMFLOAT3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (type == "vt")
        {
            XMFLOAT2 uv;
            iss >> uv.x >> uv.y;
            uvs.push_back(uv);
        }
        else if (type == "vn")
        {
            XMFLOAT3 n;
            iss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (type == "f")
        {
            std::vector<std::string> tokens;
            std::string s;
            while (iss >> s)
                tokens.push_back(s);

            // 三角形化 (三角ポリゴンのみ対応)
            for (size_t i = 1; i < tokens.size() - 1; ++i)
            {
                int idx[3][3];
                std::string vts[] = { tokens[0], tokens[i], tokens[i + 1] };
                for (int j = 0; j < 3; ++j)
                {
                    std::replace(vts[j].begin(), vts[j].end(), '/', ' ');
                    std::istringstream iss2(vts[j]);
                    iss2 >> idx[j][0] >> idx[j][1] >> idx[j][2];
                }

                for (int j = 0; j < 3; ++j)
                {
                    ModelVertex v;
                    v.pos = positions[idx[j][0] - 1];
                    v.uv = uvs[idx[j][1] - 1];
                    v.uv.y = 1.0f - v.uv.y; // Y反転
                    v.normal = normals[idx[j][2] - 1];
                    g_modelVertex[ModelIndex].push_back(v);
                }
            }
        }
    }

    if (g_modelVertex.empty()) {
        MessageBoxA(NULL, "OBJ読み込みで頂点が0です", "Error", MB_OK);
        return false;
    }

    return true;
}

bool IN_LoadFBX(const char* filename)
{
    // 既にロード済みならスキップ
    int mapSize = KeyMap_GetSize(&ModelMap);
    for (int i = 0; i < mapSize; ++i) {
        const char* key = KeyMap_GetKey(&ModelMap, i);
        if (strcmp(key, filename) == 0) return true;
    }

    // KeyMap に登録してインデックスを得る
    int ModelIndex = KeyMap_Add(&ModelMap, filename);

    // Assimp importer
    Assimp::Importer importer;

    // Assimp ReadFile に渡すパスは std::string（相対パスのまま渡す）
    std::string sFilename = filename;

    const aiScene* scene = importer.ReadFile(
        sFilename,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ConvertToLeftHanded
    );

    if (!scene) {
        std::string err = importer.GetErrorString();
        if (err.empty()) err = "Unknown Assimp error";
        MessageBoxA(nullptr, err.c_str(), "IN_LoadFBX: Assimp error", MB_OK);
        // 登録取り消し（KeyMap_Add が返した index を無効化する手段がない場合は放置）
        return false;
    }

    if (!scene->HasMeshes()) {
        MessageBoxA(nullptr, "IN_LoadFBX: no meshes in scene", "Error", MB_OK);
        return false;
    }

    // Ensure g_modelVertex has entry for this ModelIndex
    if ((int)g_modelVertex.size() <= ModelIndex) g_modelVertex.resize(ModelIndex + 1);

    // 合成先ベクター（ファイル1つ -> 1つのモデルエントリへ結合）
    std::vector<ModelVertex> outVerts;
    outVerts.clear();

    // iterate meshes and append vertices (インデックスを使って三角形を作る)
    for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi)
    {
        aiMesh* mesh = scene->mMeshes[mi];
        if (!mesh) continue;

        // positions / uvs / normals がメッシュにあるかチェック
        bool hasNormals = mesh->HasNormals();
        bool hasTexCoords = mesh->HasTextureCoords(0); // set 0 を使用

        // 読み込み: mesh の頂点ごとに一時配列を作っておく（positionsのみ）
        std::vector<XMFLOAT3> positions;
        std::vector<XMFLOAT2> uvs;
        std::vector<XMFLOAT3> normals;

        positions.resize(mesh->mNumVertices);
        if (hasTexCoords) uvs.resize(mesh->mNumVertices);
        if (hasNormals) normals.resize(mesh->mNumVertices);

        for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
        {
            positions[v] = XMFLOAT3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);

            if (hasTexCoords) {
                // Assimp のテクスチャ座標は aiVector3D（z を無視）
                aiVector3D t = mesh->mTextureCoords[0][v];
                uvs[v] = XMFLOAT2(t.x, t.y);
            }
            else {
                uvs[v] = XMFLOAT2(0.0f, 0.0f);
            }

            if (hasNormals) {
                normals[v] = XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
            }
            else {
                normals[v] = XMFLOAT3(0.0f, 0.0f, 0.0f);
            }
        }

        // faces -> 三角化済みなので、各 face のインデックスを走査して ModelVertex を生成
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
        {
            aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices < 3) continue; // 安全

            // face が三角形の場合は3つ、ポリゴン（triangulate指定済みのは3）
            for (unsigned int idx = 0; idx < face.mNumIndices; ++idx)
            {
                unsigned int vi = face.mIndices[idx];
                ModelVertex mv;
                mv.pos = positions[vi];
                mv.uv = uvs[vi];
                // Y反転が必要ならここで mv.uv.y = 1.0f - mv.uv.y; ただし Assimp 側で変換されている場合もある
                mv.normal = normals[vi];
                outVerts.push_back(mv);
            }
        }
    }

    if (outVerts.empty()) {
        MessageBoxA(nullptr, "IN_LoadFBX: no vertices extracted", "Error", MB_OK);
        return false;
    }

    // 取得頂点を g_modelVertex[ModelIndex] に保持する
    g_modelVertex[ModelIndex] = std::move(outVerts);

    // マテリアル -> テクスチャ読み込み（最初のメッシュのマテリアルのみ処理）
    if (scene->HasMaterials())
    {
        // まず mesh 0 の material index を取り、その material の diffuse を探す
        aiMesh* firstMesh = scene->mMeshes[0];
        if (firstMesh && scene->mMaterials && firstMesh->mMaterialIndex >= 0)
        {
            aiMaterial* mat = scene->mMaterials[firstMesh->mMaterialIndex];
            if (mat)
            {
                aiString texPath;
                if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
                {
                    std::string texStr = texPath.C_Str();
                    if (!texStr.empty())
                    {
                        // モデルファイルのフォルダを基準にする（相対パス対策）
                        // filename は char* （例: "assets/models/model.fbx"）
                        std::string sfilename = filename;
                        size_t pos = sfilename.find_last_of("/\\");
                        std::string folder = (pos == std::string::npos) ? std::string() : sfilename.substr(0, pos + 1);
                        std::string fullTex = folder + texStr;

                        // IN_LoadTexture は wchar を期待する可能性があるため、ここは既存 IN_LoadTexture のシグネチャに合わせて渡す
                        // 既存の IN_LoadTexture は const char* を受け取る形に合わせて fullTex.c_str() を渡します
                        IN_LoadTexture(fullTex.c_str());
                    }
                }
            }
        }
    }

    return true;
}