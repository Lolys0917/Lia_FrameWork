#include "Manager.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <cstdio>

#define MAX_MESSAGE 256
static const char* MessageList[MAX_MESSAGE] = { nullptr };

//===============================
// メッセージ
//===============================
void AddMessage(const char* sent) {
    for (int i = 0; i < MAX_MESSAGE; i++) {
        if (MessageList[i] == nullptr) {
            MessageList[i] = _strdup(sent);
            break;
        }
    }
}

const char* ConcatCStr(const char* str1, const char* str2) {
    if (!str1 && !str2) return nullptr;
    if (!str1) return str2;
    if (!str2) return str1;
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    char* result = (char*)malloc(len1 + len2 + 1);
    strcpy_s(result, len1 + 1, str1);
    strcat_s(result, len1 + len2 + 1, str2);
    return result;
}

void ConcatCStrFree(const char* str) {
    free((void*)str);
}

//===============================
// Vec4 系
//===============================
void Vec4_Init(Vec4Vector* vec) {
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}
//push_back
void Vec4_PushBack(Vec4Vector* vec, Vec4 value) {
    if (vec->size >= vec->capacity) {
        size_t new_capacity = (vec->capacity == 0) ? 4 : vec->capacity * 2;
        Vec4* new_data = (Vec4*)realloc(vec->data, new_capacity * sizeof(Vec4));
        if (!new_data) {
            AddMessage("\nerror : vector_push_back/メモリの確保に失敗\n");
            return;
        }
        vec->data = new_data;
        vec->capacity = new_capacity;
    }
    vec->data[vec->size] = value;
    vec->size++;
}
//要素の設定
void Vec4_Set(Vec4Vector* vec, size_t index, Vec4 value) {
    if (index >= vec->size) {
        AddMessage("\nerror : vector_set/インデックス範囲外\n");
        return;
    }
    vec->data[index] = value;
}
//要素を取得
Vec4 Vec4_Get(Vec4Vector* vec, size_t index) {
    if (index >= vec->size) {
        AddMessage("\nerror : vector_get/インデックス範囲外\n");
        return { -1.0f,-1.0f,-1.0f,-1.0f };
    }
    return vec->data[index];
}
//解放
void Vec4_Free(Vec4Vector* vec) {
    free(vec->data);
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

//===============================
// Char 系
//===============================
void VecC_Init(CharVector* vec) {
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}
void VecC_PushBack(CharVector* vec, const char* str) {
    //if (!str) return;

    if (vec->size >= vec->capacity) {
        size_t new_capacity = (vec->capacity == 0) ? 4 : vec->capacity * 2;
        char** new_data = (char**)realloc(vec->data, new_capacity * sizeof(const char*));
        if (!new_data) {
            AddMessage("\nerror : charvector_push_back/メモリの確保に失敗\n");
            return;
        }
        vec->data = new_data;
        vec->capacity = new_capacity;
    }

    // コピーを確保して保存
    size_t len = strlen(str) + 1;
    char* copy = (char*)malloc(len);
    if (!copy) {
        AddMessage("\nerror : charvector_push_back/文字列コピー失敗\n");
        return;
    }
    memcpy(copy, str, len);

    vec->data[vec->size++] = copy;
}
const char* VecC_Get(CharVector* vec, size_t index) {
    if (!vec)
    {
        AddMessage("\nerror : charvector_get/ベクターがNULL\n");
        return NULL;
    }
    if (index >= vec->size) {
        AddMessage("\nerror : charvector_get/インデックス範囲外\n");
        return NULL;
    }
    return vec->data[index];
}
void VecC_Set(CharVector* vec, size_t index, const char* str) {
    if (index >= vec->size) {
        AddMessage("\nerror : charvector_set/インデックス範囲外\n");
        return;
    }
    vec->data[index] = NULL;

    size_t len = strlen(str) + 1;
    char* copy = (char*)malloc(len);
    memcpy(copy, str, len);

    vec->data[index] = copy;
}
void VecC_Free(CharVector* vec) {
    for (size_t i = 0; i < vec->size; i++) {
        free((void*)vec->data[i]);
    }
    free(vec->data);
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

//===============================
// Int 系
//===============================
void VecInt_Init(IntVector* vec) {
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}
void VecInt_PushBack(IntVector* vec, int str) {
    //if (!str) return;

    if (vec->size >= vec->capacity) {
        size_t new_capacity = (vec->capacity == 0) ? 4 : vec->capacity * 2;
        int* new_data = (int*)realloc(vec->data, new_capacity * sizeof(int));
        if (!new_data) {
            AddMessage("\nerror : intvector_push_back/メモリの確保に失敗\n");
            return;
        }
        vec->data = new_data;
        vec->capacity = new_capacity;
    }

    vec->data[vec->size++] = str;
}
int VecInt_Get(IntVector* vec, size_t index) {
    if (index >= vec->size) {
        AddMessage("\nerror : intvector_get/インデックス範囲外\n");
        return -1;
    }
    return vec->data[index];
}
void VecInt_Set(IntVector* vec, size_t index, int str) {
    if (index >= vec->size) {
        AddMessage("\nerror : intvector_set/インデックス範囲外\n");
        return;
    }
    vec->data[index] = str;
}
void VecInt_Free(IntVector* vec) {
    free(vec->data);
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

//===============================
// Bool 系
//===============================
void VecBool_Init(BoolVector* vec) {
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}
void VecBool_PushBack(BoolVector* vec, bool str) {
    if (vec->size >= vec->capacity) {
        size_t new_capacity = (vec->capacity == 0) ? 4 : vec->capacity * 2;
        bool* new_data = (bool*)realloc(vec->data, new_capacity * sizeof(bool));
        if (!new_data) {
            AddMessage("\nerror : bool_vector_push_back/メモリの確保に失敗\n");
            return;
        }
        vec->data = new_data;
        vec->capacity = new_capacity;
    }

    vec->data[vec->size++] = str;
}
bool VecBool_Get(BoolVector* vec, size_t index) {
    if (index >= vec->size) {
        AddMessage("\nerror : bool_vector_get/インデックス範囲外\n");
        return NULL;
    }
    return vec->data[index];
}
void VecBool_Set(BoolVector* vec, size_t index, bool str) {
    if (index >= vec->size) {
        AddMessage("\nerror : bool_vector_set/インデックス範囲外\n");
        return;
    }
    vec->data[index] = str;
}
void VecBool_Free(BoolVector* vec) {
    free(vec->data);
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

//===============================
// KeyMap 系
//===============================
// KeyMap 関数
void KeyMap_Init(KeyMap* map) {
    map->keys = NULL;
    map->size = 0;
    map->capacity = 0;
}
void KeyMap_Free(KeyMap* map) {
    for (size_t i = 0; i < map->size; i++) {
        free(map->keys[i]);
    }
    free(map->keys);
    map->keys = NULL;
    map->size = 0;
    map->capacity = 0;
}
int KeyMap_EnsureCapacity(KeyMap* map) {
    if (map->size >= map->capacity) {
        size_t new_capacity = (map->capacity == 0) ? 4 : map->capacity * 2;
        char** new_keys = (char**)realloc(map->keys, new_capacity * sizeof(char*));
        if (!new_keys) {
            AddMessage("\nerror : keymap_ensure_capacity/メモリの確保に失敗\n");
            return 0; // メモリ確保失敗
        }
        map->keys = new_keys;
        map->capacity = new_capacity;
    }
    return 1; // 成功
}
int KeyMap_Add(KeyMap* map, const char* key) {
    for (size_t i = 0; i < map->size; i++) {
        if (strcmp(map->keys[i], key) == 0) {
            printf("error: key '%s' already exists!\n", key);
            return -1; // 既存
        }
    }
    if (!KeyMap_EnsureCapacity(map)) return -1;

    map->keys[map->size] = _strdup(key);
    return (int)map->size++; // 登録したインデックスを返す
}
int KeyMap_GetIndex(KeyMap* map, const char* key) {
    for (size_t i = 0; i < map->size; i++) {
        if (strcmp(map->keys[i], key) == 0) {
            return (int)i; // 見つかった
        }
    }
    return -1; // 見つからなかった
}
const char* KeyMap_GetKey(KeyMap* map, int index) {
    if (index < 0 || (size_t)index >= map->size) {
        AddMessage("\nerror : keymap_getkey/インデックス範囲外\n");
        return NULL;
    }
    return map->keys[index];
}