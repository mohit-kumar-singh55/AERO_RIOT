#pragma once

#include <SNX/Core/Components/Collider/Collider.h>
#include <SNX/Core/Components/Collider/SphereCollider.h>

namespace CollisionDetection {
	using DirectX::SimpleMath::Vector3;

	inline bool Intersects(const SphereCollider& a, const SphereCollider& b) {
		const float radiusSum = a.GetWorldRadius() + b.GetWorldRadius();

		const float distanceSquared = Vector3::DistanceSquared(
			a.GetCenter(), b.GetCenter()
		);

		return distanceSquared <= radiusSum * radiusSum;
	}

	inline bool IntersectsContinuous(const SphereCollider& a, const SphereCollider& b) {
		const Vector3 relativeStart = a.GetPreviousCenter() - b.GetPreviousCenter();
		const Vector3 relativeEnd = a.GetCenter() - b.GetCenter();

		const Vector3 motion = relativeEnd - relativeStart;

		// do a normal overlap check if almost no motion
		if (motion.LengthSquared() < 0.0001f)
			return Intersects(a, b);

		/*
		* minus sign is because we are minimizing
		* |relativeStart + motion * t|²
		* and the minimum occurs where the direction toward the origin
		* is perpendicular to the motion
		*/
		float t = -relativeStart.Dot(motion) / motion.Dot(motion);
		t = std::clamp(t, 0.0f, 1.0f);

		// closest point on the segment to the origin
		const Vector3 closest = relativeStart + motion * t;

		const float radiusSum = a.GetWorldRadius() + b.GetWorldRadius();

		return closest.LengthSquared() <= radiusSum * radiusSum;
	}

	inline bool Intersects(const Collider& a, const Collider& b) {
		switch (a.GetShape()) {
		case ColliderShape::Sphere: {
			if (b.GetShape() == ColliderShape::Sphere)
				if (a.GetDetectionMode() == CollisionDetectionMode::Discrete
					&& b.GetDetectionMode() == CollisionDetectionMode::Discrete)
					return Intersects(dynamic_cast<const SphereCollider&>(a), dynamic_cast<const SphereCollider&>(b));
				else
					return IntersectsContinuous(dynamic_cast<const SphereCollider&>(a), dynamic_cast<const SphereCollider&>(b));
		}
		}

		return false;
	}
}