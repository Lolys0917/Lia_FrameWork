#include "ComponentModel.h"
#include "Manager.h"
#include "AssetLoad.h"

Model::~Model()
{
    Release();
}

// --------------------------------------------
// Model パス設定（拡張子取得）
// --------------------------------------------
void Model::SetModelPath(const char* filename)
{
    modelPath = "asset/girl_model.fbx";

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

    Init();
}

// --------------------------------------------
// Init：モデルデータ取得とバッファ生成
// --------------------------------------------
void Model::Init()
{
    if (modelPath.empty())
    {
        MessageBoxA(nullptr, "Model path is empty!", modelPath.c_str(), MB_OK);
        return;
    }
    m_textureSRV = GetTextureSRV("asset/hamu.png");
    // ========= モデル頂点を取得 =========
    const std::vector<ModelVertex>* vtx = GetModelVertices(modelPath.c_str());
    if (!vtx || vtx->empty())
    {
        MessageBoxA(nullptr, ("Model not loaded: " + modelPath).c_str(), "Model", MB_OK);
        return;
    }

    vertices = *vtx;

    // ========= テクスチャが存在していれば設定 =========
    // 注意: ここは「モデルファイル名 == テクスチャ名」の場合にのみ有効。
    // 実運用では SetTexture() を呼んで明示的にテクスチャを設定するほうが確実です。
    textureSRV = GetTextureSRV(modelPath.c_str());
    useTexture = (textureSRV != nullptr);

    // ========= 頂点バッファ =========
    D3D11_BUFFER_DESC vbd{};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = (UINT)(sizeof(ModelVertex) * vertices.size());
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vinit{};
    vinit.pSysMem = vertices.data();

    HRESULT hr = GetDevice()->CreateBuffer(&vbd, &vinit, vertexBuffer.GetAddressOf());
    if (FAILED(hr)) {
        AddMessage("Model::Init - CreateBuffer(vertex) failed");
        return;
    }

    // ========= インデックス作成（トライアングルリスト） =========
    indices.resize(vertices.size());
    for (UINT i = 0; i < (UINT)vertices.size(); i++) indices[i] = i;

    indexCount = (UINT)indices.size();

    if (indexCount > 0)
    {
        D3D11_BUFFER_DESC ibd{};
        ibd.Usage = D3D11_USAGE_DEFAULT;
        ibd.ByteWidth = (UINT)(sizeof(UINT) * indexCount);
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA iinit{};
        iinit.pSysMem = indices.data();

        hr = GetDevice()->CreateBuffer(&ibd, &iinit, indexBuffer.GetAddressOf());
        if (FAILED(hr)) {
            AddMessage("Model::Init - CreateBuffer(index) failed");
            return;
        }
    }

    // ========= シェーダーとレイアウト取得 =========
    ID3D11VertexShader* vs = GetVertexShader3D();
    ID3D11PixelShader* ps = GetPixelShader3D();
    ID3DBlob* vsBlob = GetCurrent3DVSBlob();

    if (!vs || !ps || !vsBlob)
    {
        MessageBoxA(nullptr, "3D shader not ready", "Model Init", MB_OK);
        return;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,  D3D11_INPUT_PER_VERTEX_DATA,0},
        {"NORMAL",  0,DXGI_FORMAT_R32G32B32_FLOAT,0,12, D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,    0,24, D3D11_INPUT_PER_VERTEX_DATA,0},
    };

    hr = GetDevice()->CreateInputLayout(
        layout, 3,
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        inputLayout.GetAddressOf()
    );
    if (FAILED(hr)) {
        AddMessage("Model::Init - CreateInputLayout failed");
        return;
    }

    // ========= コンスタントバッファ（1 回だけ作成） =========
    D3D11_BUFFER_DESC cbd{};
    cbd.Usage = D3D11_USAGE_DYNAMIC; // UpdateSubresourceでもMapでも扱えるように動的に
    cbd.ByteWidth = sizeof(MatrixBuffer);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = GetDevice()->CreateBuffer(&cbd, nullptr, constantBuffer.GetAddressOf());
    if (FAILED(hr)) {
        AddMessage("Model::Init - CreateBuffer(constant) failed");
        return;
    }

    // ========= サンプラ（1回だけ） =========
    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;

    hr = GetDevice()->CreateSamplerState(&sd, sampler.GetAddressOf());
    if (FAILED(hr)) {
        AddMessage("Model::Init - CreateSamplerState failed");
        return;
    }
}

// --------------------------------------------
void Model::Update()
{
    world = MatSize * MatAngle * MatPos;
}

// --------------------------------------------
void Model::Draw()
{
    if (!vertexBuffer || !indexBuffer || !constantBuffer) return;

    ID3D11VertexShader* vs = GetVertexShader3D();
    ID3D11PixelShader* ps = GetPixelShader3D();

    if (!vs || !ps) return;

    // ===== InputLayout =====
    GetContext()->IASetInputLayout(inputLayout.Get());

    UINT stride = sizeof(ModelVertex);
    UINT offset = 0;

    GetContext()->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    GetContext()->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // ===== ConstantBuffer =====
    MatrixBuffer cb{};
    XMMATRIX mvp = world * ViewSet * ProjSet;
    cb.mvp = XMMatrixTranspose(mvp);
    cb.diffuseColor = diffuseColor;
    cb.useTexture = (useTexture ? 1 : 0);
    cb.pad = { 0,0,0 };

    // Map & copy (dynamic buffer)
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(GetContext()->Map(constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, &cb, sizeof(cb));
        GetContext()->Unmap(constantBuffer.Get(), 0);
    }
    else
    {
        // fallback to UpdateSubresource (shouldn't usually happen with DYNAMIC+WRITE_DISCARD)
        GetContext()->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    }

    GetContext()->VSSetShader(vs, nullptr, 0);
    GetContext()->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

    GetContext()->PSSetShader(ps, nullptr, 0);
    GetContext()->PSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

    // ===== Texture =====
    if (useTexture && textureSRV)
    {
        ID3D11ShaderResourceView* srv = m_textureSRV.Get(); // raw ptr
        GetContext()->PSSetShaderResources(0, 1, &srv);
    }
    else
    {
        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        GetContext()->PSSetShaderResources(0, 1, nullSRV);
    }

    ID3D11SamplerState* samp = sampler.Get();
    GetContext()->PSSetSamplers(0, 1, &samp);

    // ===== Draw =====
    GetContext()->DrawIndexed(indexCount, 0, 0);
}

// --------------------------------------------
void Model::Release()
{
    vertexBuffer.Reset();
    indexBuffer.Reset();
    inputLayout.Reset();
    constantBuffer.Reset();
    sampler.Reset();
    m_textureSRV.Reset();

    vertices.clear();
    indices.clear();
}

// --------------------------------------------
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
}
void Model::SetView(const XMMATRIX& v)
{
    ViewSet = v;
}
void Model::SetProj(const XMMATRIX& p)
{
    ProjSet = p;
}
