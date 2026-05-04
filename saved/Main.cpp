#include "system/SystemAPI.h"

extern "C" __declspec(dllexport) void Init()
{
    MessageBoxText("Init", "Scene3");
    AddSpriteWorld("TestSprite3", "asset/est.png");
}

extern "C" __declspec(dllexport) void Update()
{
}

extern "C" __declspec(dllexport) void Release()
{
}