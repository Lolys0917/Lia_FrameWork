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

enum class ModelType
{
    FBX,
    OBJ,
};

class Model : public Component
{
public:
    using Component::Component;

    ~Model();

    void SetModelPath(const char* filename);

    void Init() override;
    void Update()override;
    void Draw() override;
    void Release() override;

    void SetPos(float PosX, float PosY, float PosZ);
    void SetSize(float SizeX, float SizeY, float SizeZ);
    void SetAngle(float AngleX, float AngleY, float AngleZ);

    void SetView(const XMMATRIX& view);
    void SetProj(const XMMATRIX& proj);

    void AddMotion(const char* filename);//モーションの追加

    void SetMotion(const char* filename);//モーションの変化
    void SetMotionBlend(const char* filename, int changeFrame);//モーション変化

private:
    std::string modelPath;
    ModelType modelType = ModelType::OBJ;

    // assimp読み込み結果
    std::vector<ModelVertex> vertices;
    std::vector<unsigned int> indices;

    // DirectX11 buffer
    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
    UINT indexCount = 0;

    // transform matrices
    XMMATRIX MatPos = XMMatrixIdentity();
    XMMATRIX MatSize = XMMatrixIdentity();
    XMMATRIX MatAngle = XMMatrixIdentity();
    XMMATRIX world = XMMatrixIdentity();
    XMMATRIX ViewSet = XMMatrixIdentity();
    XMMATRIX ProjSet = XMMatrixIdentity();
};