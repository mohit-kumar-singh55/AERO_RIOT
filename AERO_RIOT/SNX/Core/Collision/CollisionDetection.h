#pragma once

#include <algorithm>

#include <SNX/Core/Components/Collider/Collider.h>
#include <SNX/Core/Components/Collider/SphereCollider.h>
#include <SNX/Core/Components/Collider/BoxCollider.h>

namespace CollisionDetection {
	using DirectX::SimpleMath::Vector3;
	using DirectX::SimpleMath::Quaternion;

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

	inline bool Intersects(const SphereCollider& a, const BoxCollider& b) {
		/*
		* for the Sphere vs OBB
		* we convert the sphere into the box's orientation space.
		* rather than using some complicated math based on the OBB's world orientation
		*/
		// sphere position relative to the box
		const Vector3 sphereRelativePos = a.GetCenter() - b.GetCenter();

		// undo the box rotation
		Quaternion boxInverseRotation;
		b.GetRotation().Inverse(boxInverseRotation);

		const Vector3 localSphereCenter = Vector3::Transform(sphereRelativePos, boxInverseRotation);

		/*
		* now the box is centered at the origin with bounds:
		* x = [-extent.x, +extent.x]
		* y = [-extent.y, +extent.y]
		* z = [-extent.z, +extent.z]
		*/

		// find the point on that box closest to the sphere center
		const auto extents = b.GetWorldExtents();
		const Vector3 closest{
			std::clamp(localSphereCenter.x,-extents.x,extents.x),
			std::clamp(localSphereCenter.y,-extents.y,extents.y),
			std::clamp(localSphereCenter.z,-extents.z,extents.z)
		};

		const auto distanceSquared = Vector3::DistanceSquared(
			localSphereCenter, closest
		);

		return distanceSquared <= a.GetWorldRadius() * a.GetWorldRadius();
	}

	inline bool Intersects(const Collider& a, const Collider& b) {
		switch (a.GetShape()) {
		case ColliderShape::Sphere: {
			if (b.GetShape() == ColliderShape::Sphere) {
				if (a.GetDetectionMode() == CollisionDetectionMode::Discrete
					&& b.GetDetectionMode() == CollisionDetectionMode::Discrete)
					return Intersects(dynamic_cast<const SphereCollider&>(a), dynamic_cast<const SphereCollider&>(b));
				else
					return IntersectsContinuous(dynamic_cast<const SphereCollider&>(a), dynamic_cast<const SphereCollider&>(b));
			}

			if (b.GetShape() == ColliderShape::Box) {

				if (a.GetDetectionMode() == CollisionDetectionMode::Discrete
					&& b.GetDetectionMode() == CollisionDetectionMode::Discrete)
					return Intersects(dynamic_cast<const SphereCollider&>(a), dynamic_cast<const BoxCollider&>(b));
				//else
				//	return IntersectsContinuous(dynamic_cast<const SphereCollider&>(a), dynamic_cast<const SphereCollider&>(b));
			}
		}
		case ColliderShape::Box: {
			if (b.GetShape() == ColliderShape::Sphere) {

				if (a.GetDetectionMode() == CollisionDetectionMode::Discrete
					&& b.GetDetectionMode() == CollisionDetectionMode::Discrete)
					return Intersects(dynamic_cast<const SphereCollider&>(b), dynamic_cast<const BoxCollider&>(a));
				//else
				//	return IntersectsContinuous(dynamic_cast<const SphereCollider&>(a), dynamic_cast<const SphereCollider&>(b));
			}
		}
		}

		return false;
	}
}