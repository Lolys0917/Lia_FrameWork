cbuffer MatrixBuffer : register(b0)
{
    matrix mvp;
};

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT output;
    output.pos = mul(float4(input.pos, 1), mvp);
    output.uv = input.uv;
    return output;
}
