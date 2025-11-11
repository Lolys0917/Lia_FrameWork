#include "ComponentModel.h"
#include "Main.h"  // GetDevice(), GetContext(), g_samplerState など

using namespace DirectX;

ComponentModel::~ComponentModel()
{
    Release();
}

void ComponentModel::Init()
{
    // --- モデル頂点を取得 ---
    m_vertices = GetModelVertex(m_modelName.c_str());
    if (!m_vertices || m_vertices->empty()) {
        MessageBoxA(nullptr, ("Model not found: " + m_modelName).c_str(), "Error", MB_OK);
        return;
    }

    // --- テクスチャSRVを取得 ---
    m_textureSRV = GetTextureSRV(m_textureName.c_str());
    if (!m_textureSRV) {
        MessageBoxA(nullptr, ("Texture not found: " + m_textureName).c_str(), "Warning", MB_OK);
    }

    // --- 頂点バッファ作成 ---
    D3D11_BUFFER_DESC vbd{};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = static_cast<UINT>(sizeof(ModelVertex) * m_vertices->size());
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = m_vertices->data();

    HRESULT hr = GetDevice()->CreateBuffer(&vbd, &initData, &m_vertexBuffer);
    if (FAILED(hr)) {
        MessageBoxA(nullptr, "CreateBuffer(Vertex) failed", "Error", MB_OK);
        return;
    }

    // --- 定数バッファ作成（MatrixBufferはManager.hで定義済み構造体） ---
    D3D11_BUFFER_DESC cbd{};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(MatrixBuffer);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = GetDevice()->CreateBuffer(&cbd, nullptr, &m_matrixBuffer);
    if (FAILED(hr)) {
        MessageBoxA(nullptr, "CreateBuffer(Constant) failed", "Error", MB_OK);
        return;
    }
}

void ComponentModel::Update()
{
    // 今のところ不要（将来、アニメーションなどで使用可能）
}

void ComponentModel::Draw()
{
    if (!m_vertices || !m_vertexBuffer || !m_matrixBuffer)
        return;

    auto* context = GetContext();

    UINT stride = sizeof(ModelVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // --- 行列バッファ更新 ---
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(context->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        MatrixBuffer* mat = reinterpret_cast<MatrixBuffer*>(mapped.pData);
        mat->world = XMMatrixTranspose(m_world);
        mat->view = XMMatrixIdentity(); // View行列は不要／固定視点の場合は単位行列
        mat->proj = XMMatrixIdentity(); // Proj行列も不要なら単位行列
        context->Unmap(m_matrixBuffer, 0);
    }

    // --- バッファ設定 ---
    context->VSSetConstantBuffers(0, 1, &m_matrixBuffer);

    if (m_textureSRV)
        context->PSSetShaderResources(0, 1, &m_textureSRV);

    context->PSSetSamplers(0, 1, &g_samplerState);

    // --- 描画（IndexBuffer不要タイプ） ---
    context->Draw(static_cast<UINT>(m_vertices->size()), 0);
}

void ComponentModel::Release()
{
    if (m_vertexBuffer) { m_vertexBuffer->Release(); m_vertexBuffer = nullptr; }
    if (m_matrixBuffer) { m_matrixBuffer->Release(); m_matrixBuffer = nullptr; }
}

void ComponentModel::SetWorld(const XMMATRIX& world)
{
    m_world = world;
}
