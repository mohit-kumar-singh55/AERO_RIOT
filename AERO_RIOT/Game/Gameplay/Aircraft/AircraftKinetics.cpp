#include "pch.h"

#include "AircraftKinetics.h"

#include <SNX/Core/Object/GameObject.h>
#include <SNX/Core/Components/Kinetics/KineticBody.h>

#include <stdexcept>

void AircraftKinetics::OnStart() {
	m_kb = GetGameObject().GetComponent<KineticBody>();

	if (!m_kb)
		throw std::runtime_error("AircraftKinetics::OnStart: Cannot find KineticBody component.");
}

void AircraftKinetics::Apply(AircraftControlInput& controlInput) noexcept {
	const auto& transform = GetTransform();
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

	// ? temp: apply airbrake
	totalDrag *= 1.0f + controlInput.airBrake * m_airBrakePower;

	float angleOfAttack = -std::atan2(verticalSpeed, forwardSpeed);
	angleOfAttack = DirectX::XMConvertToDegrees(angleOfAttack);

	m_kb->AddForce(totalDrag);
}