#pragma once

#include "Component.h"

#include <DirectXMath.h>

using namespace DirectX;

class Camera : public Component
{
protected:
    XMMATRIX view;
    XMMATRIX proj;
public:
    using Component::Component;

    void SetCameraView(DirectX::XMFLOAT4 CamPos, DirectX::XMFLOAT4 LookAt);
    void SetCameraProjection(float FovY, float ScreenW, float ScreenH);

    XMMATRIX GetView()
    {
        return view;
    }
    XMMATRIX GetProjection()
    {
        return proj;
    }

    void Init()override
    {
        view = XMMatrixIdentity();
        proj = XMMatrixIdentity();
    }
    void Release()override
    {
        view = XMMatrixIdentity();
        proj = XMMatrixIdentity();
    }
};