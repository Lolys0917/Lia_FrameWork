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