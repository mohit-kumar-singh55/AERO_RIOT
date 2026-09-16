#pragma once

#include <SNX/Core/Object/Component.h>

#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

enum class ForceMode { Force, Explosive, VelocityChange };

class KineticBody : public Component {
public:
	using Component::Component;

	[[nodiscard]]
	float GetMass() const noexcept { return m_mass; }

	void SetMass(const float mass) noexcept {
		m_mass = mass < MIN_MASS ? MIN_MASS : mass;
		m_inverseMass = 1.0f / m_mass;
	}

	[[nodiscard]]
	bool GetUseGravity() const noexcept { return m_useGravity; }

	void SetUseGravity(const bool useGravity) noexcept { m_useGravity = useGravity; }

	[[nodiscard]]
	float GetGravityScale() const noexcept { return m_gravityScale; }

	void SetGravityScale(const float gravityScale) noexcept { m_gravityScale = gravityScale; }

	[[nodiscard]]
	Vector3 GetLinearVelocity() const noexcept {
		return m_linearVelocity;
	}

	void SetLinearVelocity(Vector3 velocity) noexcept {
		m_linearVelocity = velocity;
	}

	void AddForce(Vector3 force, ForceMode mode = ForceMode::Force) {
		m_accumulatedForce += force;
	}

	//void AddForceAtPoint(Vector3 point, ForceMode mode = ForceMode::Explosive);

	[[nodiscard]]
	Vector3 GetAngularVelocity() const noexcept {
		return m_angularVelocity;
	}

	void SetAngularVelocity(Vector3 velocity) noexcept {
		m_angularVelocity = velocity;
	}

protected:
	void OnInitialize() override;
	void OnDestroy() override;

	void Integrate(float fixedDeltaTime) noexcept;

private:
	float m_mass = 1.0f;
	float m_inverseMass = 1.0f;

	bool m_useGravity = true;
	float m_gravityScale = 1.0f;	// gravity multiplier

	Vector3 m_linearVelocity = Vector3::Zero;
	Vector3 m_linearAcceleration = Vector3::Zero;
	Vector3 m_accumulatedForce = Vector3::Zero;

	Vector3 m_angularVelocity = Vector3::Zero;	// radians/sec on respective axis

	// minimum mass value
	static constexpr float MIN_MASS = 0.0001f;

	// it will call the Integrate method to apply physics
	friend class Kinetics;
};