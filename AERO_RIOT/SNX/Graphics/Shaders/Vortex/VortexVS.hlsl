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

// 2 triangles making a quad
static const float2 Corners[6] =
{
    float2(-0.5f, -0.5f),
    float2(-0.5f, 0.5f),
    float2(0.5f, 0.5f),

    float2(-0.5f, -0.5f),
    float2(0.5f, 0.5f),
    float2(0.5f, -0.5f)
};

struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

// vertexID : quad corner index
// instanceID : particle index
VSOutput VSMain(
    uint vertexID : SV_VertexID,
    uint instanceID : SV_InstanceID
)
{
    VSOutput output;
    
    GPUParticle particle = Particles[instanceID];
    float2 corner = Corners[vertexID];
    
    // if current particle is in-active, don't let it make real quad
    if (particle.active == 0)
    {
        float4 center = mul(float4(particle.position, 1.0f), ViewProjection);

        output.position = center;
        output.color = float4(0, 0, 0, 0);

        return output;
    }
    
    // billboarding (world pos of current vertex)
    float3 worldPosition =
        particle.position
        + CameraRight * corner.x * particle.size
        + CameraUp * corner.y * particle.size;
    
    // convert to clip-space
    output.position = mul(float4(worldPosition, 1.0f), ViewProjection);
    output.color = particle.color;
    
    return output;
}