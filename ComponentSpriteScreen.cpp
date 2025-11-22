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
    // --- シェーダー読み込み ---
    //ComPtr<ID3DBlob> vsBlob, psBlob;
    //D3DCompileFromFile(L"Shader/2D_VS.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, nullptr);
    //D3DCompileFromFile(L"Shader/2D_PS.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, nullptr);
    //
    //GetDevice()->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);
    //GetDevice()->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps);

    extern ID3DBlob* g_Default2DVSBlob;
    // --- 入力レイアウト ---
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0, D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12, D3D11_INPUT_PER_VERTEX_DATA,0},
    };
    GetDevice()->CreateInputLayout(layout, 2, 
        g_Default2DVSBlob->GetBufferPointer(), 
        g_Default2DVSBlob->GetBufferSize(), &m_layout);

    // --- 定数バッファ ---
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.ByteWidth = sizeof(MatrixBuffer);
    GetDevice()->CreateBuffer(&bd, nullptr, &m_matrixBuf);
   
    // --- サンプラーステート作成（UI専用） ---
    D3D11_SAMPLER_DESC sampDesc{};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // UI向けに滑らか補間
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.MaxAnisotropy = 1;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    GetDevice()->CreateSamplerState(&sampDesc, &m_sampler);
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
void SpriteScreen::SetPos(float x, float y)
{
    m_pos = { x, y, 0 };
}
void SpriteScreen::SetSize(float w, float h)
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
    vbd.Usage = D3D11_USAGE_DYNAMIC;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
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
    mb.color = m_color;
    GetContext()->UpdateSubresource(m_matrixBuf.Get(), 0, nullptr, &mb, 0, 0);

    // --- 深度ステンシル無効化 ---
    ID3D11DepthStencilState* prevDepth = nullptr;
    UINT stencilRef = 0;
    GetContext()->OMGetDepthStencilState(&prevDepth, &stencilRef);
    GetContext()->OMSetDepthStencilState(nullptr, 0);

    // --- バインド設定 ---
    UINT stride = sizeof(VertexScreen), offset = 0;
    GetContext()->IASetVertexBuffers(0, 1, m_vb.GetAddressOf(), &stride, &offset);
    GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    GetContext()->IASetInputLayout(m_layout.Get());

    GetContext()->VSSetShader(GetVertexShader2D(), nullptr, 0);
    GetContext()->VSSetConstantBuffers(0, 1, m_matrixBuf.GetAddressOf());

    GetContext()->PSSetShader(GetPixelShader2D(), nullptr, 0);
    GetContext()->PSSetShaderResources(0, 1, &m_srv);
    GetContext()->PSSetSamplers(0, 1, m_sampler.GetAddressOf()); // ← UI専用サンプラー設定

    // --- 描画 ---
    GetContext()->Draw(6, 0);

    // --- 深度を復帰 ---
    GetContext()->OMSetDepthStencilState(prevDepth, stencilRef);
    if (prevDepth) prevDepth->Release();

    /*ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    GetContext()->PSSetShaderResources(0, 1, nullSRV);*/
}

// -----------------------------------------------------------
// 解放処理
// -----------------------------------------------------------
void SpriteScreen::Release()
{
    m_vb.Reset();
    m_matrixBuf.Reset();
    m_layout.Reset();
    m_vs.Reset();
    m_ps.Reset();
    m_sampler.Reset();
    m_srv = nullptr;
}
