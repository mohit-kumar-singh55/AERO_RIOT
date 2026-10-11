struct GPUParticle
{
    float3 position;
    float age;

    float3 velocity;
    float lifetime;

    float4 color;

    float size;
    float rotation;
    uint active;
    float padding;
};

StructuredBuffer<GPUParticle> Particles : register(t0); // t0 -> SRV

cbuffer RenderData : register(b0)
{
    row_major float4x4 ViewProjection;

    float3 CameraRight;
    float Padding0;

    float3 CameraUp;
    float Padding1;
}

float4 VSMain(
    uint vertexID : SV_VertexID,
    uint instanceID : SV_InstanceID
) : SV_POSITION
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);  // ? TEMP
}