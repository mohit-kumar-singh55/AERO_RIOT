#include "pch.h"
#include "VortexParticleEmitter.h"

#include <SNX/Graphics/RenderContext.h>
#include <SNX/Utils/ErrorHandler.h>
#include <SNX/Utils/FileHandling.h>
#include <SNX/Core/Time.h>
#include <SNX/Core/Components/Transform.h>

#include <vector>

VortexParticleEmitter::VortexParticleEmitter(
	GameObject& gameObject,
	std::uint32_t maxCapacity
) noexcept :
	Renderer(gameObject) {
	m_capacity = std::max(1u, maxCapacity);

	// ceil(m_capacity / THREAD_GROUP_SIZE)
	m_groupCount =
		(m_capacity + THREAD_GROUP_SIZE - 1)
		/ THREAD_GROUP_SIZE;
}

void VortexParticleEmitter::Initialize(const RenderContext& context) {
	if (m_isInitialized) return;

	using namespace ErrorHandler;
	using namespace FileHandling;

	// ! create particle buffer compatible with SRV & UAV
	D3D11_BUFFER_DESC structuredBufferDesc{};
	structuredBufferDesc.ByteWidth = sizeof(GPUParticle) * m_capacity;
	structuredBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	structuredBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	structuredBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	structuredBufferDesc.StructureByteStride = sizeof(GPUParticle);

	std::vector<GPUParticle> initialParticles(m_capacity);
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

	// ! create SRV to access particle buffer read-only from VS
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

	// ! create UAV to access particle buffer read/write from CS
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

	// ! create one-element structured buffer used as an atomic spawn counter
	D3D11_BUFFER_DESC spawnCounterDesc{};
	spawnCounterDesc.ByteWidth = sizeof(std::uint32_t);
	spawnCounterDesc.Usage = D3D11_USAGE_DEFAULT;
	spawnCounterDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	spawnCounterDesc.CPUAccessFlags = 0;
	spawnCounterDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	spawnCounterDesc.StructureByteStride = sizeof(std::uint32_t);

	std::uint32_t initialSpawnCounter = 0;
	D3D11_SUBRESOURCE_DATA initialSpawnCounterData{};
	initialSpawnCounterData.pSysMem = &initialSpawnCounter;

	ThrowIfFailed(
		context.device->CreateBuffer(
			&spawnCounterDesc,
			&initialSpawnCounterData,
			&m_spawnCounterBuffer
		),
		"VortexParticleEmitter::Initialize: Unable to create spawn counter buffer"
	);

	D3D11_UNORDERED_ACCESS_VIEW_DESC spawnCounterUAVDesc{};
	spawnCounterUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
	spawnCounterUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	spawnCounterUAVDesc.Buffer.FirstElement = 0;
	spawnCounterUAVDesc.Buffer.NumElements = 1;

	ThrowIfFailed(
		context.device->CreateUnorderedAccessView(
			m_spawnCounterBuffer.Get(),
			&spawnCounterUAVDesc,
			&m_spawnCounterUAV
		),
		"VortexParticleEmitter::Initialize: Unable to create spawn counter UAV"
	);

	// ! create constant buffer for simulation data
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

	// ! create render data constant buffer
	D3D11_BUFFER_DESC renderDataDesc{};
	renderDataDesc.ByteWidth = sizeof(ParticleRenderBuffer);
	renderDataDesc.Usage = D3D11_USAGE_DEFAULT;
	renderDataDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	renderDataDesc.CPUAccessFlags = 0;

	ThrowIfFailed(
		context.device->CreateBuffer(
			&renderDataDesc,
			nullptr,
			&m_renderConstantBuffer
		),
		"VortexParticleEmitter::Initialize: Unable to create render constant buffer"
	);

	// ! create vertex shader
	auto vsBytecode = LoadShaderBytecode("Shaders\\VortexVS.cso");
	ThrowIfFailed(
		context.device->CreateVertexShader(
			vsBytecode.data(),
			vsBytecode.size(),
			nullptr,
			&m_particleVS
		),
		"VortexParticleEmitter::Initialize: Unable to create Vertex Shader"
	);

	// ! create pixel shader
	auto psBytecode = LoadShaderBytecode("Shaders\\VortexPS.cso");
	ThrowIfFailed(
		context.device->CreatePixelShader(
			psBytecode.data(),
			psBytecode.size(),
			nullptr,
			&m_particlePS
		),
		"VortexParticleEmitter::Initialize: Unable to create Pixel Shader"
	);

	m_isInitialized = true;
}

void VortexParticleEmitter::Draw(const RenderContext& context) {
	// ! one time initialization
	Initialize(context);

	const float deltaTime = Time::DeltaTime();

	// ! prepare simulation data
	// accumulate the no. of particle for this frame
	m_spawnAccumulator += m_emissionRate * deltaTime;
	// get the actual round int no. of particles to spawn thie frame
	std::uint32_t spawnCount = static_cast<std::uint32_t>(m_spawnAccumulator);
	// store the fraction part for next use
	m_spawnAccumulator -= spawnCount;

	ParticleSimulationBuffer simulationData{};
	simulationData.deltaTime = deltaTime;
	simulationData.emitterPosition = GetTransform().GetRenderPosition();
	simulationData.gravity = m_gravity;
	simulationData.spawnCount = spawnCount;
	simulationData.lifetime = m_lifetime;
	simulationData.startColor = m_startColor;
	simulationData.startSize = m_startSize;
	simulationData.startVelocity = m_startVelocity;

	context.deviceContext->UpdateSubresource(
		m_simulationConstantBuffer.Get(),
		0,
		nullptr,
		&simulationData,
		0,
		0
	);

	// Reset the atomic spawn counter before this frame's compute dispatch
	const UINT clearSpawnCounter[4] = { 0, 0, 0, 0 };
	context.deviceContext->ClearUnorderedAccessViewUint(
		m_spawnCounterUAV.Get(),
		clearSpawnCounter
	);

	// bind simulation constant buffer
	ID3D11Buffer* simulationBuffer = m_simulationConstantBuffer.Get();
	context.deviceContext->CSSetConstantBuffers(
		0,
		1,
		&simulationBuffer
	);

	// u0 = particle buffer, u1 = spawn counter
	ID3D11UnorderedAccessView* computeUAVs[] = {
		m_particleUAV.Get(),
		m_spawnCounterUAV.Get()
	};
	context.deviceContext->CSSetUnorderedAccessViews(
		0,
		_countof(computeUAVs),
		computeUAVs,
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

	// ! unbind UAVs before reading the particle buffer as an SRV
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };
	context.deviceContext->CSSetUnorderedAccessViews(
		0,
		_countof(nullUAVs),
		nullUAVs,
		nullptr
	);
	context.deviceContext->CSSetShader(
		nullptr,
		nullptr,
		0
	);

	// prepare render constant buffer data
	ParticleRenderBuffer renderData{};
	renderData.viewProjection = context.view * context.projection;
	renderData.cameraRight = context.cameraRight;
	renderData.cameraUp = context.cameraUp;

	context.deviceContext->UpdateSubresource(
		m_renderConstantBuffer.Get(),
		0,
		nullptr,
		&renderData,
		0,
		0
	);

	// bind render constant buffer
	ID3D11Buffer* renderDataBuffer = m_renderConstantBuffer.Get();
	context.deviceContext->VSSetConstantBuffers(
		0,
		1,
		&renderDataBuffer
	);

	// bind particle SRV at t0
	auto* particleSRV = m_particleSRV.Get();
	context.deviceContext->VSSetShaderResources(
		0,
		1,
		&particleSRV
	);

	// no vertex buffer/input layout; VS generates quad corners from SV_VertexID
	context.deviceContext->IASetInputLayout(nullptr);
	context.deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context.deviceContext->VSSetShader(
		m_particleVS.Get(),
		nullptr,
		0
	);

	context.deviceContext->PSSetShader(
		m_particlePS.Get(),
		nullptr,
		0
	);

	context.deviceContext->DrawInstanced(
		6,
		m_capacity,
		0,
		0
	);

	// ! unbind SRV so the same particle buffer can become a UAV next frame
	ID3D11ShaderResourceView* nullSRV = nullptr;
	context.deviceContext->VSSetShaderResources(
		0,
		1,
		&nullSRV
	);
}
