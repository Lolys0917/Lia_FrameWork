#pragma once

#include "Component.h"

#include <string>
#include <vector>

#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXMathMatrix.inl>
#include <wrl.h>

#pragma comment (lib, "d3dcompiler.lib")

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class Model : public Component
{
public:
    using Component::Component;

    void SettingModelVertex(const char* filename);

    void Init();
    void Draw() override;

    void Release() override;

    void SetPos(float PosX, float PosY, float PosZ);
    void SetSize(float SizeX, float SizeY, float SizeZ);
    void SetAngle(float AngleX, float AngleY, float AngleZ);

    void SetView(const XMMATRIX& view);
    void SetProj(const XMMATRIX& proj);

private:
    void IN_DrawObj();
    void IN_DrawFBX();

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
    //ÉèÅ[ÉãÉhçsóÒ
    XMMATRIX world = XMMatrixIdentity();

    XMMATRIX ViewSet;
    XMMATRIX ProjSet;

    UINT IndexCount = 0;
};