
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
    /*
    * dxtk stores matrices in row-major order,
    * but HLSL defaults to column-major constant-buffer storage
    */
    row_major float4x4 WVP; // MVP (Model * View * Projeciton) = WVP (World * View * Projection)
}

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), WVP);
    return output;
}