#include "pch.h"
#include "TrailRenderer.h"

#include <SNX/Graphics/RenderContext.h>
#include <SNX/Core/Object/GameObject.h>
#include <SNX/Core/Components/Transform.h>
#include <SNX/Core/Time.h>

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

}