#include "ComponentSpriteCylinder.h"

using Microsoft::WRL::ComPtr;

void SpriteCylinder::Init()
{
    // --------------------------
    // シェーダー読み込み
    // --------------------------
    ComPtr<ID3DBlob> vsBlob, psBlob;
    D3DCompileFromFile(L"Shader/2D_VS.hlsl", nullptr, nullptr,
        "VSMain", "vs_5_0", 0, 0, &vsBlob, nullptr);
    D3DCompileFromFile(L"Shader/2D_PS.hlsl", nullptr, nullptr,
        "PSMain", "ps_5_0", 0, 0, &psBlob, nullptr);

    GetDevice()->CreateVertexShader(
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);
    GetDevice()->CreatePixelShader(
        psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps);

    // InputLayout（SpriteWorldと同じ）
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0, D3D11_INPUT_PER_VERTEX_DATA,0 },
        { "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12, D3D11_INPUT_PER_VERTEX_DATA,0 },
    };
    GetDevice()->CreateInputLayout(layout, 2,
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_layout);

    // ConstantBuffers
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    bd.ByteWidth = sizeof(MatrixBuffer);
    GetDevice()->CreateBuffer(&bd, nullptr, &m_matrixBuf);

    bd.ByteWidth = sizeof(ColorBuffer);
    GetDevice()->CreateBuffer(&bd, nullptr, &m_colorBuf);

    // Sampler
    D3D11_SAMPLER_DESC samp{};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = samp.AddressV = samp.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    GetDevice()->CreateSamplerState(&samp, &m_sampler);

    // メッシュ生成
    BuildMesh();
}

void SpriteCylinder::SetPos(float x, float y, float z)
{
    m_pos = { x,y,z };
}
void SpriteCylinder::SetSize(float r, float h)
{
    m_size = { r, h };
    BuildMesh();
}
void SpriteCylinder::SetAngle(float rx, float ry, float rz)
{
    m_angle = { rx,ry,rz };
}
void SpriteCylinder::SetColor(float r, float g, float b, float a)
{
    m_color = { r,g,b,a };
}

void SpriteCylinder::SetSideTexture(const char* path)
{
    m_srvSide = GetTextureSRV(path);
}
void SpriteCylinder::SetTopTexture(const char* path)
{
    m_srvTop = GetTextureSRV(path);
}
void SpriteCylinder::SetBottomTexture(const char* path)
{
    m_srvBottom = GetTextureSRV(path);
}

void SpriteCylinder::SetSegment(int seg)
{
    if (seg < 3) seg = 3;
    m_seg = seg;
    BuildMesh();
}

void SpriteCylinder::SetView(const XMMATRIX& view)
{
    ViewSet = view;
}
void SpriteCylinder::SetProj(const XMMATRIX& proj)
{
    ProjSet = proj;
}

void SpriteCylinder::Draw()
{
    UINT stride = sizeof(Vertex), offset = 0;

    // 共通 WorldViewProj
    XMMATRIX world =
        XMMatrixRotationRollPitchYaw(m_angle.x, m_angle.y, m_angle.z) *
        XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);

    MatrixBuffer mb;
    mb.mvp = XMMatrixTranspose(world * ViewSet * ProjSet);
    mb.diffuseColor = m_color;
    mb.useTexture = 1;
    mb.pad = XMFLOAT3(0, 0, 0);

    ColorBuffer cb{ m_color };

    // シェーダ・定数バッファ等の共通バインド（最初に一度でOK）
    GetContext()->VSSetShader(m_vs.Get(), nullptr, 0);
    GetContext()->PSSetShader(m_ps.Get(), nullptr, 0);
    GetContext()->IASetInputLayout(m_layout.Get());
    // VS 定数バッファスロット0 に m_matrixBuf を用いる
    GetContext()->VSSetConstantBuffers(0, 1, m_matrixBuf.GetAddressOf());
    // PS 定数バッファスロット1 に m_colorBuf を用いる
    GetContext()->PSSetConstantBuffers(1, 1, m_colorBuf.GetAddressOf());
    GetContext()->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

    // DepthStencil はシーンに合わせて（ここでは既定のまま）。
    // GetContext()->OMSetDepthStencilState(nullptr, 0);

    // ----------------------------
    // 側面 (triangle strip) - 頂点は BuildMesh で (m_seg+1)*2 作ってある
    // ----------------------------
    if (m_vbSide && m_srvSide)
    {
        GetContext()->UpdateSubresource(m_matrixBuf.Get(), 0, nullptr, &mb, 0, 0);
        GetContext()->UpdateSubresource(m_colorBuf.Get(), 0, nullptr, &cb, 0, 0);

        GetContext()->PSSetShaderResources(0, 1, &m_srvSide);
        GetContext()->IASetVertexBuffers(0, 1, m_vbSide.GetAddressOf(), &stride, &offset);
        GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        // side vert count = (m_seg + 1) * 2
        UINT sideCount = (UINT)((m_seg + 1) * 2);
        GetContext()->Draw(sideCount, 0);
    }

    // ----------------------------
    // 上面 (triangle list: center + each triangle)
    // ----------------------------
    if (m_vbTop && m_srvTop)
    {
        GetContext()->UpdateSubresource(m_matrixBuf.Get(), 0, nullptr, &mb, 0, 0);
        GetContext()->UpdateSubresource(m_colorBuf.Get(), 0, nullptr, &cb, 0, 0);

        GetContext()->PSSetShaderResources(0, 1, &m_srvTop);
        GetContext()->IASetVertexBuffers(0, 1, m_vbTop.GetAddressOf(), &stride, &offset);
        GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // top triangle count = m_seg * 3 vertices (each tri is 3 verts in our buffer)
        UINT topCount = (UINT)(m_seg * 3);
        GetContext()->Draw(topCount, 0);
    }

    // ----------------------------
    // 底面 (triangle list)
    // ----------------------------
    if (m_vbBottom && m_srvBottom)
    {
        GetContext()->UpdateSubresource(m_matrixBuf.Get(), 0, nullptr, &mb, 0, 0);
        GetContext()->UpdateSubresource(m_colorBuf.Get(), 0, nullptr, &cb, 0, 0);

        GetContext()->PSSetShaderResources(0, 1, &m_srvBottom);
        GetContext()->IASetVertexBuffers(0, 1, m_vbBottom.GetAddressOf(), &stride, &offset);
        GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        UINT bottomCount = (UINT)(m_seg * 3);
        GetContext()->Draw(bottomCount, 0);
    }

    // SRVクリア（後片付け）
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    GetContext()->PSSetShaderResources(0, 1, nullSRV);
}

void SpriteCylinder::Release()
{
    m_vbSide.Reset();
    m_vbTop.Reset();
    m_vbBottom.Reset();
}

void SpriteCylinder::BuildMesh()
{
    const float r = m_size.x;
    const float h = m_size.y;
    const float halfH = h * 0.5f;

    // ----------------------------
    // 側面メッシュ（triangle strip vertices）
    // ----------------------------
    std::vector<Vertex> sideVerts;
    sideVerts.reserve((m_seg + 1) * 2);

    for (int i = 0; i <= m_seg; ++i)
    {
        float t = (float)i / m_seg;
        float theta = t * XM_2PI;
        float x = cosf(theta) * r;
        float z = sinf(theta) * r;

        // 上端 -> 下端 の順で strip を作る
        sideVerts.push_back({ {x, +halfH, z}, { t, 0.0f } });
        sideVerts.push_back({ {x, -halfH, z}, { t, 1.0f } });
    }

    // VB 作成（side）
    D3D11_BUFFER_DESC vbd{};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.ByteWidth = (UINT)(sideVerts.size() * sizeof(Vertex));
    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = sideVerts.data();

    m_vbSide.Reset();
    HRESULT hr = GetDevice()->CreateBuffer(&vbd, &init, &m_vbSide);
    if (FAILED(hr)) {
        AddMessage("CreateBuffer side failed");
    }

    // ----------------------------
    // 上面（triangle list）: center + (i, i+1) for i in [0, m_seg-1]
    // 頂点バッファには (center, v0, v1), (center, v1, v2), ... を連続で入れる
    // ----------------------------
    std::vector<Vertex> topVerts;
    topVerts.reserve(m_seg * 3);

    // precompute perimeter points
    std::vector<Vertex> perim;
    perim.reserve(m_seg + 1);
    for (int i = 0; i <= m_seg; ++i)
    {
        float t = (float)i / m_seg;
        float theta = t * XM_2PI;
        float x = cosf(theta) * r;
        float z = sinf(theta) * r;
        // UV mapped to unit square [0,1] using x,z
        float u = (x / r + 1.0f) * 0.5f;
        float v = (z / r + 1.0f) * 0.5f;
        perim.push_back({ { x, halfH, z }, { u, v } });
    }

    // center vertex (UV at center)
    Vertex centerTop = { {0.0f, halfH, 0.0f}, {0.5f, 0.5f} };

    for (int i = 0; i < m_seg; ++i)
    {
        // triangle: center, perim[i], perim[i+1]
        topVerts.push_back(centerTop);
        topVerts.push_back(perim[i]);
        topVerts.push_back(perim[i + 1]);
    }

    vbd.ByteWidth = (UINT)(topVerts.size() * sizeof(Vertex));
    init.pSysMem = topVerts.data();
    m_vbTop.Reset();
    hr = GetDevice()->CreateBuffer(&vbd, &init, &m_vbTop);
    if (FAILED(hr)) {
        AddMessage("CreateBuffer top failed");
    }

    // ----------------------------
    // 底面（triangle list）: center + (i+1, i) to flip winding for correct normal
    // ----------------------------
    std::vector<Vertex> bottomVerts;
    bottomVerts.reserve(m_seg * 3);

    Vertex centerBottom = { {0.0f, -halfH, 0.0f}, {0.5f, 0.5f} };

    // note: invert winding to face downward (so normal points -Y)
    for (int i = 0; i < m_seg; ++i)
    {
        // triangle: center, perim[i+1], perim[i]
        bottomVerts.push_back(centerBottom);
        bottomVerts.push_back(perim[i + 1]);
        bottomVerts.push_back(perim[i]);
    }

    vbd.ByteWidth = (UINT)(bottomVerts.size() * sizeof(Vertex));
    init.pSysMem = bottomVerts.data();
    m_vbBottom.Reset();
    hr = GetDevice()->CreateBuffer(&vbd, &init, &m_vbBottom);
    if (FAILED(hr)) {
        AddMessage("CreateBuffer bottom failed");
    }
}