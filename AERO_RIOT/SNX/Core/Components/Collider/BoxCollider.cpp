#include "pch.h"
#include "BoxCollider.h"

BoxCollider::BoxCollider(
	GameObject& gameObject,
	DirectX::SimpleMath::Vector3 extents
) noexcept :
	Collider(gameObject) {
	SetExtents(extents);
}