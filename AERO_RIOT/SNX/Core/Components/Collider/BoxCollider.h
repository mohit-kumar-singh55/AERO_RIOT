#pragma once

#include "Collider.h"

#include <SimpleMath.h>

class BoxCollider final : public Collider {
public:
	BoxCollider(
		GameObject& gameObject,
		DirectX::SimpleMath::Vector3 extents = { 0.5f, 0.5f, 0.5f }
	) noexcept;

	[[nodiscard]]
	DirectX::SimpleMath::Vector3 GetExtents() const noexcept { return m_extents; }

	void SetExtents(DirectX::SimpleMath::Vector3 extents) noexcept {
		m_extents = DirectX::SimpleMath::Vector3::Max(
			extents,
			DirectX::SimpleMath::Vector3::Zero
		);
	}

private:
	// half size
	DirectX::SimpleMath::Vector3 m_extents = { 0.5f, 0.5f, 0.5f };
};