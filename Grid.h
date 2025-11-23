#pragma once

#include "Component.h"

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class Grid
{
public:
    Grid() {}
    ~Grid() {}

    void Init();
    void Draw();

    void SetView(const XMMATRIX& View);
    void SetProj(const XMMATRIX& Proj);
    void SetColor(const XMFLOAT4& color);
    void SetPos(XMFLOAT3 Start, XMFLOAT3 End); // push a single line

    void DrawBox(const XMFLOAT3& pos, const XMFLOAT3& size, const XMFLOAT3& Angle);
    void DrawGridPolygonGrid(
        int cols, int rows,
        float spacing, int sides,
        float radius,
        const XMFLOAT3& origin, const XMFLOAT3& Angle);
    void DrawGridPolygon(int sides, const XMFLOAT3& pos, const XMFLOAT3& size, const XMFLOAT3& Angle);

private:
    struct Vertex {
        XMFLOAT3 position;
        // NOTE: user requested size/rotation/color as input layout — to support that,
        // the vertex shader must declare matching input semantics and ShaderManager
        // must provide the corresponding VS blob. For now we keep only position,
        // matching the current DefaultGrid VS that takes float3 pos : POSITION.
    };

    struct ConstantBuffer {
        XMMATRIX viewProj;
        XMFLOAT4 lineColor;
    };

    // camera / color
    XMMATRIX ViewSet = XMMatrixIdentity();
    XMMATRIX ProjSet = XMMatrixIdentity();
    XMFLOAT4 ColorSet{ 1,1,1,1 };

    // GPU resources (ComPtr)
    ComPtr<ID3D11Buffer> m_vertexBuffer;      // dynamic vertex buffer (big enough for all lines)
    ComPtr<ID3D11Buffer> m_constantBuffer;
    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader> m_pixelShader;
    ComPtr<ID3D11InputLayout> m_inputLayout;

    // CPU-side accumulation list: each pair of vertices forms a line (line-list)
    std::vector<Vertex> m_pendingVertices;

    // device/context
    ID3D11Device* DeviceGetter = nullptr;
    ID3D11DeviceContext* Context = nullptr;

    // current GPU capacity (in bytes or vertex count)
    UINT m_gpuVertexCapacity = 0; // number of Vertex entries allocated on GPU

    // helpers
    void EnsureGPUBufferCapacity(UINT vertexCount);
    void FlushPendingToGPUAndDraw();

    // existing helpers used by original Grid implementation (kept)
    void DrawPolygonGrid(const XMFLOAT3& pos, float radius, int sides, const XMFLOAT3& Angle);
};
