
#include "ComponentCamera.h"

#include "Main.h"

void Camera::SetCameraView(DirectX::XMFLOAT4 CamPos, DirectX::XMFLOAT4 LookAt)
{
    view = XMMatrixLookAtLH(
        XMVectorSet(CamPos.x, -CamPos.y, CamPos.z, 1.0f), // ÉJÉÅÉâà íu
        XMVectorSet(LookAt.x, -LookAt.y, LookAt.z, 1.0f),   // íçéãì_
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));  // è„ï˚å¸
}

void Camera::SetCameraProjection(float FovY, float ScreenW, float ScreenH)
{
    proj = XMMatrixPerspectiveFovLH(
        /*XM_PIDIV4*/
        XMConvertToRadians(FovY), GetScreenWidth() / GetScreenHeight(), 0.1f, 100.0f);
}