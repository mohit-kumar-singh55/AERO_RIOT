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

RWStructuredBuffer<GPUParticle> Particles : register(u0);

cbuffer Simulation : register(b0)
{
    float DeltaTime;
    float3 EmitterPosition;

    uint SpawnCount;
    float3 Gravity;
}

[numthreads(256, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    uint particleCount;
    uint stride;
    
    // get how many particles actually exist
    Particles.GetDimensions(particleCount, stride);
    
    // check if current thread (particle) is out of (has exceeded) max particle capacity
    if (id.x >= particleCount)
        return;
    
    GPUParticle particle = Particles[id.x];
    
    if (particle.active == 0)
        return;
    
    particle.age += DeltaTime;
    
    particle.velocity += Gravity * DeltaTime;
    particle.position += particle.velocity * DeltaTime;
    
    if (particle.age >= particle.lifetime)
        particle.active = 0;
    
    Particles[id.x] = particle;
}