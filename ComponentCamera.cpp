#include "ComponentCamera.h"
#include "Main.h"

void Camera::SetCameraView(DirectX::XMFLOAT4 CamPos, DirectX::XMFLOAT4 LookAt)
{
    view = XMMatrixLookAtLH(
        XMVectorSet(CamPos.x, -CamPos.y, CamPos.z, 1.0f), // カメラ位置
        XMVectorSet(LookAt.x, -LookAt.y, LookAt.z, 1.0f),   // 注視点
        XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f));  // 上方向
}

void Camera::SetCameraProjection(float FovY, float ScreenW, float ScreenH)
{
    if (ScreenH <= 0.0f)
    {
        MessageBoxA(nullptr, "ScreenH <= 0.0f", "Camera Error", MB_OK);
        ScreenH = 1.0f;
    }

    if (ScreenW <= 0.0f)
    {
        MessageBoxA(nullptr, "ScreenW <= 0.0f", "Camera Error", MB_OK);
        ScreenW = 1.0f;
    }

    if (FovY <= 0.0f || FovY >= 179.0f)
    {
        MessageBoxA(nullptr, "FovY が不正値です。", "Camera Error", MB_OK);
        FovY = 70.0f;
    }

    float aspect = ScreenW / ScreenH;

    proj
        = 
        XMMatrixPerspectiveFovLH(
        XMConvertToRadians(FovY),
        aspect,
        0.1f,
        100.0f);
}