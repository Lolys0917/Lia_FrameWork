#pragma once

#include "Manager.h"
#include "Component.h"

#include <string>
#include <vector>
#include <unordered_map>

#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <wrl.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#pragma comment (lib, "d3dcompiler.lib")

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct ModelVertex;
struct MatrixBuffer;

enum class ModelType
{
    FBX,
    OBJ,
};

class Model : public Component
{
public:
    using Component::Component;

    ~Model()override;

    void SetModelPath(const char* filename);
    void SetTexture(ID3D11ShaderResourceView* srv);

    void Init() override;
    void Update()override;
    void Draw() override;
    void Release() override;

    void SetPos(float PosX, float PosY, float PosZ);
    void SetSize(float SizeX, float SizeY, float SizeZ);
    void SetAngle(float AngleX, float AngleY, float AngleZ);
    void SetColor(float R, float G, float B, float A);

    void SetView(const XMMATRIX& view);
    void SetProj(const XMMATRIX& proj);

    void SetMotion(const char* filename);//モーションの変化
    void SetMotionBlend(const char* filename, int changeFrame);//モーション変化

private:
    struct ColorBuffer {
        XMFLOAT4 color;
    };
    
    std::string modelPath;
    ModelType modelType = ModelType::OBJ;

    // assimp読み込み結果
    std::vector<ModelVertex> vertices;
    std::vector<unsigned int> indices;
    // DirectX11 buffer
    ComPtr<ID3D11InputLayout> inputLayout;
    ComPtr<ID3D11Buffer> m_colorBuf;
    ComPtr<ID3D11Buffer> m_matrixBuf;
    ComPtr<ID3D11Buffer> constantBuffer;
    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
    ComPtr<ID3D11VertexShader> m_vs;
    ComPtr<ID3D11PixelShader> m_ps;
    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11ShaderResourceView> m_textureSRV;
    UINT indexCount = 0;
    XMFLOAT4 color;

    XMFLOAT4 diffuseColor = { 1,1,1,1 };
    bool useTexture = false;
    ID3D11ShaderResourceView* textureSRV = nullptr;

    //行列
    XMMATRIX MatPos = XMMatrixIdentity();
    XMMATRIX MatSize = XMMatrixIdentity();
    XMMATRIX MatAngle = XMMatrixIdentity();
    XMMATRIX world = XMMatrixIdentity();
    XMMATRIX ViewSet = XMMatrixIdentity();
    XMMATRIX ProjSet = XMMatrixIdentity();
};