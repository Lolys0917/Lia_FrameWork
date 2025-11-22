// 処理順整理メモ
//  User
//  ↓依頼
//  ShaderManager※現在cpp
//  ↓作成&設定
//  ObjectManager
//  ↓Shaderをセット
//  描画
//

#include "Manager.h"

#include <d3dcompiler.h>

ID3DBlob* g_Default2DVSBlob = nullptr;
ID3DBlob* g_Default2DPSBlob = nullptr;
ID3DBlob* g_Default3DVSBlob = nullptr;
ID3DBlob* g_Default3DPSBlob = nullptr;

ID3D11VertexShader* g_Default2DVS = nullptr;
ID3D11PixelShader*  g_Default2DPS = nullptr;
ID3D11VertexShader* g_Default3DVS = nullptr;
ID3D11PixelShader*  g_Default3DPS = nullptr;

//キーマップ
Char2Vector g_VSList;
Char2Vector g_PSList;

//インデックス
static int g_ShaderVSIndex = 0, g_ShaderVSOldIndex = 0;
static int g_ShaderPSIndex = 0, g_ShaderPSOldIndex = 0;

static int g_Use2DPSIndex = 0;
static int g_Use2DVSIndex = 0;
static int g_Use3DPSIndex = 0;
static int g_Use3DVSIndex = 0;
static int g_Use3DGridVSIndex = 0;
static int g_Use3DGridPSIndex = 0;

//シェーダー保存
static ID3D11VertexShader* g_VSObject[1024];
static ID3D11PixelShader*  g_PSObject[1024];

static ID3D11VertexShader* g_VS2DShader;
static ID3D11PixelShader*  g_PS2DShader;
static ID3D11VertexShader* g_VS3DShader;
static ID3D11PixelShader*  g_PS3DShader;

static ID3DBlob* g_VSBlobObject[1024];

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
void ShaderManager_Init()
{
    Char2_Init(&g_VSList);
    Char2_Init(&g_PSList);

    g_ShaderVSIndex = g_ShaderPSIndex = 0;
    g_ShaderVSOldIndex = g_ShaderPSOldIndex = 0;

    // デフォルト 2D シェーダー登録
    InitShaderDefault();

    // 即コンパイル
    ShaderManager_Update();
}
//差分追加
void ShaderManager_Update()
{
    ID3D11Device* dev = GetDevice();
    if (!dev) return;

    // ---- PS ----
    while (g_ShaderPSOldIndex < g_ShaderPSIndex)
    {
        Char2 d = Char2_Get(&g_PSList, g_ShaderPSOldIndex);

        ID3DBlob* psBlob = nullptr;
        ID3DBlob* err = nullptr;

        HRESULT hr = D3DCompile(
            d.End, strlen(d.End),
            NULL, NULL, NULL,
            "PSMain", "ps_5_0",
            0, 0,
            &psBlob, &err);

        if (FAILED(hr))
        {
            AddMessage(ConcatCStr("PixelShader Compile Error: ", d.First));
            if (err) AddMessage((char*)err->GetBufferPointer());
        }
        else
        {
            dev->CreatePixelShader(
                psBlob->GetBufferPointer(),
                psBlob->GetBufferSize(),
                nullptr,
                &g_PSObject[g_ShaderPSOldIndex]);
        }

        if (err) err->Release();
        if (psBlob) psBlob->Release();

        g_ShaderPSOldIndex++;
    }

    // ---- VS ----
    while (g_ShaderVSOldIndex < g_ShaderVSIndex)
    {
        Char2 d = Char2_Get(&g_VSList, g_ShaderVSOldIndex);

        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* err = nullptr;

        HRESULT hr = D3DCompile(
            d.End, strlen(d.End),
            NULL, NULL, NULL,
            "VSMain", "vs_5_0",
            0, 0,
            &vsBlob, &err);

        if (FAILED(hr))
        {
            AddMessage(ConcatCStr("VertexShader Compile Error: ", d.First));
            if (err) AddMessage((char*)err->GetBufferPointer());
        }
        else
        {
            // シェーダー生成
            dev->CreateVertexShader(
                vsBlob->GetBufferPointer(),
                vsBlob->GetBufferSize(),
                nullptr,
                &g_VSObject[g_ShaderVSOldIndex]);

            // Blob を保存（Release しない）
            g_VSBlobObject[g_ShaderVSOldIndex] = vsBlob;
            vsBlob = nullptr;
        }

        if (err) err->Release();
        if (vsBlob) vsBlob->Release();

        g_ShaderVSOldIndex++;
    }
}


void Set2DShaderVS(const char* ShaderName)
{//デフォルトのシェーダーのインデックスを設定※Vertex
    int idx = Char2_GetIndex(&g_VSList, ShaderName);

    g_Use2DVSIndex = idx;
}
void Set2DShaderPS(const char* ShaderName)
{//デフォルトのシェーダーのインデックスを設定※Pixel
    int idx = Char2_GetIndex(&g_PSList, ShaderName);

    g_Use2DPSIndex = idx;
}
void Set3DShaderVS(const char* ShaderName)
{
    int idx = Char2_GetIndex(&g_VSList, ShaderName);

    g_Use3DVSIndex = idx;
}
void Set3DShaderPS(const char* ShaderName)
{
    int idx = Char2_GetIndex(&g_PSList, ShaderName);

    g_Use3DPSIndex = idx;
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


ID3D11VertexShader* GetVertexShader2D()
{
    int index = g_Use2DVSIndex;
    if (index < 0 || index >= g_ShaderVSOldIndex) return nullptr;
    return g_VSObject[index];
}
ID3D11PixelShader* GetPixelShader2D()
{
    int index = g_Use2DPSIndex;
    if (index < 0 || index >= g_ShaderPSOldIndex) return nullptr;
    return g_PSObject[index];
}
ID3D11VertexShader* GetVertexShader3D()
{
    int index = g_Use3DVSIndex;
    if (index < 0 || index >= g_ShaderVSOldIndex) return nullptr;
    return g_VSObject[index];
}
ID3D11PixelShader* GetPixelShader3D()
{
    int index = g_Use3DPSIndex;
    if (index < 0 || index >= g_ShaderPSOldIndex) return nullptr;
    return g_PSObject[index];
}
ID3D11VertexShader* GetVertexShader3DGrid()
{
    int index = g_Use3DGridVSIndex;
    if (index < 0 || index >= g_ShaderVSOldIndex) return nullptr;
    return g_VSObject[index];
}
ID3D11PixelShader* GetPixelShader3DGrid()
{
    int index = g_Use3DGridPSIndex;
    if (index < 0 || index >= g_ShaderPSOldIndex) return nullptr;
    return g_PSObject[index];
}

void InitShaderDefault()
{
    const char* VSDefault2D =
        R"EOT(
        cbuffer ConstantBuffer : register(b0)
        {
            matrix mvp; // 64バイト
            float4 diffuseColor; // 16バイト
            int useTexture; // 4バイト
            float3 pad; // 12バイト（アライメント調整）
        };
        
        struct VS_INPUT
        {
            float3 pos : POSITION;
            float2 uv : TEXCOORD0;
            //float3 nor : NORMAL;
        };
        
        struct PS_INPUT
        {
            float4 pos : SV_POSITION;
            float2 uv : TEXCOORD0;
            //float3 nor : NORMAL;
        };
        
        PS_INPUT VSMain(VS_INPUT input)
        {
            PS_INPUT output;
            output.pos = mul(float4(input.pos, 1.0f), mvp);
            output.uv = input.uv;
            //output.nor = input.nor;
            return output;
        }
        )EOT";
    AddVertexShader("DefaultVertexShader2D", VSDefault2D);
    g_Use2DVSIndex = 0;

    const char* PSDefault2D =
        R"EOT(
        cbuffer ColorBuffer : register(b1)
        {
            float4 color;
        }

        Texture2D tex0 : register(t0);
        SamplerState samp0 : register(s0);

        float4 PSMain(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
        {
            float4 texColor = tex0.Sample(samp0, uv);

        // 透明ピクセル部分の破棄
        if (texColor.a * color.a < 0.5f)
            discard;

        return texColor * color;
        }
        )EOT";
    AddPixelShader("DefaultPixelShader2D", PSDefault2D);
    g_Use2DPSIndex = 0;

    const char* VSDefault3D =
        R"EOT(
        cbuffer ConstantBuffer : register(b0)
        {
            matrix mvp; // 64バイト
            float4 diffuseColor; // 16バイト
            int useTexture; // 4バイト
            float3 pad; // 12バイト（アライメント調整）
        };
        
        struct VS_INPUT
        {
            float3 pos : POSITION;
            float2 uv : TEXCOORD0;
            //float3 nor : NORMAL;
        };
        
        struct PS_INPUT
        {
            float4 pos : SV_POSITION;
            float2 uv : TEXCOORD0;
            //float3 nor : NORMAL;
        };
        
        PS_INPUT VSMain(VS_INPUT input)
        {
            PS_INPUT output;
            output.pos = mul(float4(input.pos, 1.0f), mvp);
            output.uv = input.uv;
            //output.nor = input.nor;
            return output;
        }
        )EOT";
    AddVertexShader("DefaultVertexShader3D", VSDefault3D);
    g_Use3DVSIndex = 1;

    const char* PSDefault3D =
        R"EOT(
        cbuffer ColorBuffer : register(b1)
        {
            float4 color;       // diffuseColor と同じ扱い
        }
        
        Texture2D tex0 : register(t0);
        SamplerState samp0 : register(s0);
        
        float4 PSMain(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
        {
            // テクスチャ有無
            float4 texColor = tex0.Sample(samp0, uv);
        
            // αが弱いピクセルは捨てる
            if (texColor.a * color.a < 0.5f)
                discard;
        
            // 最終色 = テクスチャ × カラー
            return texColor * color;
        }
        )EOT";
    AddPixelShader("DefaultPixelShader3D", PSDefault3D);
    g_Use3DPSIndex = 1;

    const char* VSDefaultGrid =
        R"EOT(
        cbuffer ConstantBuffer : register(b0)
        {
            matrix viewProj;
            float4 lineColor;
        };
        
        struct VS_INPUT
        {
            float3 pos : POSITION;
        };
        
        struct PS_INPUT
        {
            float4 pos : SV_POSITION;
        };
        
        PS_INPUT VSMain(VS_INPUT input)
        {
            PS_INPUT output;
            output.pos = mul(float4(input.pos, 1.0f), viewProj);
            return output;
        }
        )EOT";

    AddVertexShader("DefaultVertexShader3DGrid", VSDefaultGrid);
    g_Use3DGridVSIndex = 2;

    const char* PSDefaultGrid =
        R"EOT(
        cbuffer ConstantBuffer : register(b0)
        {
            matrix viewProj;
            float4 lineColor;
        };
        
        struct PS_INPUT
        {
            float4 pos : SV_POSITION;
        };
        
        float4 PSMain(PS_INPUT input) : SV_TARGET
        {
            return lineColor;
        }
        )EOT";

    AddPixelShader("DefaultPixelShader3DGrid", PSDefaultGrid);
    g_Use3DGridPSIndex = 2;
}

ID3DBlob* GetCurrent2DVSBlob()
{
    int idx = g_Use2DVSIndex;

    if (idx < 0 || idx >= g_ShaderVSOldIndex)
        return nullptr;

    return g_VSBlobObject[idx];
}
ID3DBlob* GetCurrent3DVSBlob()
{
    int idx = g_Use3DVSIndex;

    if (idx < 0 || idx >= g_ShaderVSOldIndex)
        return nullptr;

    return g_VSBlobObject[idx];
}
ID3DBlob* GetCurrent3DGridVSBlob()
{
    int idx = g_Use3DGridVSIndex;

    if (idx < 0 || idx >= g_ShaderVSOldIndex)
        return nullptr;

    return g_VSBlobObject[idx];
}