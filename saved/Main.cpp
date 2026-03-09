#include "system/SystemAPI.h"

extern "C" __declspec(dllexport) void Init()
{
    MessageBoxText("Init", "Scene3");
}

extern "C" __declspec(dllexport) void Update()
{
}

extern "C" __declspec(dllexport) void Release()
{
}