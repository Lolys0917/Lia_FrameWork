#pragma once

#include "Component.h"

#include <string>

#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

class SpriteWorld : public Component
{
public:
    using Component::Component;
    ~SpriteWorld() override;

    void Draw()override;

    bool Setting(const std::wstring& texturePath);

    void SetView(const XMMATRIX& view);
    void SetProj(const XMMATRIX& proj);
    void SetColor(const XMFLOAT4& color);

    void SetSize(float SizeW, float SizeH);
    void SetPos(float PosX, float PosY, float PosZ);
    void SetAngle(float AngleX, float AngleY, float AngleZ);

    void SetIfBillboard(bool Judge);
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

    float PosX;
    float PosY;
    float PosZ;

    float SizeW;
    float SizeH;

    float AngleX;
    float AngleY;
    float AngleZ;

    bool Billboard;

    XMMATRIX ViewSet;
    XMMATRIX ProjSet;
    XMFLOAT4 ColorSet;

    XMMATRIX MatPos;
    XMMATRIX MatSize;
    XMMATRIX MatAngle;

    XMMATRIX world;

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