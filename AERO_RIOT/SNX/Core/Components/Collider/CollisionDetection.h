#pragma once

#include "Collider.h"
#include "SphereCollider.h"

namespace CollisionDetection {
	inline bool Intersects(const SphereCollider& a, const SphereCollider& b) {
		const float radiusSum = a.GetRadius() + b.GetRadius();

		const float distanceSquared = DirectX::SimpleMath::Vector3::DistanceSquared(
			a.GetCenter(), b.GetCenter()
		);

		return distanceSquared <= radiusSum * radiusSum;
	}

	inline bool Intersects(const Collider& a, const Collider& b) {
		switch (a.GetShape()) {
		case ColliderShape::Sphere: {
			if (b.GetShape() == ColliderShape::Sphere)
				return Intersects(dynamic_cast<const SphereCollider&>(a), dynamic_cast<const SphereCollider&>(b));
		}
		}

		return false;
	}
}