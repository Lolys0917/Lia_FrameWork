// grid_vs.hlsl

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