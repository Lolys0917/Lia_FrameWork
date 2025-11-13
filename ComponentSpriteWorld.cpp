#include "AssetLoad.h"
#include "ComponentSpriteWorld.h"
#include "Main.h"

void SpriteWorld::Init()
{
    // --- シェーダー読み込み ---
    ComPtr<ID3DBlob> vsBlob, psBlob;
    D3DCompileFromFile(L"Shader/2D_VS.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, nullptr);
    D3DCompileFromFile(L"Shader/2D_PS.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, nullptr);
    GetDevice()->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);
    GetDevice()->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps);

    // --- 入力レイアウト ---
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0, D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12, D3D11_INPUT_PER_VERTEX_DATA,0},
    };
    HRESULT hr = GetDevice()->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_layout);
    if (FAILED(hr)) {
        char buf[128]; sprintf_s(buf, "CreateInputLayout failed 0x%08X", (unsigned)hr);
        MessageBoxA(nullptr, buf, "Error", MB_OK);
        return;
    }

    // --- 定数バッファ作成 ---
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    bd.ByteWidth = sizeof(MatrixBuffer);
    GetDevice()->CreateBuffer(&bd, nullptr, &m_matrixBuf);

    bd.ByteWidth = sizeof(ColorBuffer);
    GetDevice()->CreateBuffer(&bd, nullptr, &m_colorBuf);

    // Sampler
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = sampDesc.AddressV = sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    GetDevice()->CreateSamplerState(&sampDesc, &m_samplerState); // m_samplerState はメンバに追加しておく

    // Blend
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    GetDevice()->CreateBlendState(&blendDesc, &m_blendState);

    // Depth stencil: 書き込み無効 or 常に成功の設定(テスト用)
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    GetDevice()->CreateDepthStencilState(&dsDesc, &m_depthState);

    dsDesc.DepthEnable = TRUE; // テストの間は無効にする
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    GetDevice()->CreateDepthStencilState(&dsDesc, &m_noDepthState);
}

void SpriteWorld::SetTexture(const char* assetPath)
{
    // AssetManager経由で取得
    if (!assetPath || strlen(assetPath) == 0) {
        MessageBoxA(nullptr, assetPath, "SpriteWorld::SetTexture: Error Invalid", MB_OK);
        return;
    }

    m_srv = GetTextureSRV(assetPath);

    if (!m_srv)
    {
        MessageBoxA(nullptr, assetPath, "SpriteWorld: Texture not found", MB_OK);
    }
}

void SpriteWorld::SetPos(float x, float y, float z) { m_pos = { x,y,z }; }
void SpriteWorld::SetSize(float w, float h) { m_size = { w,h }; }
void SpriteWorld::SetAngle(float rx, float ry, float rz) { m_angle = { rx,ry,rz }; }
void SpriteWorld::SetColor(const XMFLOAT4& color) { m_color = color; }
void SpriteWorld::SetBillboard(bool enable) { m_isBillboard = enable; }

void SpriteWorld::Draw()
{
    if (!m_srv) 
    {
        MessageBoxA(nullptr, "SpriteWorld : Error No SRV", "Draw", MB_OK);
        return;
    }

    char buf[256];
    sprintf_s(buf, "Draw: srv=%p pos=(%f,%f,%f) size=(%f,%f) angle=(%f,%f,%f)",
        m_srv, m_pos.x, m_pos.y, m_pos.z, m_size.x, m_size.y, m_angle.x, m_angle.y, m_angle.z);
    AddMessage(buf);
    //MessageBoxA(nullptr, buf, "Draw", MB_OK);
    // 頂点設定
    float hw = m_size.x * 0.5f, hh = m_size.y * 0.5f;
    Vertex verts[4] = {
        {{-hw, hh, 0}, {0,0}},
        {{ hw, hh, 0}, {1,0}},
        {{-hw,-hh, 0}, {0,1}},
        {{ hw,-hh, 0}, {1,1}},
    };

    // 頂点バッファ生成
    if (m_vb) m_vb.Reset();
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.ByteWidth = sizeof(verts);
    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = verts;
    GetDevice()->CreateBuffer(&bd, &init, &m_vb);

    // Billboard処理（カメラ向き）
    XMMATRIX world = XMMatrixRotationRollPitchYaw(m_angle.x, m_angle.y, m_angle.z)
        * XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);

    XMMATRIX view = ViewSet;
    XMMATRIX proj = ProjSet;

    MatrixBuffer mb;
    mb.mvp = XMMatrixTranspose(world * view * proj);
    mb.diffuseColor = XMFLOAT4(1, 1, 1, 1); // 必要なら変更
    mb.useTexture = (m_srv != nullptr) ? 1 : 0;
    mb.pad = XMFLOAT3(0, 0, 0);
    GetContext()->UpdateSubresource(m_matrixBuf.Get(), 0, nullptr, &mb, 0, 0);

    ColorBuffer cb{ m_color };
    GetContext()->UpdateSubresource(m_colorBuf.Get(), 0, nullptr, &cb, 0, 0);

    // バインド
    UINT stride = sizeof(Vertex), offset = 0;
    GetContext()->IASetVertexBuffers(0, 1, m_vb.GetAddressOf(), &stride, &offset);
    GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    GetContext()->IASetInputLayout(m_layout.Get());

    GetContext()->VSSetShader(m_vs.Get(), nullptr, 0);
    GetContext()->VSSetConstantBuffers(0, 1, m_matrixBuf.GetAddressOf());

    GetContext()->PSSetShader(m_ps.Get(), nullptr, 0);
    GetContext()->PSSetConstantBuffers(1, 1, m_colorBuf.GetAddressOf());

    GetContext()->PSSetShaderResources(0, 1, &m_srv);
    GetContext()->PSSetSamplers(0, 1, &m_samplerState);
    float blendFactor[4] = { 0,0,0,0 };
    GetContext()->OMSetBlendState(m_blendState, blendFactor, 0xffffffff);
    GetContext()->OMSetDepthStencilState(m_depthState, 0);

    GetContext()->Draw(4, 0);

    //GetContext()->OMSetDepthStencilState(m_noDepthState, 0);

}

void SpriteWorld::Release()
{
    m_vb.Reset();
    m_matrixBuf.Reset();
    m_colorBuf.Reset();
    m_layout.Reset();
    m_vs.Reset();
    m_ps.Reset();
    m_srv = nullptr;
}

void SpriteWorld::SetView(const XMMATRIX& view)
{
    // ビュー行列を設定
    // 必要に応じて保存しておく

    ViewSet = view;
}
void SpriteWorld::SetProj(const XMMATRIX& proj)
{
    // 射影行列を設定
    // 必要に応じて保存しておく

    ProjSet = proj;
}