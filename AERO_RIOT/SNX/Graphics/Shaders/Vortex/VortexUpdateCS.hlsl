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

RWStructuredBuffer<GPUParticle> Particles : register(u0); // u0 ->  UAV
RWStructuredBuffer<uint> SpawnCounter : register(u1);

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
    
    // update particle
    if (particle.active == 1)
    {
        particle.age += DeltaTime;
    
        particle.velocity += Gravity * DeltaTime;
        particle.position += particle.velocity * DeltaTime;
    
        if (particle.age >= particle.lifetime)
            particle.active = 0;
    
        Particles[id.x] = particle;
        
        return;
    }
    
    // else find available/dead slot
    uint spawnIndex;
    
    InterlockedAdd(SpawnCounter[0], 1, spawnIndex);
    
    if (spawnIndex >= SpawnCount)
        return;
    
    // initialize particle
    particle.age = 0;
    particle.active = 1;
    particle.size = 0.2f;
    particle.lifetime = 3.0f;
    particle.position = float3(0.0f, 0.0f, 0.0f);
    particle.velocity = float3(0.0f, 2.0f, 0.0f);
    particle.color = float4(1.0f, 0.0f, 1.0f, 1.0f);
    
    SpawnCount = 5;
}