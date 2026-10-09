#include "pch.h"
#include "VortexParticleEmitter.h"

#include <SNX/Graphics/RenderContext.h>

VortexParticleEmitter::VortexParticleEmitter(
	GameObject& gameObject,
	std::uint32_t maxCapacity
) noexcept :
	Renderer(gameObject),
	m_capacity(maxCapacity) {}

void VortexParticleEmitter::Initialize(const RenderContext& context) {
	if (m_isInitialized) return;
	m_isInitialized = true;

	// ! create buffer compatible with SRV & UAV
	D3D11_BUFFER_DESC structuredBufferDesc{};
	structuredBufferDesc.ByteWidth = sizeof(GPUParticle) * m_capacity;
	structuredBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	structuredBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;			// it allows us to create SRV & make shader able to read this buffer as a shader resource
	structuredBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;	// it says: treat this buffer as an array of fixed-size structures
	structuredBufferDesc.StructureByteStride = sizeof(GPUParticle);

	context.device->CreateBuffer(
		&structuredBufferDesc,
		nullptr,
		&m_particleBuffer
	);

	// ! create SRV to access (read-only) particle buffer from VS
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = m_capacity;

	context.device->CreateShaderResourceView(
		m_particleBuffer.Get(),
		&srvDesc,
		&m_particleSRV
	);

	// ! create SRV to access (read/write) particle buffer from CS
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = m_capacity;

	context.device->CreateUnorderedAccessView(
		m_particleBuffer.Get(),
		&uavDesc,
		&m_particleUAV
	);

	// ! create constant buffer for additional game data
	D3D11_BUFFER_DESC simulationBufferDesc{};
	simulationBufferDesc.ByteWidth = sizeof(ParticleSimulationBuffer);
	simulationBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	simulationBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	simulationBufferDesc.CPUAccessFlags = 0;

	context.device->CreateBuffer(
		&simulationBufferDesc,
		nullptr,
		&m_simulationConstantBuffer
	);

	// TODO: load & create CS
}

void VortexParticleEmitter::Draw(const RenderContext& context) {
	// ! one time initialization
	Initialize(context);
}