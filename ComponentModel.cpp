#include "ComponentModel.h"
#include "Manager.h"
#include "AssetLoad.h"
#include <fstream>
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;
Model::~Model()
{
    Release();
}

void Model::SetModelPath(const char* filename)
{
    modelPath = filename;

    std::string s = filename;
    auto dot = s.find_last_of('.');
    if (dot != std::string::npos)
    {
        std::string ext = s.substr(dot + 1);
        for (auto& c : ext) c = ::tolower(c);

        if (ext == "fbx") modelType = ModelType::FBX;
        else modelType = ModelType::OBJ;
    }
    else modelType = ModelType::OBJ;

    if (modelPath.empty())
    {
        MessageBoxA(nullptr, "Model path is empty!", "Model::SetModelPath", MB_OK);
        return;
    }

    int meshCount = AL_GetModelMeshCount(modelPath.c_str());
    if (meshCount <= 0)
    {
        MessageBoxA(nullptr, ("Model not loaded or no meshes: " + modelPath).c_str(),
            "Model::SetModelPath", MB_OK);
        return;
    }
    Release();
    subMeshes.clear();
    subMeshes.resize(meshCount);

    ID3D11VertexShader* vs = GetVertexShader3D();
    ID3D11PixelShader* ps = GetPixelShader3D();
    ID3DBlob* vsBlob = GetCurrent3DVSBlob();
    if (!vs || !ps || !vsBlob)
    {
        MessageBoxA(nullptr, "3D shader not ready", "Model::SetModelPath", MB_OK);
        return;
    }

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
    {
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0, D3D11_INPUT_PER_VERTEX_DATA,0},
        {"NORMAL",  0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,   0,24,D3D11_INPUT_PER_VERTEX_DATA,0},
    };

    HRESULT hr = GetDevice()->CreateInputLayout(
        layoutDesc, 3,
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        inputLayout.GetAddressOf()
    );
    if (FAILED(hr)) {
        MessageBoxA(nullptr, "CreateInputLayout failed", "Model::SetModelPath", MB_OK);
        return;
    }

    D3D11_BUFFER_DESC cbd{};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(MatrixBuffer);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = GetDevice()->CreateBuffer(&cbd, nullptr, constantBuffer.GetAddressOf());
    if (FAILED(hr)) {
        MessageBoxA(nullptr, "CreateBuffer(constant) failed", "Model::SetModelPath", MB_OK);
        return;
    }

    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    hr = GetDevice()->CreateSamplerState(&sd, sampler.GetAddressOf());
    if (FAILED(hr)) {
        MessageBoxA(nullptr, "CreateSamplerState failed", "Model::SetModelPath", MB_OK);
        return;
    }

    totalIndexCount = 0;

    auto Normalize = [&](std::string& x) {
        std::replace(x.begin(), x.end(), '\\', '/');
        };

    auto GetBasename = [&](const std::string& p) -> std::string {
        size_t pos = p.find_last_of('/');
        if (pos == std::string::npos) return p;
        return p.substr(pos + 1);
        };

    auto GetDir = [&](const std::string& p) -> std::string {
        size_t pos = p.find_last_of('/');
        if (pos == std::string::npos) return "";
        return p.substr(0, pos);
        };

    // ===== メインループ =====
    for (int mi = 0; mi < meshCount; ++mi)
    {
        const std::vector<ModelVertex>* vtx = AL_GetModelMeshVertices(modelPath.c_str(), mi);
        const std::vector<unsigned int>* idx = AL_GetModelMeshIndices(modelPath.c_str(), mi);

        if (!vtx || vtx->empty() || !idx || idx->empty())
            continue;

        D3D11_BUFFER_DESC vbd{};
        vbd.Usage = D3D11_USAGE_DEFAULT;
        vbd.ByteWidth = (UINT)(sizeof(ModelVertex) * vtx->size());
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vinit{};
        vinit.pSysMem = vtx->data();
        hr = GetDevice()->CreateBuffer(&vbd, &vinit, subMeshes[mi].vertexBuffer.GetAddressOf());

        D3D11_BUFFER_DESC ibd{};
        ibd.Usage = D3D11_USAGE_DEFAULT;
        ibd.ByteWidth = (UINT)(sizeof(unsigned int) * idx->size());
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA iinit{};
        iinit.pSysMem = idx->data();
        hr = GetDevice()->CreateBuffer(&ibd, &iinit, subMeshes[mi].indexBuffer.GetAddressOf());

        subMeshes[mi].indexCount = (UINT)idx->size();
        totalIndexCount += subMeshes[mi].indexCount;

        subMeshes[mi].materialDiffuse =
            AL_GetModelMeshMaterialDiffuse(modelPath.c_str(), mi);

        subMeshes[mi].hasMaterialColor = true;

        const char* rawTex = AL_GetModelMeshTextureName(modelPath.c_str(), mi);

        if (!rawTex || rawTex[0] == '\0') continue;

        std::string tex = rawTex;
        Normalize(tex);

        std::vector<std::string> exts = { ".png", ".tga", ".jpg", ".jpeg", ".dds" };
        std::vector<std::string> candidates;

        // 元パス
        candidates.push_back(tex);

        std::string base = GetBasename(tex);
        if (base != tex) candidates.push_back(base);

        // stem 作成
        std::string stem = base;
        size_t d = stem.find_last_of('.');
        if (d != std::string::npos) stem = stem.substr(0, d);

        // ext 差し替え
        for (auto& e : exts)
            candidates.push_back(stem + e);

        // モデルのフォルダ
        std::string mdir = GetDir(modelPath);
        if (!mdir.empty())
        {
            candidates.push_back(mdir + "/" + base);
            for (auto& e : exts)
                candidates.push_back(mdir + "/" + stem + e);
        }

        // ===== ロード試行 =====
        ID3D11ShaderResourceView* loaded = nullptr;

        for (auto& c : candidates)
        {
            if (loaded) break;

            Normalize(c);

            loaded = GetTextureSRV(c.c_str());
            if (loaded) break;

            if (AL_LoadFromPackageByName(c.c_str()))
            {
                loaded = GetTextureSRV(c.c_str());
                if (loaded) break;
            }

            // 実ファイル
            {
                std::ifstream test(c, std::ios::binary);
                if (test.is_open())
                {
                    test.close();
                    if (RegisterAndLoadFileToPackage(c))
                    {
                        loaded = GetTextureSRV(c.c_str());
                        if (!loaded)
                        {
                            std::string cb = GetBasename(c);
                            loaded = GetTextureSRV(cb.c_str());
                        }
                        if (loaded) break;
                    }
                }
            }
        }

        if (loaded)
        {
            subMeshes[mi].textureSRV = loaded;
            subMeshes[mi].hasTexture = true;
        }
    }
}

void Model::Init()
{
    // nothing (SetModelPath already constructs buffers)
}

void Model::Update()
{
    world = MatSize * MatAngle * MatPos;
}

void Model::Draw()
{
    if (subMeshes.empty()) return;

    ID3D11VertexShader* vs = GetVertexShader3D();
    ID3D11PixelShader* ps = GetPixelShader3D();
    if (!vs || !ps) return;

    GetContext()->IASetInputLayout(inputLayout.Get());

    XMMATRIX mvp = world * ViewSet * ProjSet;

    UINT stride = sizeof(ModelVertex);
    UINT offset = 0;

    for (size_t i = 0; i < subMeshes.size(); ++i)
    {
        SubMesh& sm = subMeshes[i];
        if (!sm.vertexBuffer || !sm.indexBuffer || sm.indexCount == 0) continue;

        GetContext()->IASetVertexBuffers(0, 1, sm.vertexBuffer.GetAddressOf(), &stride, &offset);
        GetContext()->IASetIndexBuffer(sm.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // ===== 優先順カラー処理 =====
        XMFLOAT4 finalColor = diffuseColor;

        bool useTex = false;
        ID3D11ShaderResourceView* srv = nullptr;

        if (sm.hasTexture && sm.textureSRV)
        {
            useTex = true;
            srv = sm.textureSRV.Get();
            // Diffuse 色も乗算（ユーザー設定色含む）
            finalColor.x *= sm.hasMaterialColor ? sm.materialDiffuse.x : 1.0f;
            finalColor.y *= sm.hasMaterialColor ? sm.materialDiffuse.y : 1.0f;
            finalColor.z *= sm.hasMaterialColor ? sm.materialDiffuse.z : 1.0f;
            finalColor.w *= sm.hasMaterialColor ? sm.materialDiffuse.w : 1.0f;
        }
        else if (sm.hasMaterialColor)
        {
            // Material × User 色
            finalColor.x *= sm.materialDiffuse.x;
            finalColor.y *= sm.materialDiffuse.y;
            finalColor.z *= sm.materialDiffuse.z;
            finalColor.w *= sm.materialDiffuse.w;
        }
        else
        {
            // UserDiffuseのみ
        }

        // ===== CB 更新 =====
        MatrixBuffer cb{};
        cb.mvp = XMMatrixTranspose(mvp);
        cb.diffuseColor = finalColor;
        cb.useTexture = XMFLOAT4(useTex ? 1.0f : 0.0f, 0, 0, 0);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(GetContext()->Map(constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, &cb, sizeof(cb));
            GetContext()->Unmap(constantBuffer.Get(), 0);
        }

        GetContext()->VSSetShader(vs, nullptr, 0);
        GetContext()->PSSetShader(ps, nullptr, 0);

        GetContext()->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
        GetContext()->PSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

        if (useTex)
            GetContext()->PSSetShaderResources(0, 1, &srv);
        else
        {
            ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
            GetContext()->PSSetShaderResources(0, 1, nullSRV);
        }

        ID3D11SamplerState* samp = sampler.Get();
        GetContext()->PSSetSamplers(0, 1, &samp);

        GetContext()->DrawIndexed(sm.indexCount, 0, 0);
    }
}

void Model::Release()
{
    for (auto& sm : subMeshes) {
        sm.vertexBuffer.Reset();
        sm.indexBuffer.Reset();
        sm.textureSRV.Reset();
    }
    subMeshes.clear();

    inputLayout.Reset();
    constantBuffer.Reset();
    sampler.Reset();
}

void Model::SetPos(float x, float y, float z)
{
    MatPos = XMMatrixTranslation(x, y, z);
}
void Model::SetSize(float x, float y, float z)
{
    MatSize = XMMatrixScaling(x, y, z);
}
void Model::SetAngle(float x, float y, float z)
{
    MatAngle = XMMatrixRotationRollPitchYaw(x, y, z);
}
void Model::SetColor(float r, float g, float b, float a)
{
    diffuseColor = { r, g, b, a };
    userColorSet = true;
}
void Model::SetView(const XMMATRIX& v)
{
    ViewSet = v;
}
void Model::SetProj(const XMMATRIX& p)
{
    ProjSet = p;
}

void Model::SetTexture(const char* texName)
{
    // model-level override: set all submeshes to this texture
    texturePath = texName;
    ID3D11ShaderResourceView* tmp = GetTextureSRV(texName);
    if (!tmp) {
        // try load from package
        AL_LoadFromPackageByName(texName);
        tmp = GetTextureSRV(texName);
    }
    if (tmp) {
        for (auto& sm : subMeshes) {
            sm.textureSRV = tmp;
            sm.hasTexture = true;
        }
    }
    else {
        for (auto& sm : subMeshes) {
            sm.textureSRV.Reset();
            sm.hasTexture = false;
        }
    }
}

void Model::SetSubmeshTexture(int submeshIndex, const char* texName)
{
    if (submeshIndex < 0 || submeshIndex >= (int)subMeshes.size()) return;
    ID3D11ShaderResourceView* tmp = GetTextureSRV(texName);
    if (!tmp) {
        AL_LoadFromPackageByName(texName);
        tmp = GetTextureSRV(texName);
    }
    if (tmp) {
        subMeshes[submeshIndex].textureSRV = tmp;
        subMeshes[submeshIndex].hasTexture = true;
    }
    else {
        subMeshes[submeshIndex].textureSRV.Reset();
        subMeshes[submeshIndex].hasTexture = false;
    }
}
