cbuffer Material : register(b0)
{
    float4 Color;
    float4 Emission;
}

cbuffer Frame : register(b1)
{
    float ElapsedTime;
    float3 _Padding;
}

float4 PSMain() : SV_TARGET
{
    // remap [-1,1] into [0,1]
    float pulse = sin(ElapsedTime) * 0.5f + 0.5f;
    float3 finalEmission = Emission.rbg * pulse;
    return float4(Color.rgb + finalEmission, Color.a);
}