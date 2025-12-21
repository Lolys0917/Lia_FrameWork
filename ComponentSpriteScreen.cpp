#include "ComponentSpriteScreen.h"
#include "Main.h"
#include <d3dcompiler.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// -----------------------------------------------------------
// 初期化
// -----------------------------------------------------------
void SpriteScreen::Init()
{
    // === エンジンのシェーダー管理から取得 ===
    m_vs = GetVertexShader2D();
    m_ps = GetPixelShader2D();

    if (!m_vs || !m_ps)
    {
        MessageBoxA(0, "SpriteScreen: Default shaders not ready", "ERROR", MB_OK);
        return;
    }

    // === 入力レイアウトを作成 ===
    // ♠ 必要なのは「VS のバイトコード」だが、ShaderManager では g_Default2DVSBlob を保持している

    ID3DBlob* vsBlob = GetCurrent2DVSBlob();
    if (!vsBlob)
    {
        MessageBoxA(nullptr, "SpriteScreen: VS Blob is NULL", "ERROR", MB_OK);
        return;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,  D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,   0,12, D3D11_INPUT_PER_VERTEX_DATA,0},
    };

    GetDevice()->CreateInputLayout(
        layout,
        2,
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &m_layout
    );

    // --- 定数バッファ ---
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.ByteWidth = sizeof(MatrixBuffer);
    GetDevice()->CreateBuffer(&bd, nullptr, &m_matrixBuf);

	// --- カラーバッファ ---
    bd.ByteWidth = sizeof(ColorBuffer);
    GetDevice()->CreateBuffer(&bd, nullptr, &m_colorBuf);

    // --- サンプラー ---
    D3D11_SAMPLER_DESC samp{};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = samp.AddressV = samp.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    GetDevice()->CreateSamplerState(&samp, &m_sampler);

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
}


// -----------------------------------------------------------
// テクスチャ設定
// -----------------------------------------------------------
void SpriteScreen::SetTexture(const char* path)
{
    m_srv = GetTextureSRV(path);
    if (!m_srv)
    {
        MessageBoxA(nullptr, path, "SpriteScreen: Texture not found", MB_OK);
    }
}

// -----------------------------------------------------------
// 各種パラメータ設定
// -----------------------------------------------------------
void SpriteScreen::SetPos2D(float x, float y)
{
    m_pos = { x, y, 0 };
}
void SpriteScreen::SetSize2D(float w, float h)
{
    m_size = { w, h, 1 };
}
void SpriteScreen::SetColor(float r, float g, float b, float a)
{
    m_color = { r, g, b, a };
}

// -----------------------------------------------------------
// 描画
// -----------------------------------------------------------
void SpriteScreen::Draw()
{

    if (!m_visible || !m_srv) return;

    // --- 頂点データ作成 ---
    float x = m_pos.x;
    float y = m_pos.y;
    float w = m_size.x;
    float h = m_size.y;

    VertexScreen verts[6] = {
        {{x,     y,     0}, {0,0}},
        {{x + w, y,     0}, {1,0}},
        {{x,     y + h, 0}, {0,1}},
        {{x + w, y,     0}, {1,0}},
        {{x + w, y + h, 0}, {1,1}},
        {{x,     y + h, 0}, {0,1}},
    };

    // --- 頂点バッファ作成 ---
    D3D11_BUFFER_DESC vbd{};
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.ByteWidth = sizeof(verts);

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = verts;

    if (m_vb) m_vb.Reset();
    GetDevice()->CreateBuffer(&vbd, &initData, &m_vb);

    // --- 射影行列（スクリーン座標）---
    float width = (float)800;
    float height = (float)600;
    XMMATRIX ortho = XMMatrixOrthographicOffCenterLH(0.0f, width, height, 0.0f, 0.0f, 1.0f);

    MatrixBuffer mb;
    mb.mvp = XMMatrixTranspose(ortho);
    mb.diffuseColor = m_color;
    mb.useTexture = XMFLOAT4(1, 0, 0, 0);
    GetContext()->UpdateSubresource(m_matrixBuf.Get(), 0, nullptr, &mb, 0, 0);
    
    ColorBuffer cb{ m_color };
    GetContext()->UpdateSubresource(m_colorBuf.Get(), 0, nullptr, &cb, 0, 0);
    // --- バインド設定 ---
    UINT stride = sizeof(VertexScreen), offset = 0;
    GetContext()->IASetVertexBuffers(0, 1, m_vb.GetAddressOf(), &stride, &offset);
    GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    GetContext()->IASetInputLayout(m_layout.Get());

    GetContext()->VSSetShader(GetVertexShader2D(), nullptr, 0);
    GetContext()->VSSetConstantBuffers(0, 1, m_matrixBuf.GetAddressOf());

    GetContext()->PSSetShader(GetPixelShader2D(), nullptr, 0);
    GetContext()->PSSetConstantBuffers(1, 1, m_colorBuf.GetAddressOf());
    GetContext()->PSSetShaderResources(0, 1, &m_srv);
    GetContext()->PSSetSamplers(0, 1, m_sampler.GetAddressOf()); // ← UI専用サンプラー設定
    float blendFactor[4] = { 0,0,0,0 };
    GetContext()->OMSetBlendState(m_blendState, blendFactor, 0xffffffff);

    // --- 深度ステンシル無効化 ---
    ID3D11DepthStencilState* prevDepth = nullptr;
    UINT stencilRef = 0;
    GetContext()->OMGetDepthStencilState(&prevDepth, &stencilRef);
    GetContext()->OMSetDepthStencilState(nullptr, 0);


    // --- 描画 ---
    GetContext()->Draw(6, 0);

    // --- 深度を復帰 ---
    GetContext()->OMSetDepthStencilState(prevDepth, stencilRef);
    if (prevDepth) prevDepth->Release();
}

// -----------------------------------------------------------
// 解放処理
// -----------------------------------------------------------
void SpriteScreen::Release()
{
    m_vb.Reset();
    m_matrixBuf.Reset();
	m_colorBuf.Reset();
    m_layout.Reset();
    m_vs.Reset();
    m_ps.Reset();
    m_sampler.Reset();
    m_srv = nullptr;
}