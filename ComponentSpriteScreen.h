#pragma once
#include "Manager.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class SpriteScreen : public Component
{
public:
    using Component::Component;
    ~SpriteScreen() override {}

    void Init() override;
    void Draw() override;
    void Release() override;

    void SetTexture(const char* path);
    void SetPos(float x, float y);
    void SetSize(float w, float h);
    void SetColor(float r, float g, float b, float a);

private:
    struct VertexScreen {
        XMFLOAT3 pos;
        XMFLOAT2 uv;
    };

    struct MatrixBuffer {
        XMMATRIX mvp;
        XMFLOAT4 color;
    };


    XMFLOAT3 m_pos{ 0, 0, 0 };
    XMFLOAT3 m_size{ 100, 100, 1 };
    XMFLOAT4 m_color{ 1, 1, 1, 1 };
    bool m_visible = true;

    ID3D11ShaderResourceView* m_srv = nullptr;

    ComPtr<ID3D11Buffer> m_vb;
    ComPtr<ID3D11Buffer> m_matrixBuf;
    ComPtr<ID3D11InputLayout> m_layout;
    ComPtr<ID3D11VertexShader> m_vs;
    ComPtr<ID3D11PixelShader> m_ps;
    ComPtr<ID3D11SamplerState> m_sampler; // Å© SpriteScreenêÍóp
};
