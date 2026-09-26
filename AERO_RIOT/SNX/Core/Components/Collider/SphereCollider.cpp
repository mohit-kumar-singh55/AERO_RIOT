#include "pch.h"
#include "SphereCollider.h"

SphereCollider::SphereCollider(
	GameObject& gameObject,
	float radius
) noexcept :
	Collider(gameObject) {
	SetRadius(radius);
}