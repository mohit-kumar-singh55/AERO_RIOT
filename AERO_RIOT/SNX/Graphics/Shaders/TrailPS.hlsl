cbuffer Material : register(b0)
{
    float4 Color;
}

// output of the VS
struct PSInput
{
    float4 position : SV_POSITION;
    float alpha : ALPHA;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(Color.rgb, Color.a * saturate(input.alpha));
}