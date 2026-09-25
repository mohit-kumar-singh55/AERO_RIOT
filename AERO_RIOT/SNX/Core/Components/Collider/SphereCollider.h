#pragma once

#include "Collider.h"

#include <algorithm>

class SphereCollider final : public Collider {
public:
	SphereCollider(
		GameObject& gameObject,
		float radius = 1.0f
	) noexcept;

	[[nodiscard]]
	float GetRadius() const noexcept { return m_radius; }

	void SetRadius(float radius) noexcept {
		m_radius = std::max(radius, 0.0f);
	}

private:
	float m_radius = 1.0f;
};