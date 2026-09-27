#pragma once

#include <vector>
#include <SimpleMath.h>
#include <unordered_set>

class KineticBody;
class Collider;

struct CollisionPair {
	CollisionPair(Collider* first, Collider* second) {
		// if second < first
		if (std::less<Collider*>{}(second, first))
			std::swap(first, second);

		a = first;
		b = second;
	}

	bool operator==(const CollisionPair& other) const noexcept {
		return a == other.a && b == other.b;
	}

	Collider* a;	// smaller
	Collider* b;	// higher
};

struct CollisionPairHash {
	std::size_t operator()(const CollisionPair& pair) const noexcept {
		// calculate hash
		auto hashA = std::hash<Collider*>{}(pair.a);
		auto hashB = std::hash<Collider*>{}(pair.b);

		return hashA ^ (hashB << 1);
	}
};

/// <summary>
/// Responsible to:
/// - apply physics
/// - collision detection
/// - etc
/// </summary>
class Kinetics final {
public:
	explicit Kinetics() = default;
	~Kinetics();

	Kinetics(const Kinetics&) = delete;
	Kinetics& operator=(const Kinetics&) = delete;
	Kinetics(Kinetics&&) = delete;
	Kinetics& operator=(Kinetics&&) = delete;

	void RegisterKineticBody(KineticBody* body);
	void UnregisterKineticBody(KineticBody* body);

	void RegisterCollider(Collider* collider);
	void UnregisterCollider(Collider* collider);

	// apply physics for this step to all the bodies
	void Integrate(float fixedDeltaTime) noexcept;
	void UpdateInterpolation(float alpha) noexcept;

	// check collision
	void DetectCollision() noexcept;

private:
	std::vector<KineticBody*> m_kineticBodies;
	std::vector<Collider*> m_colliders;

	std::unordered_set<CollisionPair, CollisionPairHash> m_previousCollisions;
	std::unordered_set<CollisionPair, CollisionPairHash> m_currentCollisions;

	DirectX::SimpleMath::Vector3 m_gravity{ 0.0f, -9.81f, 0.0f };
};