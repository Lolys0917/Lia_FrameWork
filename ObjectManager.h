#pragma once
#include "UtilManager.h"
#include "SceneManager.h"
#include <DirectXMath.h>
using namespace DirectX;

class Object;
class Grid;

//===============================
// Objectä«óù
//===============================
void InitDo();
void UpdateDo();
void DrawDo();
void ReleaseDo();

//===============================
// Camera
//===============================
void AddCamera(const char* name);
void SetCameraPos(const char* name, float x, float y, float z);
void SetCameraLook(const char* name, float x, float y, float z);
void UseCameraSet(const char* name);
void CreateCamera();
void SettingCameraOnce();

//===============================
// Grid
//===============================
void AddGridBox(const char* name);
void SetGridBoxPos(const char* name, float x, float y, float z);
void SetGridBoxSize(const char* name, float x, float y, float z);
void SetGridBoxColor(const char* name, float R, float G, float B, float A);

void AddGridPolygon(const char* name);
void SetGridPolygonPos(const char* name, float x, float y, float z);
void SetGridPolygonColor(const char* name, float R, float G, float B, float A);
void SetGridPolygonSides(const char* name, int sides);

void DrawGridBase();