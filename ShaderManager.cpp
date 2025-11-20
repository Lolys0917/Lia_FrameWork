#include "Manager.h"

//キーマップ
Char2Vector g_VSList;
Char2Vector g_PSList;

//インデックス
static int g_ShaderVSIndex = 0, g_ShaderVSOldIndex;
static int g_ShaderPSIndex = 0, g_ShaderPSOldIndex;

//シェーダー保存
static ID3D11VertexShader* g_VSObject[1024];
static ID3D11PixelShader* g_PSObject[1024];

static ID3D11VertexShader* g_VS3DShader;
static ID3D11VertexShader* g_VS2DShader;
static ID3D11PixelShader* g_PS3DShader;
static ID3D11PixelShader* g_PS2DShader;

//追加
void AddVertexShader(const char* shaderName, const char* shaderCode)
{
	Char2_PushBack(&g_VSList, { shaderName, shaderCode });
	g_ShaderVSIndex++;
}
void AddPixelShader(const char* shaderName, const char* shaderCode)
{
	Char2_PushBack(&g_PSList, { shaderName, shaderCode });
	g_ShaderPSIndex++;
}
//差分追加
void ShaderManager_Update()
{
    ID3D11Device* dev = GetDevice();
    if (!dev) return;

    //======== Pixel Shader 生成（差分）========
    while (g_ShaderPSOldIndex < g_ShaderPSIndex)
    {
        Char2 d = Char2_Get(&g_PSList, g_ShaderPSOldIndex);

        ID3DBlob* psBlob = nullptr;
        ID3DBlob* err = nullptr;

        HRESULT hr = D3DCompile(
            d.End,
            strlen(d.End),
            NULL, NULL, NULL,
            "PSMain",
            "ps_5_0",
            0, 0,
            &psBlob, &err);

        if (FAILED(hr))
        {
            AddMessage(ConcatCStr("PixelShader Compile Error: ", d.First));
            if (err) AddMessage((char*)err->GetBufferPointer());
        }
        else
        {
            dev->CreatePixelShader(psBlob->GetBufferPointer(),
                psBlob->GetBufferSize(),
                nullptr,
                &g_PSObject[g_ShaderPSOldIndex]);
        }

        if (psBlob) psBlob->Release();
        if (err) err->Release();

        g_ShaderPSOldIndex++;
    }

    //======== Vertex Shader 生成（差分）========
    while (g_ShaderVSOldIndex < g_ShaderVSIndex)
    {
        Char2 d = Char2_Get(&g_VSList, g_ShaderVSOldIndex);

        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* err = nullptr;

        HRESULT hr = D3DCompile(
            d.End,
            strlen(d.End),
            NULL, NULL, NULL,
            "VSMain",
            "vs_5_0",
            0, 0,
            &vsBlob, &err);

        if (FAILED(hr))
        {
            AddMessage(ConcatCStr("VertexShader Compile Error: ", d.First));
            if (err) AddMessage((char*)err->GetBufferPointer());
        }
        else
        {
            dev->CreateVertexShader(vsBlob->GetBufferPointer(),
                vsBlob->GetBufferSize(),
                nullptr,
                &g_VSObject[g_ShaderVSOldIndex]);
        }

        if (vsBlob) vsBlob->Release();
        if (err) err->Release();

        g_ShaderVSOldIndex++;
    }
}

int GetVertexShaderIndex(const char* shaderName)
{
    for (int i = 0; i < g_ShaderVSIndex; i++)
    {
        Char2 d = Char2_Get(&g_VSList, i);
        if (strcmp(d.First, shaderName) == 0)
            return i;
    }
    return -1;
}

int GetPixelShaderIndex(const char* shaderName)
{
    for (int i = 0; i < g_ShaderPSIndex; i++)
    {
        Char2 d = Char2_Get(&g_PSList, i);
        if (strcmp(d.First, shaderName) == 0)
            return i;
    }
    return -1;
}

ID3D11VertexShader* GetVertexShader(int index)
{
    if (index < 0 || index >= g_ShaderVSOldIndex) return nullptr;
    return g_VSObject[index];
}

ID3D11PixelShader* GetPixelShader(int index)
{
    if (index < 0 || index >= g_ShaderPSOldIndex) return nullptr;
    return g_PSObject[index];
}
