#include "Aircraft.h"

#include <SNX/Core/Components/Transform.h>
#include <SNX/Core/Time.h>

void Aircraft::OnUpdate() {
	using DirectX::SimpleMath::Vector3;
	using DirectX::SimpleMath::Quaternion;

	auto& transform = GetTransform();

	// rotation
	auto finalRotation = Quaternion::CreateFromAxisAngle(transform.GetRight(), m_rotationSpeed * m_controlInput.pitch) *
		Quaternion::CreateFromAxisAngle(transform.GetUp(), m_rotationSpeed * m_controlInput.turn) *
		Quaternion::CreateFromAxisAngle(transform.GetForward(), m_rotationSpeed * m_controlInput.pitch * m_controlInput.turn);

	//auto lerpedRot = Quaternion::Slerp()

	transform.SetRotation(finalRotation * Time::DeltaTime());

	// position
	//Vector3 targetPos = transform.GetPosition() + transform.GetForward() * m_controlInput.throttle * m_speed * Time::DeltaTime();
	//transform.SetPosition(Vector3::Lerp(transform.GetPosition(), targetPos, Time::DeltaTime()));
	//transform.SetPosition(targetPos);
	Vector3 posChange = transform.GetForward() * m_controlInput.throttle * m_speed * Time::DeltaTime();
	posChange *= 1 - m_controlInput.airBrake;
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
	auto& transform = GetTransform();

	// continue rolling towards the cached rolling direction command
	if (m_isEvadeRolling && m_currentRollingDir != EvadeRoll::None) {
		auto targetRot = Quaternion::CreateFromAxisAngle(transform.GetForward(), (float)m_currentRollingDir);
		auto lerpedRot = Quaternion::Slerp(transform.GetRotation(), targetRot, m_evadeRollRate * Time::DeltaTime());
		transform.SetRotation(lerpedRot);

		if (transform.GetRotation() == targetRot) {
			m_isEvadeRolling = false;
			m_currentRollingDir = EvadeRoll::None;
		}

		return;
	}

	// start new rolling
	m_isEvadeRolling = true;
	m_currentRollingDir = m_controlInput.evadeRoll;

	auto targetRot = Quaternion::CreateFromAxisAngle(transform.GetForward(), (float)m_currentRollingDir);
	auto lerpedRot = Quaternion::Slerp(transform.GetRotation(), targetRot, m_evadeRollRate * Time::DeltaTime());
	transform.SetRotation(lerpedRot);
}