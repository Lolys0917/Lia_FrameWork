cbuffer ColorBuffer : register(b1)
{
    float4 color;
}

Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);

float4 PSMain(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
    float4 texColor = tex0.Sample(samp0, uv);

    // “§–¾ƒsƒNƒZƒ‹•”•ª‚Ì”jŠü
    if (texColor.a * color.a < 0.5f)
        discard;

    return texColor * color;
}