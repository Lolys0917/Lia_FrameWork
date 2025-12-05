// ComponentModel.cpp
// Model コンポーネント。AssetManager のサブメッシュ/マテリアル情報を使って GPU バッファを作る。
// Model::SetModelPath 関連の全文を含む差し替え版。

#include "ComponentModel.h"
#include "Manager.h"
#include "AssetLoad.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <assimp/scene.h>
namespace fs = std::filesystem;

Model::~Model()
{
    Release();
}

void Model::SetModelPath(const char* filename)
{
    modelPath = filename ? filename : std::string();

    std::string s = modelPath;
    auto dot = s.find_last_of('.');
    if (dot != std::string::npos)
    {
        std::string ext = s.substr(dot + 1);
        for (auto& c : ext) c = ::tolower((unsigned char)c);

        if (ext == "fbx") modelType = ModelType::FBX;
        else modelType = ModelType::OBJ;
    }
    else modelType = ModelType::OBJ;

    if (modelPath.empty())
    {
        MessageBoxA(nullptr, "Model path is empty!", "Model::SetModelPath", MB_OK);
        return;
    }

    // サブメッシュ数を AssetManager から取得
    int meshCount = AL_GetModelMeshCount(modelPath.c_str());
    if (meshCount <= 0)
    {
        MessageBoxA(nullptr, ("Model not loaded or no meshes: " + modelPath).c_str(), "Model::SetModelPath", MB_OK);
        return;
    }

    // 既存リソースを解放して初期化
    Release();
    subMeshes.clear();
    subMeshes.resize(meshCount);

    // シェーダー / 入力レイアウト / 定数バッファ / サンプラを作成
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
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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

    // サンプラは CLAMP にしておく（キャラクタ系テクスチャのタイリング回避）
    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    hr = GetDevice()->CreateSamplerState(&sd, sampler.GetAddressOf());
    if (FAILED(hr)) {
        MessageBoxA(nullptr, "CreateSamplerState failed", "Model::SetModelPath", MB_OK);
        return;
    }

    totalIndexCount = 0;

    // 各サブメッシュごとに VB / IB を作成し、マテリアル情報 -> テクスチャをセット
    for (int mi = 0; mi < meshCount; ++mi)
    {
        const std::vector<ModelVertex>* vtx = AL_GetModelMeshVertices(modelPath.c_str(), mi);
        const std::vector<unsigned int>* idx = AL_GetModelMeshIndices(modelPath.c_str(), mi);

        if (!vtx || vtx->empty() || !idx || idx->empty()) {
            // empty サブメッシュならスキップ
            continue;
        }

        // 頂点バッファ
        D3D11_BUFFER_DESC vbd{};
        vbd.Usage = D3D11_USAGE_DEFAULT;
        vbd.ByteWidth = (UINT)(sizeof(ModelVertex) * vtx->size());
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vinit{};
        vinit.pSysMem = vtx->data();

        /*for (int k = 0; k < 10 && k < vtx->size(); ++k) {
            char msg[256];
            sprintf_s(msg, "VTX[%d] U=%.3f  V=%.3f", k, vtx->at(k).uv.x, vtx->at(k).uv.y);
            MessageBoxA(NULL, msg, "CHECK UV", MB_OK);
        }*/

        hr = GetDevice()->CreateBuffer(&vbd, &vinit, subMeshes[mi].vertexBuffer.GetAddressOf());
        if (FAILED(hr)) {
            MessageBoxA(nullptr, "CreateBuffer(vertex) failed", "Model::SetModelPath", MB_OK);
            continue;
        }

        // インデックスバッファ
        D3D11_BUFFER_DESC ibd{};
        ibd.Usage = D3D11_USAGE_DEFAULT;
        ibd.ByteWidth = (UINT)(sizeof(unsigned int) * idx->size());
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA iinit{};
        iinit.pSysMem = idx->data();

        hr = GetDevice()->CreateBuffer(&ibd, &iinit, subMeshes[mi].indexBuffer.GetAddressOf());
        if (FAILED(hr)) {
            MessageBoxA(nullptr, "CreateBuffer(index) failed", "Model::SetModelPath", MB_OK);
            continue;
        }

        subMeshes[mi].indexCount = (UINT)idx->size();
        totalIndexCount += subMeshes[mi].indexCount;

        // マテリアル色を取得
        XMFLOAT4 matCol = AL_GetModelMeshMaterialDiffuse(modelPath.c_str(), mi);
        subMeshes[mi].materialDiffuse = matCol;
        subMeshes[mi].hasMaterialColor = true;

        // テクスチャ（マテリアルにセットされていれば GetTextureSRV で取得）
        const char* texName = AL_GetModelMeshTextureName(modelPath.c_str(), mi);
        if (texName && texName[0] != '\0') {
            ID3D11ShaderResourceView* srv = GetTextureSRV(texName);
            if (srv) {
                subMeshes[mi].textureSRV = srv;
                subMeshes[mi].hasTexture = true;
            }
            else {
                // パッケージからロードして再試行
                AL_LoadFromPackageByName(texName);
                ID3D11ShaderResourceView* srv2 = GetTextureSRV(texName);
                if (srv2) {
                    subMeshes[mi].textureSRV = srv2;
                    subMeshes[mi].hasTexture = true;
                }
                else {
                    // 試しにベース名でのロードを試す（AssetManager が basename を登録した可能性）
                    std::string base = fs::path(texName).filename().string();
                    if (!base.empty()) {
                        AL_LoadFromPackageByName(base.c_str());
                        ID3D11ShaderResourceView* srv3 = GetTextureSRV(base.c_str());
                        if (srv3) {
                            subMeshes[mi].textureSRV = srv3;
                            subMeshes[mi].hasTexture = true;
                        }
                    }
                }
            }
        }
    }

    // ※補正: Assimp 側で left-handed 変換を行っているため、ここでは追加の座標変換は行わない。
    // もしモデルがまだ反転・オフセットしている場合は AssetManager の読み込みオプションを調整してください。
}

void Model::Init()
{
    // nothing (SetModelPath already constructs buffers)
}

void Model::Update()
{
    MatPos = XMMatrixTranslation(GetPosition().x, GetPosition().y, GetPosition().z);
    MatSize = XMMatrixScaling(GetSize().x, GetSize().y, GetSize().z);
    MatAngle = XMMatrixRotationRollPitchYaw(GetAngle().x, GetAngle().y, GetAngle().z);

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
            finalColor.x *= sm.hasMaterialColor ? sm.materialDiffuse.x : 1.0f;
            finalColor.y *= sm.hasMaterialColor ? sm.materialDiffuse.y : 1.0f;
            finalColor.z *= sm.hasMaterialColor ? sm.materialDiffuse.z : 1.0f;
            finalColor.w *= sm.hasMaterialColor ? sm.materialDiffuse.w : 1.0f;
        }
        else if (sm.hasMaterialColor)
        {
            finalColor.x *= sm.materialDiffuse.x;
            finalColor.y *= sm.materialDiffuse.y;
            finalColor.z *= sm.materialDiffuse.z;
            finalColor.w *= sm.materialDiffuse.w;
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
    texturePath = texName;
    ID3D11ShaderResourceView* tmp = GetTextureSRV(texName);
    if (!tmp) {
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
