// grid_ps.hlsl

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