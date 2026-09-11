#include "Aircraft.h"

#include <SNX/Core/Components/Transform.h>
#include <SNX/Core/Object/GameObject.h>
#include <SNX/Core/Components/Kinetics/KineticBody.h>
#include <SNX/Core/Time.h>

void Aircraft::OnInitialize() {
	// ! considering the body gameobject is at index 0
	// TODO: change it to explicit
	m_aircraftBody = GetTransform().GetChild(0);

	if (!m_aircraftBody)
		throw std::runtime_error("Aircraft::OnInitialize: Cannot find Aircraft Body Gameobject at child index 0.");
}

void Aircraft::OnStart() {
	m_kb = GetGameObject().GetComponent<KineticBody>();

	if (!m_kb)
		throw std::runtime_error("Aircraft::OnInitialize: Cannot find KineticBody component.");
}

void Aircraft::OnFixedUpdate() {
	using DirectX::SimpleMath::Vector3;

	// ? temp direct rotation
	auto& transform = GetTransform();

	// rotation
	float speedDelta = m_rotationSpeed * Time::DeltaTime();
	transform.RotateEulerDegrees({
		-m_controlInput.pitch * speedDelta,
		-m_controlInput.turn * speedDelta,
		-m_controlInput.turn * speedDelta
		});
	// ? *********************

	// ! apply thrust
	const Vector3 thrust =
		GetTransform().GetForward()
		* m_maxThrust
		* m_controlInput.throttle;

	m_kb->AddForce(thrust);

	// ! apply aerodynamic drag
	const Vector3 velocity = m_kb->GetLinearVelocity();
	const float speed = velocity.Length();
	Vector3 drag = -m_airDrag * speed * velocity;

	if (m_controlInput.airBrake > 0.0f)
		drag *= m_controlInput.airBrake * m_airBrakePower;

	m_kb->AddForce(drag);
}

void Aircraft::OnLateUpdate() {
	PerformEvadeRoll();
}

void Aircraft::PerformEvadeRoll() noexcept {
	if (!m_aircraftBody) return;

	/*
	* return if not currently rolling &
	* no new roll input command
	*/
	if (!m_isEvadeRolling &&
		m_controlInput.evadeRoll == EvadeRoll::None &&
		m_currentRollingDir == EvadeRoll::None)
		return;

	using DirectX::SimpleMath::Quaternion;
	using DirectX::SimpleMath::Vector3;
	auto& rootTransform = GetTransform();

	// start new roll
	if (!m_isEvadeRolling) {
		m_isEvadeRolling = true;
		m_currentRollingDir = m_controlInput.evadeRoll;
		m_startRotation = m_aircraftBody->GetLocalRotation();
		m_startRight = rootTransform.GetRight();
		m_evadeRollElapsedTime = 0.0f;
		m_previousDisplaceOffset = 0.0f;
	}

	m_evadeRollElapsedTime += Time::DeltaTime();

	float t = m_evadeRollElapsedTime / m_evadeRollDuration;
	t = std::clamp(t, 0.0f, 1.0f);
	float angle = DirectX::XMConvertToRadians(m_evadeRollAngle) * t;

	angle *= (int)m_currentRollingDir;

	// roll (the body)
	auto rollDelta = Quaternion::CreateFromAxisAngle(Vector3::Forward, angle);
	const Quaternion result = Quaternion::Concatenate(rollDelta, m_startRotation);
	m_aircraftBody->SetLocalRotation(result);

	// displace (the root)
	float currentOffset = m_evadeDistance * t;
	// offset to move this frame
	float deltaOffset = currentOffset - m_previousDisplaceOffset;
	Vector3 newPosition = rootTransform.GetPosition() + m_startRight * (int)m_currentRollingDir * deltaOffset;
	rootTransform.SetPosition(newPosition);
	m_previousDisplaceOffset = currentOffset;

	// finish the roll
	if (m_evadeRollElapsedTime >= m_evadeRollDuration) {
		m_isEvadeRolling = false;
		m_currentRollingDir = EvadeRoll::None;

		// first and last rotation should be same
		m_aircraftBody->SetLocalRotation(m_startRotation);
	}
}