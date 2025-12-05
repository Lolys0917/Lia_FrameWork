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
    void SetTexture(const char* texName); // override model-level texture (applies to all submeshes if used)
    void SetSubmeshTexture(int submeshIndex, const char* texName); // override specific submesh

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
    struct SubMesh {
        ComPtr<ID3D11Buffer> vertexBuffer;
        ComPtr<ID3D11Buffer> indexBuffer;
        UINT indexCount = 0;

        bool hasTexture = false;
        ComPtr<ID3D11ShaderResourceView> textureSRV; // per-submesh override or asset SRV

        bool hasMaterialColor = false;
        XMFLOAT4 materialDiffuse = { 1,1,1,1 };

        // user override color per-submesh (if you want in future)
        bool userColorSet = false;
        XMFLOAT4 userColor = { 1,1,1,1 };
    };

    std::string modelPath;
    std::string texturePath; // model-level override
    ModelType modelType = ModelType::OBJ;

    // per-submesh data
    std::vector<SubMesh> subMeshes;

    // DirectX11 common state
    ComPtr<ID3D11InputLayout> inputLayout;
    ComPtr<ID3D11Buffer> constantBuffer;
    ComPtr<ID3D11SamplerState> sampler;

    UINT totalIndexCount = 0;

    // fallback / user color
    XMFLOAT4 diffuseColor = { 1,1,1,1 };
    bool userColorSet = false;

    bool useTexture = false; // legacy overall flag (not used per-submesh logic)

    // transformation
    XMMATRIX MatPos = XMMatrixIdentity();
    XMMATRIX MatSize = XMMatrixIdentity();
    XMMATRIX MatAngle = XMMatrixIdentity();
    XMMATRIX world = XMMatrixIdentity();
    XMMATRIX ViewSet = XMMatrixIdentity();
    XMMATRIX ProjSet = XMMatrixIdentity();

    MotionPackage* motionPackage = nullptr;
};
