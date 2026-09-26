#pragma once

#include "Collider.h"

#include <SimpleMath.h>

class BoxCollider final : public Collider {
public:
	BoxCollider(
		GameObject& gameObject,
		// local-spcae
		DirectX::SimpleMath::Vector3 extents = { 0.5f, 0.5f, 0.5f }
	) noexcept;

	// local-space extents
	[[nodiscard]]
	DirectX::SimpleMath::Vector3 GetExtents() const noexcept {
		return m_extents;
	}

	// local-space entents
	void SetExtents(DirectX::SimpleMath::Vector3 extents) noexcept {
		m_extents = DirectX::SimpleMath::Vector3::Max(
			extents,
			DirectX::SimpleMath::Vector3::Zero
		);
	}

	// world-space extents
	[[nodiscard]]
	DirectX::SimpleMath::Vector3 GetWorldExtents() const noexcept {
		const auto scale = GetTransform().GetScale();

		return {
			m_extents.x * std::abs(scale.x),
			m_extents.y * std::abs(scale.y),
			m_extents.z * std::abs(scale.z)
		};
	}

	ColliderShape GetShape() const noexcept override { return ColliderShape::Box; }

private:
	// half size, in local-space
	DirectX::SimpleMath::Vector3 m_extents = { 0.5f, 0.5f, 0.5f };
};