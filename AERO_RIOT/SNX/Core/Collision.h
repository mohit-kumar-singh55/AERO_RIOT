#pragma once

#include <SimpleMath.h>

// 2d rect collision
struct RectF {
	float x;
	float y;
	float width;
	float height;
};

inline bool Intersects(const RectF& a, const RectF& b) noexcept {
	return
		a.x < b.x + b.width &&
		a.x + a.width > b.x &&
		a.y < b.y + b.height &&
		a.y + a.height > b.y;
}

// 3d sphere collision
struct SphereBounds {
	DirectX::SimpleMath::Vector3 center;
	float radius;
};

inline bool Intersects(const SphereBounds& a, const SphereBounds b) {
	const float radiusSum = a.radius + b.radius;

	const float distanceSquared = DirectX::SimpleMath::Vector3::DistanceSquared(
		a.center, b.center
	);

	return distanceSquared <= radiusSum * radiusSum;
}