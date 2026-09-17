#pragma once

#include <SNX/Core/Object/Component.h>

#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

enum class ForceMode { Force, Explosive, VelocityChange };

class KineticBody : public Component {
public:
	using Component::Component;

	[[nodiscard]]
	bool GetUseGravity() const noexcept { return m_useGravity; }

	void SetUseGravity(const bool useGravity) noexcept { m_useGravity = useGravity; }

	[[nodiscard]]
	bool GetUseLinearDamping() const noexcept { return m_useLinearDamping; }

	void SetUseLinearDamping(const bool useLinearDamping) noexcept { m_useLinearDamping = useLinearDamping; }

	[[nodiscard]]
	float GetLinearDamping() const noexcept { return m_linearDamping; }

	void SetLinearDamping(float linearDamping) noexcept {
		m_linearDamping = std::abs(linearDamping);	// negative not allowed
	}

	[[nodiscard]]
	float GetMass() const noexcept { return m_mass; }

	void SetMass(const float mass) noexcept {
		m_mass = mass < MIN_MASS ? MIN_MASS : mass;
		m_inverseMass = 1.0f / m_mass;
	}

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
	bool GetUseAngularDamping() const noexcept { return m_useAngularDamping; }

	void SetUseAngularDamping(const bool useAngularDamping) noexcept { m_useAngularDamping = useAngularDamping; }

	[[nodiscard]]
	Vector3 GetMomentOfInertia() const noexcept { return m_momentOfInertia; }

	[[nodiscard]]
	Vector3 GetAngularDamping() const noexcept { return m_angularDamping; }

	void SetAngularDamping(const Vector3 angularDamping) noexcept { m_angularDamping = angularDamping; }

	void SetMomentOfInertia(const Vector3 moi) noexcept {
		m_momentOfInertia.x = moi.x < MIN_MOI ? MIN_MOI : moi.x;
		m_momentOfInertia.y = moi.y < MIN_MOI ? MIN_MOI : moi.y;
		m_momentOfInertia.z = moi.z < MIN_MOI ? MIN_MOI : moi.z;
		m_inverseMomentOfInertia = 1.0f / m_momentOfInertia;
	}

	[[nodiscard]]
	Vector3 GetAngularVelocity() const noexcept {
		return m_angularVelocity;
	}

	void SetAngularVelocity(Vector3 velocity) noexcept {
		m_angularVelocity = velocity;
	}

	void AddTorque(Vector3 torque) {
		m_accumulatedTorque += torque;
	}

protected:
	void OnInitialize() override;
	void OnDestroy() override;

	void Integrate(float fixedDeltaTime) noexcept;

private:
	bool m_useGravity = true;
	float m_gravityScale = 1.0f;	// gravity multiplier

	bool m_useLinearDamping = true;
	float m_linearDamping = 1.0f;	// damping coefficient

	float m_mass = 1.0f;
	float m_inverseMass = 1.0f;

	Vector3 m_linearVelocity = Vector3::Zero;
	Vector3 m_linearAcceleration = Vector3::Zero;
	Vector3 m_accumulatedForce = Vector3::Zero;

	// local space (body-space)
	bool m_useAngularDamping = true;
	Vector3 m_angularDamping = Vector3::One;	// damping coefficients

	// local space (body-space)
	Vector3 m_momentOfInertia = Vector3::One;
	Vector3 m_inverseMomentOfInertia = Vector3::One;

	// world-space
	Vector3 m_angularVelocity = Vector3::Zero;	// radians/sec on respective axis
	Vector3 m_angularAcceleration = Vector3::Zero;
	Vector3 m_accumulatedTorque = Vector3::Zero;

	// minimum mass value
	static constexpr float MIN_MASS = 0.0001f;
	static constexpr float MIN_MOI = 0.0001f;	// moment of inertia

	// it will call the Integrate method to apply physics
	friend class Kinetics;
};