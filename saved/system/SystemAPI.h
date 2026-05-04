#pragma once

//API_SETTING ___________________
#ifdef SYSTEM_EXPORTS
#define SYSTEM_API __declspec(dllexport)
#else
#define SYSTEM_API __declspec(dllimport)
#endif

  //////////////////
 // DebugManager //
//////////////////
void InitDebugManager();
void DrawMessageBox();
extern "C" SYSTEM_API void MessageBoxText(const char* text, const char* caption);