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
ID3D11VertexShader* g_Default2DVS = nullptr;
ID3D11PixelShader* g_Default2DPS = nullptr;

//キーマップ
Char2Vector g_VSList;
Char2Vector g_PSList;

//インデックス
static int g_ShaderVSIndex = 0, g_ShaderVSOldIndex;
static int g_ShaderPSIndex = 0, g_ShaderPSOldIndex;

static int g_UsePSIndex = 0;
static int g_UseVSIndex = 0;

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

void ShaderManager_Init()
{
    // ====== 2D Vertex Shader ======
    D3DCompileFromFile(
        L"Shader/2D_VS.hlsl",
        nullptr, nullptr,
        "VSMain", "vs_5_0",
        0, 0,
        &g_Default2DVSBlob,
        nullptr
    );
    GetDevice()->CreateVertexShader(
        g_Default2DVSBlob->GetBufferPointer(),
        g_Default2DVSBlob->GetBufferSize(),
        nullptr,
        &g_Default2DVS
    );

    // ====== 2D Pixel Shader ======
    D3DCompileFromFile(
        L"Shader/2D_PS.hlsl",
        nullptr, nullptr,
        "PSMain", "ps_5_0",
        0, 0,
        &g_Default2DPSBlob,
        nullptr
    );
    GetDevice()->CreatePixelShader(
        g_Default2DPSBlob->GetBufferPointer(),
        g_Default2DPSBlob->GetBufferSize(),
        nullptr,
        &g_Default2DPS
    );
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


void SetShaderVS(const char* ShaderName)
{//デフォルトのシェーダーのインデックスを設定※Vertex
    int idx = Char2_GetIndex(&g_VSList, ShaderName);

    g_UseVSIndex = idx;
}
void SetShaderPS(const char* ShaderName)
{//デフォルトのシェーダーのインデックスを設定※Pixel
    int idx = Char2_GetIndex(&g_PSList, ShaderName);

    g_UsePSIndex = idx;
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
    int index = g_UseVSIndex;
    if (index < 0 || index >= g_ShaderVSOldIndex) return nullptr;
    return g_VSObject[index];
}

ID3D11PixelShader* GetPixelShader2D()
{
    int index = g_UsePSIndex;
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
    g_UseVSIndex = 0;

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
    g_UsePSIndex = 0;
}