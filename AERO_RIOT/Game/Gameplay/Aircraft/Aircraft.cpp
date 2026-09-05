#include "Aircraft.h"

#include <SNX/Core/Components/Transform.h>
#include <SNX/Core/Object/GameObject.h>
#include <SNX/Core/Time.h>

void Aircraft::OnInitialize() {
	// ! considering the body gameobject is at index 0
	m_aircraftBody = GetGameObject().GetTransform().GetChild(0)->GetGameObject();

	if (!m_aircraftBody)
		throw std::runtime_error("Aircraft: Cannot find Aircraft Body Gameobject at child index 0.");
}

void Aircraft::OnLateUpdate() {
	using DirectX::SimpleMath::Vector3;
	using DirectX::SimpleMath::Quaternion;

	auto& transform = GetTransform();

	// rotation
	float speedDelta = m_rotationSpeed * Time::DeltaTime();
	transform.RotateEulerDegrees({
		-m_controlInput.pitch * speedDelta,
		-m_controlInput.turn * speedDelta,
		-m_controlInput.turn * speedDelta
		});

	// movement
	float speedRateOfChange = m_speedRate * Time::DeltaTime();
	if (m_controlInput.throttle > 0.0f && m_controlInput.airBrake == 0.0f)
		m_currentSpeed += speedRateOfChange * m_controlInput.throttle;
	else if (m_controlInput.airBrake > 0.0f)
		m_currentSpeed -= speedRateOfChange * m_speedDrag * m_airBrakePower * m_controlInput.airBrake;
	else
		m_currentSpeed -= speedRateOfChange * m_speedDrag;

	m_currentSpeed = std::clamp(m_currentSpeed, 0.0f, m_maxSpeed);

	Vector3 posChange = transform.GetForward() * m_currentSpeed * Time::DeltaTime();
	transform.SetPosition(transform.GetPosition() + posChange);

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
	auto& bodyTransform = m_aircraftBody->GetTransform();

	// start new roll
	if (!m_isEvadeRolling) {
		m_isEvadeRolling = true;
		m_currentRollingDir = m_controlInput.evadeRoll;
		m_startRotation = bodyTransform.GetRotation();
		m_startForward = bodyTransform.GetForward();
		m_startRight = bodyTransform.GetRight();
		m_evadeRollElapsedTime = 0.0f;
		m_previousDisplaceOffset = 0.0f;
	}

	m_evadeRollElapsedTime += Time::DeltaTime();

	float t = m_evadeRollElapsedTime / m_evadeRollDuration;
	t = std::clamp(t, 0.0f, 1.0f);
	float angle = DirectX::XMConvertToRadians(m_evadeRollAngle) * t;

	angle *= (int)m_currentRollingDir;

	// roll (the body)
	auto rollDelta = Quaternion::CreateFromAxisAngle(m_startForward, angle);
	bodyTransform.SetRotation(m_startRotation * rollDelta);

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
	}
}