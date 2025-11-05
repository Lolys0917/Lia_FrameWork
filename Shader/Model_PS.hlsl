cbuffer ConstantBuffer : register(b0)
{
    matrix mvp; // 64バイト
    float4 diffuseColor; // 16バイト
    int useTexture; // 4バイト
    float3 pad; // 12バイト
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 nor : NORMAL;
};

Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);

float4 PSMain(PS_INPUT input) : SV_Target
{
    if (useTexture == 1)
    {
        return tex0.Sample(samp0, input.uv) * diffuseColor;
    }
    else
    {
        return diffuseColor;
    }
}