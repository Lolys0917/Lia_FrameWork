#pragma once

#include "Component.h"

#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;

class Grid
{
public:
    //using Component::Component;

    Grid() {}
    ~Grid() {}

    void Init();
    void Draw();

    void SetView(const XMMATRIX& View);
    void SetProj(const XMMATRIX& Proj);
    void SetColor(const XMFLOAT4& color);
    void SetPos(XMFLOAT3 Start, XMFLOAT3 End);

    void DrawBox(const XMFLOAT3& pos, const XMFLOAT3& size, const XMFLOAT3& Angle);
private:
    struct Vertex {
        XMFLOAT3 position;
    };

    struct ConstantBuffer {
        XMMATRIX viewProj;
        XMFLOAT4 lineColor = { 0.0f,0.0f,0.0f,0.0f };
    };

    XMMATRIX ViewSet;
    XMMATRIX ProjSet;
    XMFLOAT4 ColorSet;

    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11Buffer* m_constantBuffer = nullptr;
    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    ID3D11Device* DeviceGetter;
};