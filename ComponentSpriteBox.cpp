// SpriteBox.cpp
#include "ComponentSpriteBox.h"
#include "Main.h" // GetDevice(), GetContext(), GetTextureSRV(), AddMessage()
#include <d3dcompiler.h>
#include <cmath>
#include <vector>
#include <unordered_map>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// store per-instance depth since header m_size is XMFLOAT2
static std::unordered_map<const SpriteBox*, float> s_instanceDepth;

SpriteBox::~SpriteBox()
{
    Release();
    s_instanceDepth.erase(this);
}

void SpriteBox::Init()
{
    // compile shaders
    ComPtr<ID3DBlob> vsBlob, psBlob;
    HRESULT hr = D3DCompileFromFile(L"Shader/2D_VS.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, nullptr);
    if (FAILED(hr)) { AddMessage("SpriteBox: VS compile failed"); return; }
    hr = D3DCompileFromFile(L"Shader/2D_PS.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, nullptr);
    if (FAILED(hr)) { AddMessage("SpriteBox: PS compile failed"); return; }

    ID3D11Device* dev = GetDevice();

    dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);
    dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps);

    // input layout: position(3), uv(2)
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0, D3D11_INPUT_PER_VERTEX_DATA,0 },
        { "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12, D3D11_INPUT_PER_VERTEX_DATA,0 },
    };
    dev->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_layout);

    // constant buffers
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.ByteWidth = sizeof(MatrixBuffer);
    dev->CreateBuffer(&bd, nullptr, &m_matrixBuf);

    bd.ByteWidth = sizeof(ColorBuffer);
    dev->CreateBuffer(&bd, nullptr, &m_colorBuf);

    // sampler (wrap)
    D3D11_SAMPLER_DESC samp{};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = samp.AddressV = samp.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samp.MinLOD = 0;
    samp.MaxLOD = D3D11_FLOAT32_MAX;
    dev->CreateSamplerState(&samp, &m_samplerState);

    // blend (standard alpha)
    D3D11_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    dev->CreateBlendState(&blendDesc, &m_blendState);

    // depth stencil: enable depth test & write
    D3D11_DEPTH_STENCIL_DESC dsDesc{};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dev->CreateDepthStencilState(&dsDesc, &m_depthState);

    // build initial mesh (uses current m_size and stored depth)
    BuildMesh();
}

void SpriteBox::SetTextureTop(const char* assetPath) { m_srvTop = GetTextureSRV(assetPath); if (!m_srvTop) AddMessage(ConcatCStr("TextureNotFound(Top):", assetPath)); }
void SpriteBox::SetTextureBottom(const char* assetPath) { m_srvBottom = GetTextureSRV(assetPath); if (!m_srvBottom) AddMessage(ConcatCStr("TextureNotFound(Bottom):", assetPath)); }
void SpriteBox::SetTextureFront(const char* assetPath) { m_srvFront = GetTextureSRV(assetPath); if (!m_srvFront) AddMessage(ConcatCStr("TextureNotFound(Front):", assetPath)); }
void SpriteBox::SetTextureRear(const char* assetPath) { m_srvRear = GetTextureSRV(assetPath); if (!m_srvRear) AddMessage(ConcatCStr("TextureNotFound(Rear):", assetPath)); }
void SpriteBox::SetTextureLeft(const char* assetPath) { m_srvLeft = GetTextureSRV(assetPath); if (!m_srvLeft) AddMessage(ConcatCStr("TextureNotFound(Left):", assetPath)); }
void SpriteBox::SetTextureRight(const char* assetPath) { m_srvRight = GetTextureSRV(assetPath); if (!m_srvRight) AddMessage(ConcatCStr("TextureNotFound(Right):", assetPath)); }

void SpriteBox::SetPos(float x, float y, float z) { m_pos = { x,y,z }; }
void SpriteBox::SetAngle(float x, float y, float z) { m_angle = { x,y,z }; }
void SpriteBox::SetColor(float r, float g, float b, float a) { m_color = { r,g,b,a }; }

void SpriteBox::SetSize(float x, float y, float z)
{
    // header has m_size as XMFLOAT2; use x->m_size.x, y->m_size.y, store depth separately
    m_size.x = x;
    m_size.y = y;
    s_instanceDepth[this] = z;
    BuildMesh();
}

void SpriteBox::SetView(const XMMATRIX& view) { ViewSet = view; }
void SpriteBox::SetProj(const XMMATRIX& proj) { ProjSet = proj; }

void SpriteBox::BuildMesh()
{
    // Build six faces as separate vertex buffers (each face: 2 triangles -> 6 verts)
    float halfW = m_size.x * 0.5f;
    float halfH = m_size.y * 0.5f;
    float halfD = 0.5f;
    auto it = s_instanceDepth.find(this);
    if (it != s_instanceDepth.end()) halfD = it->second * 0.5f;
    else halfD = 0.5f; // default depth 1.0 if not set

    // Vertex layout: position, uv
    struct V { XMFLOAT3 p; XMFLOAT2 uv; };

    // Helper to create a face VB given 4 corner positions (clockwise when looking at outside)
    auto createFaceVB = [&](const XMFLOAT3& p00, const XMFLOAT3& p10, const XMFLOAT3& p11, const XMFLOAT3& p01) -> ComPtr<ID3D11Buffer> {
        // UVs: p00 -> (0,0), p10 -> (1,0), p11 -> (1,1), p01 -> (0,1)
        V verts[6] = {
            { p00, {0.0f, 0.0f} }, { p10, {1.0f, 0.0f} }, { p11, {1.0f, 1.0f} },
            { p00, {0.0f, 0.0f} }, { p11, {1.0f, 1.0f} }, { p01, {0.0f, 1.0f} }
        };
        D3D11_BUFFER_DESC bd{};
        bd.Usage = D3D11_USAGE_IMMUTABLE;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.ByteWidth = sizeof(verts);
        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = verts;
        ComPtr<ID3D11Buffer> vb;
        HRESULT hr = GetDevice()->CreateBuffer(&bd, &init, &vb);
        if (FAILED(hr)) { AddMessage("SpriteBox: CreateBuffer face failed"); return nullptr; }
        return vb;
        };

    XMFLOAT3 tlb = { -halfW, +halfH, -halfD }; // top-left-back
    XMFLOAT3 trb = { +halfW, +halfH, -halfD }; // top-right-back
    XMFLOAT3 blb = { -halfW, -halfH, -halfD };
    XMFLOAT3 brb = { +halfW, -halfH, -halfD };

    XMFLOAT3 tlf = { -halfW, +halfH, +halfD }; // top-left-front
    XMFLOAT3 trf = { +halfW, +halfH, +halfD };
    XMFLOAT3 blf = { -halfW, -halfH, +halfD };
    XMFLOAT3 brf = { +halfW, -halfH, +halfD };

    m_vbTop.Reset();
    m_vbTop = createFaceVB(tlf, trf, trb, tlb);
    m_topVertexCount = m_vbTop ? 6U : 0U;

    m_vbBottom.Reset();
    m_vbBottom = createFaceVB(blb, brb, brf, blf);
    m_bottomVertexCount = m_vbBottom ? 6U : 0U;

    m_vbFront.Reset();
    m_vbFront = createFaceVB(tlf, trf, brf, blf);

    m_vbRear.Reset();
    m_vbRear = createFaceVB(trb, tlb, blb, brb);

    m_vbLeft.Reset();
    m_vbLeft = createFaceVB(tlb, tlf, blf, blb);

    m_vbRight.Reset();
    m_vbRight = createFaceVB(trf, trb, brb, brf);
}

void SpriteBox::Draw()
{
    ID3D11DeviceContext* ctx = GetContext();
    if (!ctx) return;

    // Prepare matrix
    XMMATRIX world =
        XMMatrixRotationRollPitchYaw(m_angle.x, m_angle.y, m_angle.z) *
        XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);

    MatrixBuffer mb;
    mb.mvp = XMMatrixTranspose(world * ViewSet * ProjSet);
    mb.diffuseColor = m_color;
    mb.useTexture = 1;
    mb.pad = XMFLOAT3(0, 0, 0);

    ColorBuffer cb{ m_color };

    // common binds
    ctx->VSSetShader(m_vs.Get(), nullptr, 0);
    ctx->PSSetShader(m_ps.Get(), nullptr, 0);
    ctx->IASetInputLayout(m_layout.Get());
    ctx->VSSetConstantBuffers(0, 1, m_matrixBuf.GetAddressOf());
    ctx->PSSetConstantBuffers(1, 1, m_colorBuf.GetAddressOf());
    ctx->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
    float blendFactor[4] = { 0,0,0,0 };
    ctx->OMSetBlendState(m_blendState.Get(), blendFactor, 0xffffffff);
    ctx->OMSetDepthStencilState(m_depthState.Get(), 0);

    // Update constant buffers
    ctx->UpdateSubresource(m_matrixBuf.Get(), 0, nullptr, &mb, 0, 0);
    ctx->UpdateSubresource(m_colorBuf.Get(), 0, nullptr, &cb, 0, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    // Draw order: Top -> Bottom -> Front -> Rear -> Left -> Right
    // For each face: bind texture, set VB, draw 6 verts (triangle list)

    auto drawFace = [&](ComPtr<ID3D11Buffer>& vb, ID3D11ShaderResourceView* srv) {
        if (!vb || !srv) return;
        // bind srv
        ctx->PSSetShaderResources(0, 1, &srv);
        // bind vb
        UINT faceStride = sizeof(Vertex);
        UINT faceOffset = 0;
        ctx->IASetVertexBuffers(0, 1, vb.GetAddressOf(), &faceStride, &faceOffset);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->Draw(6, 0);
        // clear srv
        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        ctx->PSSetShaderResources(0, 1, nullSRV);
        };

    // We need Vertex structure type for stride; reuse same as header's Vertex (pos, uv).
    // But we didn't declare Vertex type in this cpp; header declared SpriteBox::Vertex. Use that.
    // Ensure stride = sizeof(SpriteBox::Vertex)
    // Recompute stride accordingly:
    stride = sizeof(SpriteBox::Vertex);
    offset = 0;

    // Top
    if (m_vbTop && m_srvTop) {
        ctx->IASetVertexBuffers(0, 1, m_vbTop.GetAddressOf(), &stride, &offset);
        ctx->PSSetShaderResources(0, 1, &m_srvTop);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->Draw(6, 0);
        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        ctx->PSSetShaderResources(0, 1, nullSRV);
    }

    // Bottom
    if (m_vbBottom && m_srvBottom) {
        ctx->IASetVertexBuffers(0, 1, m_vbBottom.GetAddressOf(), &stride, &offset);
        ctx->PSSetShaderResources(0, 1, &m_srvBottom);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->Draw(6, 0);
        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        ctx->PSSetShaderResources(0, 1, nullSRV);
    }

    // Front
    if (m_vbFront && m_srvFront) {
        ctx->IASetVertexBuffers(0, 1, m_vbFront.GetAddressOf(), &stride, &offset);
        ctx->PSSetShaderResources(0, 1, &m_srvFront);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->Draw(6, 0);
        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        ctx->PSSetShaderResources(0, 1, nullSRV);
    }

    // Rear
    if (m_vbRear && m_srvRear) {
        ctx->IASetVertexBuffers(0, 1, m_vbRear.GetAddressOf(), &stride, &offset);
        ctx->PSSetShaderResources(0, 1, &m_srvRear);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->Draw(6, 0);
        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        ctx->PSSetShaderResources(0, 1, nullSRV);
    }

    // Left
    if (m_vbLeft && m_srvLeft) {
        ctx->IASetVertexBuffers(0, 1, m_vbLeft.GetAddressOf(), &stride, &offset);
        ctx->PSSetShaderResources(0, 1, &m_srvLeft);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->Draw(6, 0);
        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        ctx->PSSetShaderResources(0, 1, nullSRV);
    }

    // Right
    if (m_vbRight && m_srvRight) {
        ctx->IASetVertexBuffers(0, 1, m_vbRight.GetAddressOf(), &stride, &offset);
        ctx->PSSetShaderResources(0, 1, &m_srvRight);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->Draw(6, 0);
        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        ctx->PSSetShaderResources(0, 1, nullSRV);
    }

    // restore defaults if needed (not strictly necessary here)
}

void SpriteBox::Release()
{
    m_vbTop.Reset();
    m_vbBottom.Reset();
    m_vbFront.Reset();
    m_vbRear.Reset();
    m_vbLeft.Reset();
    m_vbRight.Reset();

    m_matrixBuf.Reset();
    m_colorBuf.Reset();
    m_layout.Reset();
    m_vs.Reset();
    m_ps.Reset();
    m_samplerState.Reset();
    m_blendState.Reset();
    m_depthState.Reset();

    m_srvTop = m_srvBottom = m_srvFront = m_srvRear = m_srvLeft = m_srvRight = nullptr;
}
