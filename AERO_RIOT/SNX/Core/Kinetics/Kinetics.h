#pragma once

#include <vector>
#include <SimpleMath.h>

class KineticBody;
class Collider;

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

	DirectX::SimpleMath::Vector3 m_gravity{ 0.0f, -9.81f, 0.0f };
};