#pragma once

#include "Component.h"
#include "Manager.h"
#include "Main.h" // GetDevice(), GetContext(), GetTextureSRV(), AddMessage()
#include <DirectXMath.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class SpriteCylinder : public Component
{
public:
    using Component::Component;

    void Init() override;
    void Update() override {}
    void Draw() override;
    void Release() override;

    // setters (API compatible with SpriteWorld-like usage)
    void SetPos(float x, float y, float z);
    void SetSize(float x, float y, float z);
    void SetAngle(float rx, float ry, float rz); // rad
    void SetColor(float r, float g, float b, float a);

    void SetView(const XMMATRIX& view);
    void SetProj(const XMMATRIX& proj);

    void SetSideTexture(const char* path);
    void SetTopTexture(const char* path);
    void SetBottomTexture(const char* path);

    void SetSegment(int seg);

private:
    struct Vertex {
        XMFLOAT3 pos;
        XMFLOAT2 uv;
    };

    struct MatrixBuffer {
        XMMATRIX mvp;
        XMFLOAT4 diffuseColor;
        int useTexture;
        XMFLOAT3 pad;
    };
    struct ColorBuffer {
        XMFLOAT4 color;
    };

    // transform / visual
    XMFLOAT3 m_pos{ 0,0,0 };
    XMFLOAT3 m_angle{ 0,0,0 };
    XMFLOAT3 m_size{ 1,1,1 }; // radius, height
    XMFLOAT4 m_color{ 1,1,1,1 };

    // camera matrices (set each frame by SceneManager)
    XMMATRIX ViewSet{};
    XMMATRIX ProjSet{};

    // textures (raw pointers to SRV managed elsewhere)
    ID3D11ShaderResourceView* m_srvSide = nullptr;
    ID3D11ShaderResourceView* m_srvTop = nullptr;
    ID3D11ShaderResourceView* m_srvBottom = nullptr;

    // buffers & pipeline
    ComPtr<ID3D11Buffer> m_vbSide;
    ComPtr<ID3D11Buffer> m_vbTop;
    ComPtr<ID3D11Buffer> m_vbBottom;

    ComPtr<ID3D11InputLayout> m_layout;
    ComPtr<ID3D11VertexShader> m_vs;
    ComPtr<ID3D11PixelShader> m_ps;
    ComPtr<ID3D11Buffer> m_matrixBuf;
    ComPtr<ID3D11Buffer> m_colorBuf;
    ComPtr<ID3D11SamplerState> m_sampler;
    ComPtr<ID3D11BlendState> m_blend;
    ComPtr<ID3D11DepthStencilState> m_depth;

    // counts
    UINT m_sideVertexCount = 0;
    UINT m_topVertexCount = 0;
    UINT m_bottomVertexCount = 0;

    int m_seg = 32;

    void BuildMesh();
};
