#pragma once

#include "Renderer.h"

#include <wrl/client.h>
#include <d3d11.h>

#include <SimpleMath.h>

struct GPUParticle {
	DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
	float age = 0.0f;

	DirectX::SimpleMath::Vector3 velocity = DirectX::SimpleMath::Vector3::Zero;
	float lifetime = 1.0f;

	DirectX::SimpleMath::Vector4 color{ 1.0f,1.0f,1.0f,1.0f };

	float size = 1.0f;
	float rotation = 0.0f;
	std::uint32_t active = 0u;	// is active?
	float padding = 0.0f;
};

struct ParticleSimulationBuffer {
	float deltaTime;
	DirectX::SimpleMath::Vector3 emitterPosition;

	std::uint32_t spawnCount;
	DirectX::SimpleMath::Vector3 gravity;
};

struct ParticleRenderBuffer {
	DirectX::SimpleMath::Matrix viewProjection;

	DirectX::SimpleMath::Vector3 cameraRight;
	float padding0 = 0.0f;

	DirectX::SimpleMath::Vector3 cameraUp;
	float padding1 = 0.0f;
};

class VortexParticleEmitter : public Renderer {
public:
	VortexParticleEmitter(
		GameObject& gameObject,
		std::uint32_t maxCapacity = 8192	// max particle it can hold
	) noexcept;

	[[nodiscard]]
	RenderPass GetRenderPass() const noexcept override { return RenderPass::Transparent; }

	void SetEmissionRate(float particlesPerSecond) noexcept {
		m_emissionRate = std::max(0.0f, particlesPerSecond);
	}

	[[nodiscard]]
	float GetEmissionRate() const noexcept { return m_emissionRate; }

protected:
	void Draw(const RenderContext& context) override;

private:
	void Initialize(const RenderContext& context);

private:
	bool m_isInitialized = false;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_particleBuffer;					// GPU particle storage
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_particleSRV;			// same buffer, read-only view
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_particleUAV;		// same buffer, read/write view
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_updateCS;					// simulation
	Microsoft::WRL::ComPtr<ID3D11Buffer>  m_simulationConstantBuffer;		// game data

	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_particleVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_particlePS;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_renderConstantBuffer;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_spawnCounterBuffer;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_spawnCounterUAV;

	std::uint32_t m_capacity;	// max GPU particle slots

	std::uint32_t m_groupCount;	// no. of thread groups

	static constexpr std::uint32_t THREAD_GROUP_SIZE = 256;	// no. of threads in each group

	float m_emissionRate = 10.0f;		// particles/sec
	float m_spawnAccumulator = 0.0f;	// remaining particles to spawn for this second
};