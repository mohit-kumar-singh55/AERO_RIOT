#pragma once

#include "Collider.h"

#include <algorithm>

class SphereCollider final : public Collider {
public:
	SphereCollider(
		GameObject& gameObject,
		float radius = 0.5f
	) noexcept;

	// local radius
	[[nodiscard]]
	float GetRadius() const noexcept { return m_radius; }

	void SetRadius(float localRadius) noexcept {
		m_radius = std::max(localRadius, 0.0f);
	}

	// world radius (scales with the scale of the object)
	[[nodiscard]]
	float GetWorldRadius() const noexcept {
		auto scale = GetTransform().GetScale();
		float max = std::max(scale.x, scale.y);
		return m_radius * std::max(max, scale.z);
	}

	ColliderShape GetShape() const noexcept override { return ColliderShape::Sphere; }

private:
	float m_radius = 0.5f;	// local radius
};