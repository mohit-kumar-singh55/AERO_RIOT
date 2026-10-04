
struct VSInput
{
    float3 position : POSITION; // world-space
    float4 color : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

cbuffer Transform : register(b0)
{
    row_major float4x4 ViewProjection; // World is not needed as input position is already world-space
}

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    // convert to clip-space
    output.position = mul(float4(input.position, 1.0f), ViewProjection);
    output.color = input.color;
    return output;
}