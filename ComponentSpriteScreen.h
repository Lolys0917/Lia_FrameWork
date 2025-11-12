#pragma once

#include "Component.h"
#include "Manager.h"

#include <d3d11.h>
#include <DirectXMath.h>

#include <string>

using namespace DirectX;

class SpriteScreen : public Component
{
public:
    using Component::Component;
    ~SpriteScreen()override;

    void Draw()override;

    bool SettingPath(const char* texturePath);

	void SetSRV(ID3D11ShaderResourceView* srv);

    //Top,Bottom,Left,Right
    void SetTBLR(float top, float bottom, float left, float right);

    //ÉJÉÅÉâÇ∆å©ÇΩñ⁄ÇÃê›íË
    void SetView(const XMMATRIX& view);
    void SetProj(const XMMATRIX& proj);
    void SetColor(const XMFLOAT4& color);

private:
    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT2 uv;
    };

    _declspec(align(16))
        struct MatrixBuffer
    {
        XMMATRIX mvp;
    };
    _declspec(align(16))
        struct ColorBuffer
    {
        XMFLOAT4 color;
    };

    float Top;
    float Bottom;
    float Left;
    float Right;

    XMMATRIX ViewSet;
    XMMATRIX ProjSet;
    XMFLOAT4 ColorSet;

    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11Buffer* m_matrixBuffer = nullptr;
    ID3D11Buffer* m_colorBuffer = nullptr;

    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;
    ID3D11ShaderResourceView* m_textureSRV = nullptr;
    ID3D11SamplerState* m_samplerState = nullptr;
    ID3D11BlendState* m_blendState = nullptr;

    ID3D11DepthStencilState* m_depthState = nullptr;
    ID3D11DepthStencilState* m_noDepthState = nullptr;

    bool LoadTextureFromFile(const std::wstring& filename);
};