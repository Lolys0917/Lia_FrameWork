#pragma once
#include <Windows.h>
#include <string>
#include <vector>

//===============================
// ç\ë¢ëÃíËã`
//===============================
typedef struct { float X, Y, Z, W; } Vec4;

typedef struct { Vec4* data; size_t size; size_t capacity; } Vec4Vector;
typedef struct { char** data; size_t size; size_t capacity; } CharVector;
typedef struct { int* data; size_t size; size_t capacity; } IntVector;
typedef struct { bool* data; size_t size; size_t capacity; } BoolVector;
typedef struct { char** keys; size_t size; size_t capacity; } KeyMap;

//===============================
// ÉÜÅ[ÉeÉBÉäÉeÉB
//===============================
void AddMessage(const char* sent);
const char* ConcatCStr(const char* str1, const char* str2);
void ConcatCStrFree(const char* str);

//===============================
// Vec4 ån
//===============================
void Vec4_Init(Vec4Vector* vec);
void Vec4_PushBack(Vec4Vector* vec, Vec4 value);
void Vec4_Set(Vec4Vector* vec, size_t index, Vec4 value);
Vec4 Vec4_Get(Vec4Vector* vec, size_t index);
void Vec4_Free(Vec4Vector* vec);

//===============================
// Char ån
//===============================
void VecC_Init(CharVector* vec);
void VecC_PushBack(CharVector* vec, const char* str);
void VecC_Set(CharVector* vec, size_t index, const char* str);
const char* VecC_Get(CharVector* vec, size_t index);
void VecC_Free(CharVector* vec);

//===============================
// Int ån
//===============================
void VecInt_Init(IntVector* vec);
void VecInt_PushBack(IntVector* vec, int value);
void VecInt_Set(IntVector* vec, size_t index, int value);
int VecInt_Get(IntVector* vec, size_t index);
void VecInt_Free(IntVector* vec);

//===============================
// Bool ån
//===============================
void VecBool_Init(BoolVector* vec);
void VecBool_PushBack(BoolVector* vec, bool value);
void VecBool_Set(BoolVector* vec, size_t index, bool value);
bool VecBool_Get(BoolVector* vec, size_t index);
void VecBool_Free(BoolVector* vec);

//===============================
// KeyMap ån
//===============================
void KeyMap_Init(KeyMap* map);
int KeyMap_Add(KeyMap* map, const char* key);
int KeyMap_GetIndex(KeyMap* map, const char* key);
const char* KeyMap_GetKey(KeyMap* map, int index);
void KeyMap_Free(KeyMap* map);