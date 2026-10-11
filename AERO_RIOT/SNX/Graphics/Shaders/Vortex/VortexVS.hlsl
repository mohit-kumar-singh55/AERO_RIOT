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

cbuffer BillboardData : register(b0)
{
    float Pad_1;
    float3 CameraUP;
    
    float Pad_2;
    float3 CameraRight;
}

float4 VSMain(float4 pos : POSITION) : SV_POSITION
{
    return pos;
}