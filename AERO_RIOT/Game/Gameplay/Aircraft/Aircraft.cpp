#include "Aircraft.h"

#include <SNX/Core/Components/Transform.h>
#include <SNX/Core/Time.h>

void Aircraft::OnLateUpdate() {
	using DirectX::SimpleMath::Vector3;
	using DirectX::SimpleMath::Quaternion;

	auto& transform = GetTransform();

	// rotation
	float speedDelta = m_rotationSpeed * Time::DeltaTime();
	transform.RotateEulerDegrees({
		-m_controlInput.pitch * speedDelta,
		-m_controlInput.turn * speedDelta,
		m_controlInput.turn * speedDelta
		});

	// movement
	if (m_controlInput.throttle > 0.0f && m_controlInput.airBrake == 0.0f)
		m_currentSpeed += m_speedRate * m_controlInput.throttle;
	else if (m_controlInput.airBrake > 0.0f)
		m_currentSpeed -= m_speedRate * m_speedDrag * m_airBrakePower * m_controlInput.airBrake;
	else
		m_currentSpeed -= m_speedRate * m_speedDrag;

	m_currentSpeed = std::clamp(m_currentSpeed, 0.0f, m_maxSpeed);

	Vector3 posChange = transform.GetForward() * m_currentSpeed * Time::DeltaTime();
	transform.SetPosition(transform.GetPosition() + posChange);

	PerformEvadeRoll();
}

void Aircraft::PerformEvadeRoll() noexcept {
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
	auto& transform = GetTransform();

	// start new roll
	if (!m_isEvadeRolling) {
		m_isEvadeRolling = true;
		m_currentRollingDir = m_controlInput.evadeRoll;
		m_startRotation = transform.GetRotation();
		m_startForward = transform.GetForward();
		m_startRight = transform.GetRight();
		m_evadeRollElapsedTime = 0.0f;
	}
	// continue roll
	else
		m_evadeRollElapsedTime += Time::DeltaTime();

	float t = m_evadeRollElapsedTime / m_evadeRollDuration;
	float angle = DirectX::XMConvertToRadians(m_evadeRollAngle) * t;

	angle *= (int)m_currentRollingDir;

	// roll
	auto rollDelta = Quaternion::CreateFromAxisAngle(m_startForward, angle);
	transform.SetRotation(m_startRotation * rollDelta);

	// displace
	Vector3 displacementDelta = m_startRight * (int)m_currentRollingDir * m_evadeDistance * t;
	transform.SetPosition(transform.GetPosition() + displacementDelta);

	// finish the roll
	if (m_evadeRollElapsedTime >= m_evadeRollDuration) {
		m_isEvadeRolling = false;
		m_currentRollingDir = EvadeRoll::None;
	}
}