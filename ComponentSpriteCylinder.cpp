#include "ComponentSpriteCylinder.h"
#include "Main.h" // GetDevice(), GetContext(), GetTextureSRV(), AddMessage()
#include <d3dcompiler.h>
#include <vector>
#include <cmath>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

static constexpr float TWO_PI = 2.0f * 3.14159265358979323846f;

void SpriteCylinder::Init()
{
    // Compile/load shaders (reuse 2D_VS/2D_PS as requested)
    ComPtr<ID3DBlob> vsBlob, psBlob;
    HRESULT hr = D3DCompileFromFile(L"Shader/2D_VS.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, nullptr);
    if (FAILED(hr)) { AddMessage("SpriteCylinder: VS compile failed"); return; }
    hr = D3DCompileFromFile(L"Shader/2D_PS.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, nullptr);
    if (FAILED(hr)) { AddMessage("SpriteCylinder: PS compile failed"); return; }

    GetDevice()->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);
    GetDevice()->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps);

    // Input layout: POSITION(3), TEXCOORD(2)
    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    GetDevice()->CreateInputLayout(layoutDesc, _countof(layoutDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_layout);

    // Constant buffers
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
    samp.MinLOD = 0;
    samp.MaxLOD = D3D11_FLOAT32_MAX;
    GetDevice()->CreateSamplerState(&samp, &m_sampler);

    // Blend state (enable alpha)
    D3D11_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    GetDevice()->CreateBlendState(&blendDesc, &m_blend);

    // DepthStencil: default + a no-depth-write state (we still create a normal depth state)
    D3D11_DEPTH_STENCIL_DESC dsDesc{};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    GetDevice()->CreateDepthStencilState(&dsDesc, &m_depth);

    // Build geometry
    BuildMesh();
}

void SpriteCylinder::SetPos(float x, float y, float z)
{
    m_pos = { x, y, z };
}
void SpriteCylinder::SetSize(float x, float y, float z)
{
    // API-compatible: x = radius, y = height (z ignored)
    m_size = { x, y, z };
    BuildMesh();
}
void SpriteCylinder::SetAngle(float rx, float ry, float rz)
{
    m_angle = { rx, ry, rz };
}
void SpriteCylinder::SetColor(float r, float g, float b, float a)
{
    m_color = { r, g, b, a };
}

void SpriteCylinder::SetSideTexture(const char* path)
{
    m_srvSide = GetTextureSRV(path);
    if (!m_srvSide) AddMessage(ConcatCStr("SpriteCylinder: Side texture not found: ", path));
}
void SpriteCylinder::SetTopTexture(const char* path)
{
    m_srvTop = GetTextureSRV(path);
    if (!m_srvTop) AddMessage(ConcatCStr("SpriteCylinder: Top texture not found: ", path));
}
void SpriteCylinder::SetBottomTexture(const char* path)
{
    m_srvBottom = GetTextureSRV(path);
    if (!m_srvBottom) AddMessage(ConcatCStr("SpriteCylinder: Bottom texture not found: ", path));
}

void SpriteCylinder::SetSegment(int seg)
{
    if (seg < 3) seg = 3;
    m_seg = seg;
    BuildMesh();
}

void SpriteCylinder::SetView(const XMMATRIX& view) { ViewSet = view; }
void SpriteCylinder::SetProj(const XMMATRIX& proj) { ProjSet = proj; }

void SpriteCylinder::Draw()
{
    // Safety
    if (!m_vs || !m_ps) return;

    SetSegment(m_seg);

    ID3D11DeviceContext* ctx = GetContext();

    // Calculate world-view-proj
    XMMATRIX S = XMMatrixScaling(1.0f, 1.0f, 1.0f);
    XMMATRIX R = XMMatrixRotationRollPitchYaw(m_angle.x, m_angle.y, m_angle.z);
    XMMATRIX T = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);
    XMMATRIX world = S * R * T;

    MatrixBuffer mb;
    mb.mvp = XMMatrixTranspose(world * ViewSet * ProjSet);
    mb.diffuseColor = m_color;
    mb.useTexture = 1;
    mb.pad = XMFLOAT3(0, 0, 0);

    ColorBuffer cb{ m_color };

    // Common binds
    ctx->VSSetShader(m_vs.Get(), nullptr, 0);
    ctx->PSSetShader(m_ps.Get(), nullptr, 0);
    ctx->IASetInputLayout(m_layout.Get());
    ctx->VSSetConstantBuffers(0, 1, m_matrixBuf.GetAddressOf());
    ctx->PSSetConstantBuffers(1, 1, m_colorBuf.GetAddressOf());
    ctx->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

    float blendFactor[4] = { 0,0,0,0 };
    ctx->OMSetBlendState(m_blend.Get(), blendFactor, 0xffffffff);
    ctx->OMSetDepthStencilState(m_depth.Get(), 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    // Draw Top first (triangle list)
    if (m_vbTop && m_srvTop && m_topVertexCount > 0)
    {
        ctx->UpdateSubresource(m_matrixBuf.Get(), 0, nullptr, &mb, 0, 0);
        ctx->UpdateSubresource(m_colorBuf.Get(), 0, nullptr, &cb, 0, 0);

        ctx->PSSetShaderResources(0, 1, &m_srvTop);
        ctx->IASetVertexBuffers(0, 1, m_vbTop.GetAddressOf(), &stride, &offset);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->Draw(m_topVertexCount, 0);
    }

    // Draw Bottom next
    if (m_vbBottom && m_srvBottom && m_bottomVertexCount > 0)
    {
        ctx->UpdateSubresource(m_matrixBuf.Get(), 0, nullptr, &mb, 0, 0);
        ctx->UpdateSubresource(m_colorBuf.Get(), 0, nullptr, &cb, 0, 0);

        ctx->PSSetShaderResources(0, 1, &m_srvBottom);
        ctx->IASetVertexBuffers(0, 1, m_vbBottom.GetAddressOf(), &stride, &offset);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->Draw(m_bottomVertexCount, 0);
    }

    // Draw Side last
    if (m_vbSide && m_srvSide && m_sideVertexCount > 0)
    {
        ctx->UpdateSubresource(m_matrixBuf.Get(), 0, nullptr, &mb, 0, 0);
        ctx->UpdateSubresource(m_colorBuf.Get(), 0, nullptr, &cb, 0, 0);

        ctx->PSSetShaderResources(0, 1, &m_srvSide);
        ctx->IASetVertexBuffers(0, 1, m_vbSide.GetAddressOf(), &stride, &offset);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->Draw(m_sideVertexCount, 0);
    }

    // Clear SRV slot 0
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    ctx->PSSetShaderResources(0, 1, nullSRV);
}

void SpriteCylinder::Release()
{
    m_vbSide.Reset();
    m_vbTop.Reset();
    m_vbBottom.Reset();
    m_layout.Reset();
    m_vs.Reset();
    m_ps.Reset();
    m_matrixBuf.Reset();
    m_colorBuf.Reset();
    m_sampler.Reset();
    m_blend.Reset();
    m_depth.Reset();
    m_srvSide = m_srvTop = m_srvBottom = nullptr;
    m_sideVertexCount = m_topVertexCount = m_bottomVertexCount = 0;
}

void SpriteCylinder::BuildMesh()
{
    // radius and height
    const float r = m_size.x <= 0.0f ? 1.0f : m_size.x;
    const float h = m_size.y;
    const float halfH = h * 0.5f;

    if (m_seg < 3) m_seg = 3;

    // Precompute perimeter points (m_seg+1 so last == first)
    std::vector<XMFLOAT3> perim;
    perim.reserve(m_seg + 1);
    for (int i = 0; i <= m_seg; ++i)
    {
        float t = (float)i / (float)m_seg;
        float theta = t * TWO_PI;
        float x = cosf(theta) * r;
        float z = sinf(theta) * r;
        perim.emplace_back(x, 0.0f, z);
    }

    // --- Side (triangle list) ---
    // For each segment i: two triangles (a,b,c) and (a,c,d)
    // where a = top_i, b = bottom_i, c = bottom_i+1, d = top_i+1
    std::vector<Vertex> sideVerts;
    sideVerts.reserve(m_seg * 6);
    for (int i = 0; i < m_seg; ++i)
    {
        XMFLOAT3 p0 = perim[i];
        XMFLOAT3 p1 = perim[i + 1];

        float u0 = (float)i / (float)m_seg;
        float u1 = (float)(i + 1) / (float)m_seg;

        // original positions
        Vertex top0 = { { p0.x, +halfH, p0.z }, { u0, 0.0f } };
        Vertex bot0 = { { p0.x, -halfH, p0.z }, { u0, 1.0f } };
        Vertex top1 = { { p1.x, +halfH, p1.z }, { u1, 0.0f } };
        Vertex bot1 = { { p1.x, -halfH, p1.z }, { u1, 1.0f } };

        // Triangle 1: bot1, bot0, top0   (‹t‡)
        sideVerts.push_back(bot1);
        sideVerts.push_back(bot0);
        sideVerts.push_back(top0);

        // Triangle 2: top1, bot1, top0   (‹t‡)
        sideVerts.push_back(top1);
        sideVerts.push_back(bot1);
        sideVerts.push_back(top0);
    }

    if (!sideVerts.empty())
    {
        D3D11_BUFFER_DESC vbd{};
        vbd.Usage = D3D11_USAGE_IMMUTABLE;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.ByteWidth = (UINT)(sideVerts.size() * sizeof(Vertex));
        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = sideVerts.data();
        m_vbSide.Reset();
        HRESULT hr = GetDevice()->CreateBuffer(&vbd, &init, &m_vbSide);
        if (FAILED(hr)) AddMessage("SpriteCylinder: CreateBuffer side failed");
        m_sideVertexCount = (UINT)sideVerts.size();
    }
    else
    {
        m_vbSide.Reset();
        m_sideVertexCount = 0;
    }

    // --- Top (center + triangles) ---
    std::vector<Vertex> topVerts;
    topVerts.reserve(m_seg * 3);
    Vertex centerTop{ {0.0f, +halfH, 0.0f}, {0.5f, 0.5f} };
    for (int i = 0; i < m_seg; ++i)
    {
        XMFLOAT3 p0 = perim[i];
        XMFLOAT3 p1 = perim[i + 1];
        XMFLOAT2 uv0{ (p0.x / r + 1.0f) * 0.5f, (p0.z / r + 1.0f) * 0.5f };
        XMFLOAT2 uv1{ (p1.x / r + 1.0f) * 0.5f, (p1.z / r + 1.0f) * 0.5f };

        topVerts.push_back(centerTop);
        topVerts.push_back({ { p1.x, +halfH, p1.z }, uv1 });
        topVerts.push_back({ { p0.x, +halfH, p0.z }, uv0 });
    }
    if (!topVerts.empty())
    {
        D3D11_BUFFER_DESC vbd{};
        vbd.Usage = D3D11_USAGE_IMMUTABLE;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.ByteWidth = (UINT)(topVerts.size() * sizeof(Vertex));
        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = topVerts.data();
        m_vbTop.Reset();
        HRESULT hr = GetDevice()->CreateBuffer(&vbd, &init, &m_vbTop);
        if (FAILED(hr)) AddMessage("SpriteCylinder: CreateBuffer top failed");
        m_topVertexCount = (UINT)topVerts.size();
    }
    else
    {
        m_vbTop.Reset();
        m_topVertexCount = 0;
    }

    // --- Bottom (center + triangles, reversed winding) ---
    std::vector<Vertex> bottomVerts;
    bottomVerts.reserve(m_seg * 3);
    Vertex centerBottom{ {0.0f, -halfH, 0.0f}, {0.5f, 0.5f} };
    for (int i = 0; i < m_seg; ++i)
    {
        XMFLOAT3 p0 = perim[i];
        XMFLOAT3 p1 = perim[i + 1];
        XMFLOAT2 uv0{ (p0.x / r + 1.0f) * 0.5f, (p0.z / r + 1.0f) * 0.5f };
        XMFLOAT2 uv1{ (p1.x / r + 1.0f) * 0.5f, (p1.z / r + 1.0f) * 0.5f };

        // reverse order to flip normal downward
        bottomVerts.push_back(centerBottom);
        bottomVerts.push_back({ { p0.x, -halfH, p0.z }, uv0 });
        bottomVerts.push_back({ { p1.x, -halfH, p1.z }, uv1 });
    }
    if (!bottomVerts.empty())
    {
        D3D11_BUFFER_DESC vbd{};
        vbd.Usage = D3D11_USAGE_IMMUTABLE;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.ByteWidth = (UINT)(bottomVerts.size() * sizeof(Vertex));
        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = bottomVerts.data();
        m_vbBottom.Reset();
        HRESULT hr = GetDevice()->CreateBuffer(&vbd, &init, &m_vbBottom);
        if (FAILED(hr)) AddMessage("SpriteCylinder: CreateBuffer bottom failed");
        m_bottomVertexCount = (UINT)bottomVerts.size();
    }
    else
    {
        m_vbBottom.Reset();
        m_bottomVertexCount = 0;
    }
}
