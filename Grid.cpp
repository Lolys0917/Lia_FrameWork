// Grid.cpp
#include "Grid.h"
#include "Manager.h"
#include "Main.h"

#include <d3dcompiler.h>
#include <wrl.h>
#include <vector>
#include <cmath>

#define M_PI 3.14159265358979323846

using Microsoft::WRL::ComPtr;

void Grid::Init()
{
    DeviceGetter = GetDevice();
    Context = GetContext();
    if (!DeviceGetter || !Context) {
        AddMessage("Grid::Init - Device/Context null");
        return;
    }

    // create a modest dynamic vertex buffer initially (will grow if needed)
    m_gpuVertexCapacity = 1024; // initial number of Vertex elements
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * m_gpuVertexCapacity);
    bd.StructureByteStride = sizeof(Vertex);

    HRESULT hr = DeviceGetter->CreateBuffer(&bd, nullptr, &m_vertexBuffer);
    if (FAILED(hr)) {
        AddMessage("Grid::Init - CreateBuffer vertex failed");
    }

    // constant buffer
    bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.ByteWidth = sizeof(ConstantBuffer);
    hr = DeviceGetter->CreateBuffer(&bd, nullptr, &m_constantBuffer);
    if (FAILED(hr)) {
        AddMessage("Grid::Init - CreateBuffer constant failed");
    }

    // get shaders from ShaderManager (the engine's default grid shaders)
    m_vertexShader = GetVertexShader3DGrid();
    m_pixelShader = GetPixelShader3DGrid();

    if (!m_vertexShader || !m_pixelShader) {
        MessageBoxA(nullptr, "Grid: Default shaders not ready", "ERROR", MB_OK);
        return;
    }

    // obtain VS blob from ShaderManager to create input layout (must match the VS signature!)
    ID3DBlob* vsBlob = GetCurrent3DGridVSBlob();
    if (!vsBlob) {
        MessageBoxA(nullptr, "Grid: VS Blob is NULL", "ERROR", MB_OK);
        return;
    }

    // Input layout must match the vertex shader input. Default grid VS expects POSITION only.
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
            D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = DeviceGetter->CreateInputLayout(
        layout, ARRAYSIZE(layout),
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        &m_inputLayout
    );
    if (FAILED(hr)) {
        AddMessage("Grid::Init - CreateInputLayout failed");
        return;
    }

    // start with empty pending list
    m_pendingVertices.clear();
}

// Ensure dynamic GPU buffer capacity is enough for vertexCount vertices.
// If not, release and recreate a larger dynamic buffer (grow factor 2x).
void Grid::EnsureGPUBufferCapacity(UINT vertexCount)
{
    if (vertexCount == 0) return;
    if (vertexCount <= m_gpuVertexCapacity) return;

    // grow capacity (exponential)
    UINT newCap = m_gpuVertexCapacity ? m_gpuVertexCapacity : 1;
    while (newCap < vertexCount) newCap *= 2;

    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * newCap);
    bd.StructureByteStride = sizeof(Vertex);

    ComPtr<ID3D11Buffer> newVB;
    HRESULT hr = DeviceGetter->CreateBuffer(&bd, nullptr, &newVB);
    if (FAILED(hr)) {
        AddMessage("Grid::EnsureGPUBufferCapacity - CreateBuffer failed");
        return;
    }

    // replace
    m_vertexBuffer = newVB;
    m_gpuVertexCapacity = newCap;
}

// Transfer pending CPU vertices to GPU (via Map/Unmap) and draw them.
// This draws all pending vertices as a line-list (each pair is one line).
void Grid::FlushPendingToGPUAndDraw()
{
    if (m_pendingVertices.empty()) return;
    if (!Context || !m_vertexBuffer) return;

    UINT vertexCount = (UINT)m_pendingVertices.size();
    EnsureGPUBufferCapacity(vertexCount);

    // map and copy
    D3D11_MAPPED_SUBRESOURCE mapped{};
    HRESULT hr = Context->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) {
        AddMessage("Grid::Flush - Map failed");
        return;
    }

    // copy data
    memcpy(mapped.pData, m_pendingVertices.data(), vertexCount * sizeof(Vertex));
    Context->Unmap(m_vertexBuffer.Get(), 0);

    // update constant buffer (viewProj + color)
    ConstantBuffer cb;
    cb.viewProj = XMMatrixTranspose(ViewSet * ProjSet);
    cb.lineColor = ColorSet;
    Context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    // bind and draw
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    ID3D11Buffer* vb = m_vertexBuffer.Get();
    Context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    Context->IASetInputLayout(m_inputLayout.Get());
    Context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    Context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    Context->PSSetShader(m_pixelShader.Get(), nullptr, 0); // NOTE: keep compatibility if using raw pointer; else see below
    // Wait - we use ComPtr for pixel shader: call Get()
    // But above line uses m_pixel_shader; replace properly:

    // Correct binding:
    Context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    Context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    // Draw: each 2 vertices = one line. So vertexCount must be even.
    Context->Draw(vertexCount, 0);

    // clear pending
    m_pendingVertices.clear();
}

void Grid::Draw()
{
    switch (m_gridType)
    {
    case Grid_Line:
        SetLine(
            { GetPosition().x, GetPosition().y, GetPosition().z },
            { GetSize().x, GetSize().y, GetSize().z });
        break;
    case Grid_Box:
        SetBox(
            { GetPosition().x, GetPosition().y, GetPosition().z },
            { GetSize().x, GetSize().y, GetSize().z },
            { GetAngle().x, GetAngle().y, GetAngle().z });
        break;
    case Grid_Polygon:
        SetPolygon(
            m_sides,
            { GetPosition().x, GetPosition().y, GetPosition().z },
            { GetSize().x, GetSize().y, GetSize().z },
            { GetAngle().x, GetAngle().y, GetAngle().z });
        break;
    case Grid_GridPolygon:
        break;
    default:
        break;
    }

    // Flush pending geometries into GPU and draw them
    FlushPendingToGPUAndDraw();
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

// SetPos: instead of creating a new VB each call, we push two vertices into pending list.
// Caller expects SetPos to set the one-line geometry; keep behavior: push that line.
void Grid::SetLine(XMFLOAT3 Start, XMFLOAT3 End)
{
    Vertex a{ Start };
    Vertex b{ End };
    m_pendingVertices.push_back(a);
    m_pendingVertices.push_back(b);
}

// DrawBox: push 12 line segments (24 vertices) into pending list
void Grid::SetBox(const XMFLOAT3& pos, const XMFLOAT3& size, const XMFLOAT3& Angle)
{
    //MessageBoxA(NULL, "Grid", "DRAW", S_OK);
    // create 8 corners in local space
    XMFLOAT3 vlocal[8] = {
        {-0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f},
        {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f},
        { 0.5f,  0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f},
    };

    XMMATRIX S = XMMatrixScaling(size.x, size.y, size.z);
    XMMATRIX R = XMMatrixRotationRollPitchYaw(Angle.x, Angle.y, Angle.z);
    XMMATRIX T = XMMatrixTranslation(pos.x, pos.y, pos.z);
    XMMATRIX world = S * T * R;

    XMFLOAT3 worldPos[8];
    for (int i = 0; i < 8; ++i) {
        XMVECTOR p = XMLoadFloat3(&vlocal[i]);
        p = XMVector3TransformCoord(p, world);
        XMStoreFloat3(&worldPos[i], p);
    }

    // 12 edges (pairs)
    const int edgePairs[24] = {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4, 1,5, 2,6, 3,7
    };

    for (int i = 0; i < 24; i += 2) {
        Vertex a{ worldPos[edgePairs[i]] };
        Vertex b{ worldPos[edgePairs[i + 1]] };
        m_pendingVertices.push_back(a);
        m_pendingVertices.push_back(b);
    }
}

// DrawPolygonGrid: draw many polygons by calling DrawPolygonGrid per cell
void Grid::SetGridPolygonGrid(
    int cols, int rows,
    float spacing, int sides,
    float radius,
    const XMFLOAT3& origin, const XMFLOAT3& Angle)
{
    float startX = origin.x - (cols - 1) * 0.5f * spacing;
    float startY = origin.y - (rows - 1) * 0.5f * spacing;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            XMFLOAT3 pos;
            pos.x = startX + c * spacing;
            pos.y = startY + r * spacing;
            pos.z = origin.z;
            DrawPolygonGrid(pos, radius, sides, Angle);
        }
    }
}

// DrawGridPolygon: push line segments for polygon (edges)
void Grid::SetPolygon(int sides, const XMFLOAT3& pos, const XMFLOAT3& size, const XMFLOAT3& Angle)
{
    if (sides < 3) sides = 3;

    float halfW = size.x * 0.5f;
    float halfD = size.y * 0.5f;
    float halfH = size.z * 0.5f;

    // construct top and bottom perimeter
    std::vector<XMFLOAT3> localVerts(sides * 2);
    for (int i = 0; i < sides; ++i) {
        float theta = (2.0f * static_cast<float>(M_PI) * i) / sides;
        float x = cosf(theta) * halfW;
        float y = sinf(theta) * halfD;
        localVerts[i] = XMFLOAT3{ x, y, +halfH };
        localVerts[i + sides] = XMFLOAT3{ x, y, -halfH };
    }

    XMMATRIX R = XMMatrixRotationRollPitchYaw(Angle.x, Angle.y, Angle.z);
    XMMATRIX T = XMMatrixTranslation(pos.x, pos.y, pos.z);
    XMMATRIX world = R * T;

    // transform top and bottom to world, then push edges (top perimeter edges + bottom perimeter edges)
    std::vector<XMFLOAT3> worldVerts(localVerts.size());
    for (size_t i = 0; i < localVerts.size(); ++i) {
        XMVECTOR p = XMLoadFloat3(&localVerts[i]);
        p = XMVector3TransformCoord(p, world);
        XMStoreFloat3(&worldVerts[i], p);
    }

    // top ring edges
    for (int i = 0; i < sides; ++i) {
        int ni = (i + 1) % sides;
        Vertex a{ worldVerts[i] };
        Vertex b{ worldVerts[ni] };
        m_pendingVertices.push_back(a);
        m_pendingVertices.push_back(b);
    }
    // bottom ring edges
    for (int i = 0; i < sides; ++i) {
        int ni = (i + 1) % sides;
        Vertex a{ worldVerts[i + sides] };
        Vertex b{ worldVerts[ni + sides] };
        m_pendingVertices.push_back(a);
        m_pendingVertices.push_back(b);
    }
    // side edges
    for (int i = 0; i < sides; ++i) {
        Vertex a{ worldVerts[i] };
        Vertex b{ worldVerts[i + sides] };
        m_pendingVertices.push_back(a);
        m_pendingVertices.push_back(b);
    }
}
void Grid::DrawPolygonGrid(const XMFLOAT3& pos, float radius, int sides, const XMFLOAT3& Angle)
{
    if (sides < 3) sides = 3;

    // --- 正多角形の頂点生成（ローカル座標） ---
    std::vector<XMFLOAT3> localVerts(sides);
    for (int i = 0; i < sides; ++i) {
        float theta = (2.0f * static_cast<float>(M_PI) * i) / sides;
        float x = cosf(theta) * radius;
        float y = sinf(theta) * radius;
        localVerts[i] = XMFLOAT3{ x, y, 0.0f };
    }

    // --- ワールド行列 ---
    XMMATRIX R = XMMatrixRotationRollPitchYaw(Angle.x, Angle.y, Angle.z);
    XMMATRIX T = XMMatrixTranslation(pos.x, pos.y, pos.z);
    XMMATRIX world = R * T;

    // --- 変換して pending に追加 ---
    for (int i = 0; i < sides; ++i) {
        int ni = (i + 1) % sides;

        XMFLOAT3 wp0, wp1;

        {
            XMVECTOR p = XMLoadFloat3(&localVerts[i]);
            p = XMVector3TransformCoord(p, world);
            XMStoreFloat3(&wp0, p);
        }
        {
            XMVECTOR p = XMLoadFloat3(&localVerts[ni]);
            p = XMVector3TransformCoord(p, world);
            XMStoreFloat3(&wp1, p);
        }

        m_pendingVertices.push_back({ wp0 });
        m_pendingVertices.push_back({ wp1 });
    }
}