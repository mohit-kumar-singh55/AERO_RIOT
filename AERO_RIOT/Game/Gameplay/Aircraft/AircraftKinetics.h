#pragma once

#include "AircraftControlInput.h"

#include <SNX/Core/Object/Component.h>

class KineticBody;

// physics component for aircraft
class AircraftKinetics : public Component {
public:
	using Component::Component;

	// apply the physics
	void Apply(const AircraftControlInput&) noexcept;

protected:
	void OnInitialize() override;

private:
	[[nodiscard]]
	float CalculateLiftCoefficient(const float angleOfAttack) const noexcept;

	[[nodiscard]]
	float CalculateBankAngle() const noexcept;

private:
	KineticBody* m_kb = nullptr;

	// configurable
	float m_maxThrust = 100.0f;
	// directional aerodynamic drag coefficient
	float m_forwardDrag = 1.0f;
	float m_sideDrag = 3.0f;
	float m_verticalDrag = 2.0f;
	float m_airBrakePower = 4.0f;	// additional drag

	float m_airDensity = 1.225f;
	float m_wingArea = 2.0f;
	float m_liftSlope = 4.0f;	// per radian
	float m_stallAngle = DirectX::XMConvertToRadians(15.0f);

	// control torque
	float m_pitchTorque = 20.0f;
	float m_yawTorque = 10.0f;
	float m_rollTorque = 20.0f;

	// angular damping
	float m_pitchDamping = 1.0f;
	float m_yawDamping = 1.0f;
	float m_rollDamping = 1.0f;

	float m_maxBankAngle = DirectX::XMConvertToRadians(50.0f);
	float m_bankKp = 1.5f;		// controls how strongly it tries to reach the target angle
	float m_bankKd = 2.0f;		// controls braking based on roll speed
};