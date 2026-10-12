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

RWStructuredBuffer<GPUParticle> Particles : register(u0); // u0 -> particle UAV
RWStructuredBuffer<uint> SpawnCounter : register(u1); // u1 -> atomic spawn counter

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

    Particles.GetDimensions(particleCount, stride);

    if (id.x >= particleCount)
        return;

    GPUParticle particle = Particles[id.x];

    // Update an already-live particle.
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

    // Dead slots compete for this frame's limited number of spawn requests
    uint spawnIndex;
    InterlockedAdd(SpawnCounter[0], 1, spawnIndex); // aomically increments the spawn counter and returns the previous value as a unique spawn index

    if (spawnIndex >= SpawnCount)
        return;

    // Temporary deterministic spawn values.
    particle.position = EmitterPosition;
    particle.age = 0.0f;

    particle.velocity = float3(0.0f, 2.0f, 0.0f);
    particle.lifetime = 3.0f;

    particle.color = float4(1.0f, 0.0f, 1.0f, 1.0f);

    particle.size = 0.2f;
    particle.rotation = 0.0f;
    particle.active = 1;
    particle.padding = 0.0f;

    Particles[id.x] = particle;
}
