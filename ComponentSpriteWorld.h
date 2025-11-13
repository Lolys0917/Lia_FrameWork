#pragma once

#pragma once
#include "Manager.h"
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <vector>
using Microsoft::WRL::ComPtr;

using namespace DirectX;

class SpriteWorld : public Component
{
public:
    using Component::Component;
    ~SpriteWorld() override {};

    void Init() override;
    void Draw() override;
    void Release() override;

    void SetTexture(const char* assetPath);  // AssetManagerÇ©ÇÁéÊìæ
    void SetPos(float x, float y, float z);
    void SetSize(float w, float h);
    void SetAngle(float rx, float ry, float rz);
    void SetView(const XMMATRIX& view);
    void SetProj(const XMMATRIX& proj);
    void SetColor(const XMFLOAT4& color);
    void SetBillboard(bool enable);
private:
    struct Vertex {
        XMFLOAT3 pos;
        XMFLOAT2 uv;
    };
    struct MatrixBuffer {
        XMMATRIX mvp;           // 64 bytes
        XMFLOAT4 diffuseColor;  // 16 bytes
        int useTexture;         // 4 bytes
        XMFLOAT3 pad;           // 12 bytes -> çáåv 96 (=16*6)
    };
    struct ColorBuffer {
        XMFLOAT4 color;
    };

    XMMATRIX ViewSet;
    XMMATRIX ProjSet;

    bool m_isBillboard = false;
    XMFLOAT3 m_pos{ 0,0,0 };
    XMFLOAT3 m_angle{ 0,0,0 };
    XMFLOAT2 m_size{ 1,1 };
    XMFLOAT4 m_color{ 1,1,1,1 };

    ID3D11ShaderResourceView* m_srv = nullptr;

    ComPtr<ID3D11Buffer> m_vb;
    ComPtr<ID3D11Buffer> m_matrixBuf;
    ComPtr<ID3D11Buffer> m_colorBuf;
    ComPtr<ID3D11InputLayout> m_layout;
    ComPtr<ID3D11VertexShader> m_vs;
    ComPtr<ID3D11PixelShader> m_ps;

    ID3D11SamplerState* m_samplerState = nullptr;
    ID3D11BlendState* m_blendState = nullptr;

    ID3D11DepthStencilState* m_depthState = nullptr;
    ID3D11DepthStencilState* m_noDepthState = nullptr;
};