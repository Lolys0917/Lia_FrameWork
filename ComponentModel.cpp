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
        MessageBoxA(nullptr, "Model path is empty!", modelPath.c_str(), MB_OK);
        return;
    }
    // ========= モデル頂点を取得 =========
    const std::vector<ModelVertex>* vtx = GetModelVertices(modelPath.c_str());
    if (!vtx || vtx->empty())
    {
        MessageBoxA(nullptr, ("Model not loaded: " + modelPath).c_str(), "Model", MB_OK);
        return;
    }

    vertices = *vtx;
    
    if (modelType == ModelType::OBJ)
    {
        // OBJ は Y-UP、右手 → 左手へ Z反転 でほぼ一致
        for (auto& v : vertices)
        {
            v.pos.z = -v.pos.z;
            v.normal.z = -v.normal.z;
        }
    }
    else if (modelType == ModelType::FBX)
    {
        // FBXはツールによりZ-UPの場合があるので補正
        for (auto& v : vertices)
        {
            // FBX Z-UP → Y-UP → DirectX 左手系補正
            XMFLOAT3 p = v.pos;
            XMFLOAT3 n = v.normal;

            float y = p.y;
            p.y = p.z;
            p.z = y;

            float ny = n.y;
            n.y = n.z;
            n.z = -ny;

            v.pos = p;
            v.normal = n;
        }
    }

    // ========= テクスチャが存在していれば設定 =========
    // 注意: ここは「モデルファイル名 == テクスチャ名」の場合にのみ有効。
    // 実運用では SetTexture() を呼んで明示的にテクスチャを設定するほうが確実です。
    //textureSRV = GetTextureSRV("asset/hamu.png");
    //useTexture = (textureSRV != nullptr);

    //ID3D11ShaderResourceView* tmp = GetTextureSRV("asset/hamu.png");
    //if (!tmp) {
    //    MessageBoxA(nullptr, "GetTextureSRV returned NULL for asset/test.png, attempting package load...", "SetModelPath", MB_OK);
    //    // 試しにパッケージからロードを試みる（存在すれば true を返すはず）
    //    if (AL_LoadFromPackageByName("asset/hamu.png")) {
    //        tmp = GetTextureSRV("asset/hamu.png");
    //    }
    //}

    // 最終状態を MessageBox で報告（デバッグ）
    //if (tmp) {
        //MessageBoxA(nullptr, "Texture found and will be assigned to m_textureSRV", "SetModelPath", MB_OK);
    //    m_textureSRV = tmp;         // ComPtr に代入（重要）
    //    textureSRV = tmp;          // 既存の生ポインタも合わせておく（任意）
    //    useTexture = true;
    //}
    //else {
    //    //MessageBoxA(nullptr, "Texture not found (NULL). useTexture will be disabled.", "SetModelPath", MB_OK);
    //    m_textureSRV.Reset();
    //    textureSRV = nullptr;
    //    useTexture = false;
    //}

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
// Init：モデルデータ取得とバッファ生成
// --------------------------------------------
void Model::Init()
{
}

// --------------------------------------------
void Model::Update()
{

    world = MatSize * MatAngle * MatPos;
}

// --------------------------------------------
void Model::Draw()
{
    //デバッグ群
    if (!vertexBuffer) {
        MessageBoxA(nullptr, "vertexBuffer is NULL", "Model::Draw", MB_OK);
        return;
    }
    if (!indexBuffer) {
        MessageBoxA(nullptr, "indexBuffer is NULL", "Model::Draw", MB_OK);
        return;
    }
    if (!constantBuffer) {
        MessageBoxA(nullptr, "constantBuffer is NULL", "Model::Draw", MB_OK);
        return;
    }
    /*MessageBoxA(nullptr,
        ("Model vertices: " + std::to_string(vertices.size())).c_str(),
        "SetModelPath()", MB_OK);

    MessageBoxA(nullptr,
        (std::string("TextureSRV: ") + (m_textureSRV ? "OK" : "NULL")).c_str(),
        "Draw Debug", MB_OK);*/

    /*if (XMMatrixIsIdentity(world)) {
        MessageBoxA(nullptr, "World matrix unchanged!", "Model::Draw", MB_OK);
    }

    if (XMMatrixIsIdentity(ViewSet)) {
        MessageBoxA(nullptr, "View matrix not set!", "Model::Draw", MB_OK);
    }

    if (XMMatrixIsIdentity(ProjSet)) {
        MessageBoxA(nullptr, "Proj matrix not set!", "Model::Draw", MB_OK);
    }*/

    ID3D11VertexShader* vs = GetVertexShader3D();
    ID3D11PixelShader* ps = GetPixelShader3D();
    if (!vs) {
        MessageBoxA(nullptr, "Vertex Shader is NULL", "Model::Draw", MB_OK);
        return;
    }
    if (!ps) {
        MessageBoxA(nullptr, "Pixel Shader is NULL", "Model::Draw", MB_OK);
        return;
    }

    // テクスチャ確認
    if (useTexture && !m_textureSRV) {
        MessageBoxA(nullptr, "m_textureSRV is NULL (Texture not set!)", "Model::Draw", MB_OK);
    }

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
    
	//m_textureSRV = nullptr; // デバッグ用: 強制的にNULLにしてみる

    ID3D11ShaderResourceView* srv = m_textureSRV.Get();
    GetContext()->PSSetShaderResources(0, 1, &srv);

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
void Model::SetTexture(const char* texName)
{
	//テクスチャを設定することで使用フラグを立てる
	//true:テクスチャ使用 / false:テクスチャ未使用
	//false時はdiffuseColorもしくはマテリアルカラーが使用される
    texturePath = texName;
    ID3D11ShaderResourceView* tmp = GetTextureSRV(texName);
    if (tmp)
    {
        m_textureSRV = tmp;
		textureSRV = tmp;
        useTexture = true;
    }
    else
    {
        useTexture = false;
    }
}