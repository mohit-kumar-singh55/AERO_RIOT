cbuffer Material : register(b0)
{
    float4 Color; // tint/opacity
}

// output of the VS
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 rgb = Color.rgb * input.color.rgb;
    float alpha = Color.a * saturate(input.color.a);
    return float4(rgb, alpha);
}