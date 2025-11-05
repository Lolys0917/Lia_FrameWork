#include "Grid.h"
#include "Main.h"
#include <d3dcompiler.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

D3D11_BUFFER_DESC bd = {};

void Grid::Init()
{
    DeviceGetter = GetDevice();

    // 頂点データ (1本線)/デフォルト値
    Vertex line[] = {
        { XMFLOAT3(-10.0f, 0.0f, 0.0f) },
        { XMFLOAT3(10.0f, 0.0f, 0.0f) },
    };

    // 頂点バッファ
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(line);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = line;

    GetDevice()->CreateBuffer(&bd, &initData, &m_vertexBuffer);

    // 定数バッファ
    bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    GetDevice()->CreateBuffer(&bd, nullptr, &m_constantBuffer);

    // シェーダーコンパイル

    D3DCompileFromFile(L"Shader/grid_vs.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    D3DCompileFromFile(L"Shader/grid_ps.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, &errorBlob);

    GetDevice()->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
    GetDevice()->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);

    // 入力レイアウト
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    GetDevice()->CreateInputLayout(layout, 1, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);

    if (vsBlob) vsBlob->Release();
    if (psBlob) psBlob->Release();
    if (errorBlob) errorBlob->Release();
}

void Grid::SetView(const XMMATRIX& View)
{
    ViewSet = View;
}

void Grid::SetProj(const XMMATRIX& Proj)
{
    ProjSet = Proj;
}

void Grid::SetColor(const XMFLOAT4& color)
{
    ColorSet = color;
}

void Grid::Draw()
{
    ConstantBuffer cb;
    // 定数バッファ更新
    cb.viewProj = XMMatrixTranspose(ViewSet * ProjSet);
    cb.lineColor = ColorSet;

    GetContext()->UpdateSubresource(m_constantBuffer, 0, nullptr, &cb, 0, 0);

    // バインド
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    GetContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    GetContext()->IASetInputLayout(m_inputLayout);
    GetContext()->VSSetShader(m_vertexShader, nullptr, 0);
    GetContext()->VSSetConstantBuffers(0, 1, &m_constantBuffer);
    GetContext()->PSSetShader(m_pixelShader, nullptr, 0);
    GetContext()->PSSetConstantBuffers(0, 1, &m_constantBuffer);

    // 描画
    GetContext()->Draw(2, 0);
}

void Grid::SetPos(XMFLOAT3 Start, XMFLOAT3 End)
{
    // 頂点データ (1本線)
    Vertex line[] = {
        { Start },
        { End },
    };

    // 頂点バッファ
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(line);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = line;

    GetDevice()->CreateBuffer(&bd, &initData, &m_vertexBuffer);
}

void Grid::DrawBox(const XMFLOAT3& pos, const XMFLOAT3& size, const XMFLOAT3& Angle)
{
    // --- 1. 8頂点を作成 ---
    XMFLOAT3 v[8] = {
        {-0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f},
        {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f},
        { 0.5f,  0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f},
    };

    // --- 2. ワールド行列を作成 ---
    XMMATRIX S = XMMatrixScaling(size.x, size.y, size.z);
    XMMATRIX R = XMMatrixRotationRollPitchYaw(Angle.x, Angle.y, Angle.z);
    XMMATRIX T = XMMatrixTranslation(pos.x, pos.y, pos.z);
    XMMATRIX world = S * R * T;

    // --- 3. 頂点を変換 ---
    Vertex verts[8];
    for (int i = 0; i < 8; i++)
    {
        XMVECTOR p = XMLoadFloat3(&v[i]);
        p = XMVector3TransformCoord(p, world);
        XMStoreFloat3(&verts[i].position, p);
    }

    // --- 4. 12本の線分インデックス ---
    UINT indices[] = {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4, 1,5, 2,6, 3,7
    };

    // --- 5. 一時頂点/インデックスバッファ作成 ---
    D3D11_BUFFER_DESC vbd{};
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(verts);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit{ verts };
    ComPtr<ID3D11Buffer> vb;
    DeviceGetter->CreateBuffer(&vbd, &vinit, &vb);

    D3D11_BUFFER_DESC ibd{};
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(indices);
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit{ indices };
    ComPtr<ID3D11Buffer> ib;
    DeviceGetter->CreateBuffer(&ibd, &iinit, &ib);

    // --- 6. 定数バッファ更新 ---
    ConstantBuffer cb;
    cb.viewProj = XMMatrixTranspose(ViewSet * ProjSet);
    cb.lineColor = ColorSet;
    GetContext()->UpdateSubresource(m_constantBuffer, 0, nullptr, &cb, 0, 0);

    // --- 7. 描画セットアップ ---
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    GetContext()->IASetVertexBuffers(0, 1, vb.GetAddressOf(), &stride, &offset);
    GetContext()->IASetIndexBuffer(ib.Get(), DXGI_FORMAT_R32_UINT, 0);
    GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    GetContext()->IASetInputLayout(m_inputLayout);
    GetContext()->VSSetShader(m_vertexShader, nullptr, 0);
    GetContext()->VSSetConstantBuffers(0, 1, &m_constantBuffer);
    GetContext()->PSSetShader(m_pixelShader, nullptr, 0);
    GetContext()->PSSetConstantBuffers(0, 1, &m_constantBuffer);

    // --- 8. Draw ---
    GetContext()->DrawIndexed(24, 0, 0);
}