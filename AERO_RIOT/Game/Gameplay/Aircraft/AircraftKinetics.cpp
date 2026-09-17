#include "pch.h"

#include "AircraftKinetics.h"

#include <SNX/Core/Object/GameObject.h>
#include <SNX/Core/Components/Kinetics/KineticBody.h>

#include <stdexcept>

void AircraftKinetics::OnInitialize() {
	m_kb = GetGameObject().GetComponent<KineticBody>();

	if (!m_kb)
		throw std::runtime_error("AircraftKinetics::OnInitialize: Cannot find KineticBody component.");

	// not using the default damping provided by the physics engine
	m_kb->SetUseLinearDamping(false);
	m_kb->SetUseAngularDamping(false);
}

void AircraftKinetics::Apply(const AircraftControlInput& controlInput) noexcept {
	const auto& transform = GetTransform();

	// ! apply rotation
	const Vector3 localTorque = {
		controlInput.pitch * m_pitchTorque,
		0.0f,
		0.0f
	};
	// convert to world-space
	const Vector3 worldTorque = Vector3::Transform(localTorque, transform.GetRotation());
	m_kb->AddTorque(worldTorque);

	// ! apply thrust
	const Vector3 thrust =
		transform.GetForward()
		* m_maxThrust
		* controlInput.throttle;

	m_kb->AddForce(thrust);

	// ! calc. directional aerodynamic drag
	const Vector3 velocity = m_kb->GetLinearVelocity();

	const Vector3 aircraftForward = transform.GetForward();
	const Vector3 aircraftRight = transform.GetRight();
	const Vector3 aircraftUp = transform.GetUp();

	const float forwardSpeed = velocity.Dot(aircraftForward);
	const float sideSpeed = velocity.Dot(aircraftRight);
	const float verticalSpeed = velocity.Dot(aircraftUp);

	const Vector3 forwardDrag =
		-aircraftForward
		* m_forwardDrag
		* forwardSpeed
		* std::abs(forwardSpeed);

	const Vector3 sideDrag =
		-aircraftRight
		* m_sideDrag
		* sideSpeed
		* std::abs(sideSpeed);

	const Vector3 verticalDrag =
		-aircraftUp
		* m_verticalDrag
		* verticalSpeed
		* std::abs(verticalSpeed);

	Vector3 totalDrag = forwardDrag + sideDrag + verticalDrag;

	// ? TEMP
	// TODO: calc. proper airbrake drag
	totalDrag *= 1.0f + controlInput.airBrake * m_airBrakePower;

	m_kb->AddForce(totalDrag);

	// ! calc. lift force
	float liftForce = 0.0f;
	Vector3 liftDir = Vector3::Zero;
	float pitchSpeedSquared = forwardSpeed * forwardSpeed + verticalSpeed * verticalSpeed;

	if (pitchSpeedSquared <= 0.01f * 0.01)
		liftForce = 0.0f;
	else {
		float angleOfAttack = -std::atan2(verticalSpeed, forwardSpeed);
		float liftCoef = CalculateLiftCoefficient(angleOfAttack);

		liftForce =
			0.5f
			* m_airDensity
			* pitchSpeedSquared
			* m_wingArea
			* liftCoef;

		// calc. lift dir
		const Vector3 pitchVelocity = aircraftForward * forwardSpeed + aircraftUp * verticalSpeed;
		liftDir = aircraftRight.Cross(pitchVelocity);
		liftDir.Normalize();
	}

	m_kb->AddForce(liftDir * liftForce);
}

float AircraftKinetics::CalculateLiftCoefficient(const float angleOfAttack) const noexcept {
	const float absAoA = std::abs(angleOfAttack);

	const float maxAngle = DirectX::g_XMHalfPi.f[0];	// 90.0f

	if (absAoA <= m_stallAngle)
		return m_liftSlope * angleOfAttack;
	else if (absAoA > m_stallAngle && absAoA < maxAngle) {
		float maxLiftCoef = m_liftSlope * m_stallAngle;
		float postStallT = (absAoA - m_stallAngle) / (maxAngle - m_stallAngle);
		float liftRemaining = 1 - postStallT;
		return (angleOfAttack >= 0.0f ? 1.0f : -1.0f) * maxLiftCoef * liftRemaining;
	}

	// above 90.0f
	return 0.0f;
}