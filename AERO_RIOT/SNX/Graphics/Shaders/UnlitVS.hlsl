
struct VSInput
{
    float3 position : SV_Position;
};

struct VSOutput
{
    float4 position : SV_Position; // clip-space
};

cbuffer Transform : register(b0)
{
    float4x4 WVP; // MVP (Model * View * Projeciton) = WVP (World * View * Projection)
}

VSOutput VSMain(VSInput input : SV_POSITION) : SV_POSITION
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), WVP);
    return output;
}