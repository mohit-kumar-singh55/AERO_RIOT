#include "pch.h"
#include "BoxCollider.h"

BoxCollider::BoxCollider(
	GameObject& gameObject,
	DirectX::SimpleMath::Vector3 extents
) noexcept :
	Collider(gameObject) {
	m_shape = ColliderShape::Box;
	SetExtents(extents);
}