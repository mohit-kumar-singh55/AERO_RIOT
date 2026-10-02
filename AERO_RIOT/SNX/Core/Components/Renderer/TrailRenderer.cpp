#include "pch.h"
#include "TrailRenderer.h"

#include <SNX/Graphics/RenderContext.h>
#include <SNX/Core/Object/GameObject.h>
#include <SNX/Core/Components/Transform.h>
#include <SNX/Core/Time.h>
#include <SNX/Utils/ErrorHandler.h>

#include <algorithm>

TrailRenderer::TrailRenderer(GameObject& gameObject) noexcept
	: Renderer(gameObject) {}

void TrailRenderer::OnUpdate() {
	using DirectX::SimpleMath::Vector3;

	// increase age & remove expired points
	for (auto i = m_points.begin();i != m_points.end();) {
		i->age += Time::DeltaTime();

		if (i->age >= m_lifeTime)
			i = m_points.erase(i);
		else
			i++;
	}

	// add new point
	auto currentPos = GetTransform().GetPosition();

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

	// clear previous frame vertices
	m_vertices.clear();

	// atleast 2 points are required to generate ribbon
	if (m_points.size() < 2)
		return;

	std::size_t lastIndex = m_points.size() - 1;
	for (std::size_t i = 0; i < m_points.size(); i++) {
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
		Vector3 sideDirOfPoint = trailDir.Cross(dirToCamera);
		if (sideDirOfPoint.LengthSquared() > 0.0001f)
			sideDirOfPoint.Normalize();
		else
			sideDirOfPoint = GetTransform().GetRight();

		// vertex position on left & right sides of the trail point
		Vector3 left = m_points[i].position - sideDirOfPoint * m_halfWidth;
		Vector3 right = m_points[i].position + sideDirOfPoint * m_halfWidth;

		float alpha = 1 - (m_points[i].age / m_lifeTime);

		m_vertices.push_back({ left, alpha });
		m_vertices.push_back({ right, alpha });
	}

	if (m_vertices.empty())
		return;

	if (m_vertices.size() > m_vertexCapacity)
		EnsureVertexBufferCapacity(context);
}

void TrailRenderer::EnsureVertexBufferCapacity(const RenderContext& context) {
	using namespace ErrorHandler;

	m_vertexCapacity = std::max(m_vertices.size(), m_vertexCapacity * 2);

	D3D11_BUFFER_DESC vertexBufferDesc{};
	vertexBufferDesc.ByteWidth = sizeof(TrailVertex) * m_vertexCapacity;
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;				// dynamic buffer
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;	// cpu will write it frequently

	ThrowIfFailed(
		context.device->CreateBuffer(
			&vertexBufferDesc,
			nullptr,
			&m_vertexBuffer
		),
		"Unable to create dynamic vertex buffer"
	);
}