#include "pch.h"
#include "TrailRenderer.h"

#include <SNX/Graphics/RenderContext.h>
#include <SNX/Core/Object/GameObject.h>
#include <SNX/Core/Components/Transform.h>
#include <SNX/Core/Time.h>
#include <SNX/Utils/ErrorHandler.h>

#include <algorithm>
#include <cstring>

TrailRenderer::TrailRenderer(GameObject& gameObject) noexcept
	: Renderer(gameObject) {}

void TrailRenderer::OnUpdate() {
	using DirectX::SimpleMath::Vector3;

	// increase age & remove expired points
	for (auto i = m_points.begin();i != m_points.end();) {
		i->age += Time::DeltaTime();

		if (i->age >= m_lifeTime + 0.05f)	// a small increment so triangle gets enought time to become transparent
			i = m_points.erase(i);
		else
			i++;
	}

	// add new point
	auto currentPos = GetTransform().GetRenderPosition();

	if (m_points.empty())
		m_points.push_back({ currentPos, 0.0f });
	else {
		float distanceSquared = Vector3::DistanceSquared(currentPos, m_points.back().position);
		if (distanceSquared >= m_minPointDistanceSquared)
			m_points.push_back({ currentPos, 0.0f });
	}
}

void TrailRenderer::Draw(const RenderContext& context) {
	using DirectX::SimpleMath::Vector3;
	using DirectX::SimpleMath::Vector4;

	// clear previous frame vertices
	m_vertices.clear();

	// atleast 2 points are required to generate ribbon
	if (m_points.size() < 2)
		return;

	// calculated total length of the trail
	float totalLength = 0.0f;
	for (std::size_t i = 0; i < m_points.size() - 1; i++)
		totalLength += Vector3::Distance(m_points[i].position, m_points[i + 1].position);

	// ! create vertices
	Vector3 previousSide = Vector3::Zero;
	std::size_t lastIndex = m_points.size() - 1;
	for (std::size_t i = 0; i < m_points.size(); i++) {
		// trail length upto current point
		float accumulatedLength = 0.0f;
		if (i != 0)
			accumulatedLength = Vector3::Distance(m_points[i - 1].position, m_points[i].position);

		Vector3 trailDir;

		// first point
		if (i == 0)
			trailDir = m_points[i + 1].position - m_points[i].position;
		// last point
		else if (i == lastIndex)
			trailDir = m_points[i].position - m_points[i - 1].position;
		// middle points
		else
			trailDir = m_points[i + 1].position - m_points[i - 1].position;

		// billboarding
		Vector3 dirToCamera = context.cameraPosition - m_points[i].position;
		dirToCamera.Normalize();
		trailDir.Normalize();

		Vector3 sideDirOfPoint = trailDir.Cross(dirToCamera);

		if (sideDirOfPoint.LengthSquared() > 0.0001f)
			sideDirOfPoint.Normalize();
		else {
			if (previousSide != Vector3::Zero)
				sideDirOfPoint = previousSide;
			else
				sideDirOfPoint = GetTransform().GetRight();
		}

		// prevent from sudden direction flipping
		if (sideDirOfPoint.Dot(previousSide) < 0)
			sideDirOfPoint = -sideDirOfPoint;
		previousSide = sideDirOfPoint;

		// vertex position on left & right sides of the trail point
		Vector3 left = m_points[i].position - sideDirOfPoint * m_halfWidth;
		Vector3 right = m_points[i].position + sideDirOfPoint * m_halfWidth;

		float gradientPosition = accumulatedLength / totalLength;
		Vector4 color = m_gradient.Evaluate(gradientPosition);

		m_vertices.push_back({ left, color });
		m_vertices.push_back({ right, color });
	}

	// ! lazy create TrailEffect and input layout (one time creation)
	if (!m_trailEffect)
		CreateEffectAndInputLayout(context);

	// ! create dynamic buffer / update its size
	if (m_vertices.size() > m_vertexCapacity)
		EnsureVertexBufferCapacity(context);

	// ! update vertex buffer using Map/Unmap
	// to access/write GPU resource memory from CPU
	D3D11_MAPPED_SUBRESOURCE mapped{};

	// gives pointer to the resource memory
	ErrorHandler::ThrowIfFailed(
		context.deviceContext->Map(
			m_vertexBuffer.Get(),
			0,
			D3D11_MAP_WRITE_DISCARD,	// forces gpu to discard old content & give cpu memory to write new data
			0,
			&mapped						// mapped.pData has the pointer to the resource memory
		),
		"Unable to map trail vertex buffer"
	);

	// "memory copy" It copies a specified number of bytes from one memory location to another.
	memcpy(
		mapped.pData,							// destination
		m_vertices.data(),						// source
		sizeof(TrailVertex) * m_vertices.size()	// no. of bytes
	);

	// CPU is finished writing; GPU may use this resource again
	context.deviceContext->Unmap(m_vertexBuffer.Get(), 0);

	// ! bind vertex buffer to input assembler
	ID3D11Buffer* vertexBuffer = m_vertexBuffer.Get();
	UINT stride = sizeof(TrailVertex);
	UINT offset = 0;

	context.deviceContext->IASetInputLayout(m_inputLayout.Get());
	context.deviceContext->IASetVertexBuffers(
		0,				// start slot
		1,				// no. of buffers
		&vertexBuffer,	// buffers
		&stride,		// size of buffers
		&offset			// offset
	);

	// tell IA that this is a triangle strip (m_vertices ordering is necessary)
	context.deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// ! apply effect
	m_trailEffect->SetViewProjection(context.view * context.projection);
	m_trailEffect->SetMaterial({ 1.0f,1.0f,1.0f,1.0f });
	m_trailEffect->Apply(context.deviceContext);

	// ! set necessary states
	// set rasterizer state cull mode to NONE, otherwise backside will not be visible
	ID3D11RasterizerState* oldRasterizerState = nullptr;
	context.deviceContext->RSGetState(&oldRasterizerState);
	context.deviceContext->RSSetState(m_trailRasterizerState.Get());

	// set blend state, otherwise even with alpha, trail will not become transparent
	float blendFactor[4];
	UINT sampleMask;
	ID3D11BlendState* oldBlendState = nullptr;
	context.deviceContext->OMGetBlendState(&oldBlendState, blendFactor, &sampleMask);
	context.deviceContext->OMSetBlendState(m_trailBlendState.Get(), nullptr, 0xffffffff);

	// set depth stencil state so trail's invisible part doesn't block object behind it
	ID3D11DepthStencilState* oldDepthState = nullptr;
	UINT oldStencilRef = 0;
	context.deviceContext->OMGetDepthStencilState(&oldDepthState, &oldStencilRef);
	context.deviceContext->OMSetDepthStencilState(m_trailDepthState.Get(), 0);

	// ! draw
	context.deviceContext->Draw(
		static_cast<UINT>(m_vertices.size()),
		0
	);

	// ! restore & clean up states to prevent leaking into later renderers
	// restore rasterizer state 
	context.deviceContext->RSSetState(oldRasterizerState);
	// Release the reference
	if (oldRasterizerState)
		oldRasterizerState->Release();

	// restore blend state
	context.deviceContext->OMSetBlendState(oldBlendState, blendFactor, sampleMask);
	// Release the reference
	if (oldBlendState)
		oldBlendState->Release();

	// restore blend state
	context.deviceContext->OMSetDepthStencilState(oldDepthState, oldStencilRef);
	// Release the reference
	if (oldDepthState)
		oldDepthState->Release();
}

void TrailRenderer::CreateEffectAndInputLayout(const RenderContext& context) {
	using namespace ErrorHandler;

	// ! create trail effect
	m_trailEffect = std::make_unique<TrailEffect>();

	m_trailEffect->Initialize(
		context.device,
		"Shaders\\TrailVS.cso",
		"Shaders\\TrailPS.cso"
	);

	// ! create input layout
	D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] = {
		{
			"POSITION",						// semantic name
			0,								// semantic index (if multiple fields with same semantic name)
			DXGI_FORMAT_R32G32B32_FLOAT,	// format
			0,								// input slot
			0,								// byte offset
			D3D11_INPUT_PER_VERTEX_DATA,	// input slot class
			0								// instance data step rate
		},
		{
			"COLOR",
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			0,
			12,
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		},
	};

	void const* vsBytecode;
	std::size_t vsSize;
	m_trailEffect->GetVertexShaderBytecode(&vsBytecode, &vsSize);

	ThrowIfFailed(
		context.device->CreateInputLayout(
			inputLayoutDesc,
			_countof(inputLayoutDesc),
			vsBytecode,
			vsSize,
			&m_inputLayout
		),
		"Unable to create input layout for trail vertex buffer"
	);

	// ! create rasterizer state to set cull mode to NONE
	D3D11_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.DepthClipEnable = TRUE;

	ThrowIfFailed(
		context.device->CreateRasterizerState(
			&rasterizerDesc,
			&m_trailRasterizerState
		),
		"Unable to create trail rasterizer state"
	);

	// ! create blend state to blend trail with the render-target color (without it alpha alone can't do anything)
	D3D11_BLEND_DESC blendDesc{};
	// RGB
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	// ALPHA
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	ThrowIfFailed(
		context.device->CreateBlendState(
			&blendDesc,
			&m_trailBlendState
		),
		"Unable to create trail blend state"
	);

	// ! create depth state, otherwise transparent trail part will still block the object behind it
	D3D11_DEPTH_STENCIL_DESC depthDesc{};
	depthDesc.DepthEnable = TRUE;							// trail can still correctly disappear behind walls
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;	// trail doesn't block objects rendered afterward (behind it)
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS;			// incoming depth < store depth
	depthDesc.StencilEnable = FALSE;
	depthDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depthDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	ThrowIfFailed(
		context.device->CreateDepthStencilState(
			&depthDesc,
			&m_trailDepthState
		),
		"Unable to create trail depth stencil state"
	);
}

void TrailRenderer::EnsureVertexBufferCapacity(const RenderContext& context) {
	m_vertexCapacity = std::max(m_vertices.size(), m_vertexCapacity * 2);

	D3D11_BUFFER_DESC vertexBufferDesc{};
	vertexBufferDesc.ByteWidth = sizeof(TrailVertex) * m_vertexCapacity;
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;				// dynamic buffer
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;	// cpu will write it frequently

	ErrorHandler::ThrowIfFailed(
		context.device->CreateBuffer(
			&vertexBufferDesc,
			nullptr,
			&m_vertexBuffer
		),
		"Unable to create dynamic vertex buffer"
	);
}