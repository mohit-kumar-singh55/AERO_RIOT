#pragma once

#include <vector>
#include <SimpleMath.h>

class KineticBody;

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

	// apply physics for this step to all the bodies
	void Integrate(float fixedDeltaTime) noexcept;

private:
	std::vector<KineticBody*> m_kineticBodies;

	DirectX::SimpleMath::Vector3 m_gravity{ 0.0f, -9.81f, 0.0f };
};