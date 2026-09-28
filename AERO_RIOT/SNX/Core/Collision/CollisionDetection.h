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
		* for the Sphere vs OBB - Discrete Collision
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

	inline bool IntersectsContinuous(const SphereCollider& a, const BoxCollider& b) {
		/*
		* for the Sphere vs OBB - Continuous Collision
		* we use the same concept as with Continuous Sphere vs Sphere
		* but this time we expand the box instead of sphere.
		* then convert the sphere into the box's orientation space.
		*/

		const Vector3 boxExpandedExtents = b.GetWorldExtents() + Vector3::One * a.GetWorldRadius();

		// relative motion
		const Vector3 relativeStart = a.GetPreviousCenter() - b.GetPreviousCenter();
		const Vector3 relativeEnd = a.GetCenter() - b.GetCenter();

		const Vector3 direction = relativeEnd - relativeStart;

		// undo the box rotation
		Quaternion boxInverseRotation;
		b.GetRotation().Inverse(boxInverseRotation);

		const Vector3 localSphereCenterStart = Vector3::Transform(relativeStart, boxInverseRotation);
		const Vector3 localSphereCenterEnd = Vector3::Transform(relativeEnd, boxInverseRotation);

		/*
		* segment vs expanded AABB
		* using SLAB Method.
		* line segment is:
		* P(t) = start + direction * t
		*/

		float tEnter = 0.0f;
		float tExit = 1.0f;

		auto t = [](float extent, float start, float direction) {
			return (extent - start) / direction;
			};

		auto checkifInside = [](float x, float a, float b) {
			return (x >= a && x <= b);
			};

		auto swap = [](float& a, float& b) {
			float temp = a;
			a = b;
			b = temp;
			};

		/*
		* At what t does the point enter the box, 
		* and at what t does it leave?
		*/
		auto checkForInterval = [&](float extent, float start, float direction) {
			// entering the axis interval
			float tNear = t(-extent, start, direction);
			// leaving the axis interval
			float tFar = t(extent, start, direction);

			if (tNear > tFar)
				swap(tNear, tFar);

			tEnter = std::max(tEnter, tNear);
			tExit = std::min(tExit, tFar);
			};

		// for x axis bounds/interval
		if (std::abs(direction.x) < 0.0001f) {
			if (!checkifInside(relativeStart.x, -boxExpandedExtents.x, boxExpandedExtents.x))
				return false;
			// else skip X
		}
		else
			checkForInterval(boxExpandedExtents.x, relativeStart.x, direction.x);

		// for y axis bounds/interval
		if (std::abs(direction.y) < 0.0001f) {
			if (!checkifInside(relativeStart.y, -boxExpandedExtents.y, boxExpandedExtents.y))
				return false;
			// else skip Y
		}
		else
			checkForInterval(boxExpandedExtents.y, relativeStart.y, direction.y);

		// for z axis bounds/interval
		if (std::abs(direction.z) < 0.0001f) {
			if (!checkifInside(relativeStart.z, -boxExpandedExtents.z, boxExpandedExtents.z))
				return false;
			// else skip Y
		}
		else
			checkForInterval(boxExpandedExtents.z, relativeStart.z, direction.z);

		return tEnter <= tExit;
	}

	inline bool CheckBothDiscrete(const Collider& a, const Collider& b) {
		return a.GetDetectionMode() == CollisionDetectionMode::Discrete
			&& b.GetDetectionMode() == CollisionDetectionMode::Discrete;
	}

	inline bool Intersects(const Collider& a, const Collider& b) {
		switch (a.GetShape()) {
		case ColliderShape::Sphere: {
			if (b.GetShape() == ColliderShape::Sphere) {
				if (CheckBothDiscrete(a, b))
					return Intersects(dynamic_cast<const SphereCollider&>(a), dynamic_cast<const SphereCollider&>(b));
				else
					return IntersectsContinuous(dynamic_cast<const SphereCollider&>(a), dynamic_cast<const SphereCollider&>(b));
			}
			else if (b.GetShape() == ColliderShape::Box) {
				if (CheckBothDiscrete(a, b))
					return Intersects(dynamic_cast<const SphereCollider&>(a), dynamic_cast<const BoxCollider&>(b));
				else
					return IntersectsContinuous(dynamic_cast<const SphereCollider&>(b), dynamic_cast<const BoxCollider&>(a));
			}
		}
		case ColliderShape::Box: {
			if (b.GetShape() == ColliderShape::Sphere) {
				if (CheckBothDiscrete(a, b))
					return Intersects(dynamic_cast<const SphereCollider&>(b), dynamic_cast<const BoxCollider&>(a));
				else
					return IntersectsContinuous(dynamic_cast<const SphereCollider&>(b), dynamic_cast<const BoxCollider&>(a));
			}
		}
		}

		return false;
	}
}