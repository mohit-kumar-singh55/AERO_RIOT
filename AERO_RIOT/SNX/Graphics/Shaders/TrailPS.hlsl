cbuffer Material : register(b0)
{
    float4 Color;
}

float4 PSMain() : SV_TARGET
{
    // return float4(Color.rgb,Color.a*);
    return Color;
}