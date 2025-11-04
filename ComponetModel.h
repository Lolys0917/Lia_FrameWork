#pragma once

#include "Component.h"

#include <string>
#include <vector>

#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>

#include <wrl.h>

#pragma comment (lib, "d3dcompiler.lib")

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class Model : public Component
{
public:
    using Component::Component;

    bool Load(
        const std::wstring& filename,
        const std::wstring& texturefile);
    bool LoadFBX(
        const std::wstring& ModelPath);

    void Draw() override;

    void Release() override;

    void SetPos(float PosX, float PosY, float PosZ);
    void SetSize(float SizeX, float SizeY, float SizeZ);
    void SetAngle(float AngleX, float AngleY, float AngleZ);

    void SetView(const XMMATRIX& view);
    void SetProj(const XMMATRIX& proj);

    void SetModelType(int ModelType);

private:
    struct MatrixBuffer
    {
        XMMATRIX mvp;
        XMFLOAT4 diffuseColor;
        int useTexture;
        XMFLOAT3 pad;
    };
    struct ModelVertex
    {
        XMFLOAT3 position;
        XMFLOAT2 uv;
        XMFLOAT3 normal;
    };

    bool LoadModel(const std::wstring& filename);
    bool LoadTexture(const std::wstring& filepath);

    void DrawObj();
    void DrawFBX();

    int Type;

    float PosX;
    float PosY;
    float PosZ;
    float SizeX;
    float SizeY;
    float SizeZ;
    float AngleX;
    float AngleY;
    float AngleZ;

    XMMATRIX MatPos;
    XMMATRIX MatSize;
    XMMATRIX MatAngle;
    //ワールド行列
    XMMATRIX world = XMMatrixIdentity();

    XMMATRIX ViewSet;
    XMMATRIX ProjSet;

    UINT IndexCount = 0;

    ComPtr<ID3D11Buffer> m_VertexBufferPtr;
    ComPtr<ID3D11Buffer> m_IndexPtr;

    std::vector<ModelVertex> m_vertices;
    MatrixBuffer m_matrixData;
    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11Buffer* m_matrixBuffer = nullptr;

    ID3D11VertexShader* m_vs = nullptr;
    ID3D11PixelShader* m_ps = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;

    //テクスチャ用
    ID3D11ShaderResourceView* m_textureSRV = nullptr;
    ID3D11SamplerState* m_samplerState = nullptr;
};