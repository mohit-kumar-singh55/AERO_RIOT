#include "pch.h"
#include "VortexParticleEmitter.h"

#include <SNX/Graphics/RenderContext.h>
#include <SNX/Utils/ErrorHandler.h>
#include <SNX/Utils/FileHandling.h>
#include <SNX/Core/Time.h>
#include <SNX/Core/Components/Transform.h>

#include <vector>
#include <algorithm>

VortexParticleEmitter::VortexParticleEmitter(
	GameObject& gameObject,
	std::uint32_t maxCapacity
) noexcept :
	Renderer(gameObject) {
	m_capacity = std::max(1u, maxCapacity);

	// calc. total no. of thread groups
	m_groupCount =
		(m_capacity + THREAD_GROUP_SIZE - 1)
		/ THREAD_GROUP_SIZE;	// -1 for integer ceiling division
}

void VortexParticleEmitter::Initialize(const RenderContext& context) {
	if (m_isInitialized) return;

	using namespace ErrorHandler;
	using namespace FileHandling;

	// ! create buffer compatible with SRV & UAV
	D3D11_BUFFER_DESC structuredBufferDesc{};
	structuredBufferDesc.ByteWidth = sizeof(GPUParticle) * m_capacity;
	structuredBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	structuredBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;	// it allows us to create SRV & UAV & make shader able to read/write this buffer as a shader resource
	structuredBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;	// it allows to treat this buffer as an array of fixed-size structures
	structuredBufferDesc.StructureByteStride = sizeof(GPUParticle);

	// setting initial data
	std::vector<GPUParticle> initialParticles(m_capacity);
	// ? TEMP ****
	for (std::size_t i = 0;i < 8;i++) {
		initialParticles[i].active = 1;
		initialParticles[i].lifetime = 1000.0f;
		initialParticles[i].size = 0.3f + i;
		initialParticles[i].velocity = { 0.0f,(2.0f * i),0.0f };
		initialParticles[i].color = { 1.0f,0.0f,1.0f,1.0f };
	}
	// ? *********
	D3D11_SUBRESOURCE_DATA initialParticleData{};
	initialParticleData.pSysMem = initialParticles.data();

	ThrowIfFailed(
		context.device->CreateBuffer(
			&structuredBufferDesc,
			&initialParticleData,
			&m_particleBuffer
		),
		"VortexParticleEmitter::Initialize: Unable to create structured buffer"
	);

	// ! create SRV to access (read-only) particle buffer from VS
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = m_capacity;

	ThrowIfFailed(
		context.device->CreateShaderResourceView(
			m_particleBuffer.Get(),
			&srvDesc,
			&m_particleSRV
		),
		"VortexParticleEmitter::Initialize: Unable to create SRV"
	);

	// ! create UAV to access (read/write) particle buffer from CS
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = m_capacity;

	ThrowIfFailed(
		context.device->CreateUnorderedAccessView(
			m_particleBuffer.Get(),
			&uavDesc,
			&m_particleUAV
		),
		"VortexParticleEmitter::Initialize: Unable to create UAV"
	);

	// ! create constant buffer for additional game data
	D3D11_BUFFER_DESC simulationBufferDesc{};
	simulationBufferDesc.ByteWidth = sizeof(ParticleSimulationBuffer);
	simulationBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	simulationBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	simulationBufferDesc.CPUAccessFlags = 0;

	ThrowIfFailed(
		context.device->CreateBuffer(
			&simulationBufferDesc,
			nullptr,
			&m_simulationConstantBuffer
		),
		"VortexParticleEmitter::Initialize: Unable to create simulation buffer"
	);

	// ! load & create CS
	auto csBytecode = LoadShaderBytecode("Shaders\\VortexUpdateCS.cso");

	ThrowIfFailed(
		context.device->CreateComputeShader(
			csBytecode.data(),
			csBytecode.size(),
			nullptr,
			&m_updateCS
		),
		"VortexParticleEmitter::Initialize: Unable to create Compute Shader"
	);

	m_isInitialized = true;
}

void VortexParticleEmitter::Draw(const RenderContext& context) {
	// ! one time initialization
	Initialize(context);

	// prepare simulation data
	ParticleSimulationBuffer simulationData{};
	simulationData.deltaTime = Time::DeltaTime();
	simulationData.emitterPosition = GetTransform().GetRenderPosition();
	simulationData.gravity = { 0.0f, -2.0f, 0.0f };
	simulationData.spawnCount = 0;

	context.deviceContext->UpdateSubresource(
		m_simulationConstantBuffer.Get(),
		0,
		nullptr,
		&simulationData,
		0,
		0
	);

	// bind buffers
	ID3D11Buffer* simulationBuffer = m_simulationConstantBuffer.Get();
	context.deviceContext->CSSetConstantBuffers(
		0,
		1,
		&simulationBuffer
	);

	auto* particleUAV = m_particleUAV.Get();
	context.deviceContext->CSSetUnorderedAccessViews(
		0,
		1,
		&particleUAV,
		nullptr
	);

	// set compute shader
	context.deviceContext->CSSetShader(
		m_updateCS.Get(),
		nullptr,
		0
	);

	// ! dispatch compute shader
	context.deviceContext->Dispatch(m_groupCount, 1, 1);

	// ! IMP: after dispatch, unbind particle buffer from UAV (necessary for binding it with SRV later)
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	context.deviceContext->CSSetUnorderedAccessViews(
		0,
		1,
		&nullUAV,
		nullptr
	);
	context.deviceContext->CSSetShader(
		nullptr,
		nullptr,
		0
	);
}