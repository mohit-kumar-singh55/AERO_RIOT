#include "pch.h"

#include "AircraftCameraController.h"

#include <SNX/Input/InputManager.h>
#include <SNX/Core/Object/GameObject.h>
#include <SNX/Core/Time.h>

#include <stdexcept>
#include <cmath>
#include <algorithm>

AircraftCameraController::AircraftCameraController(
	GameObject& gameObject,
	Transform* target
) noexcept :
	Component(gameObject),
	m_target(target) {}

void AircraftCameraController::OnInitialize() {
	m_mainCam = GetGameObject().GetComponent<Camera>();

	if (!m_mainCam)
		throw std::runtime_error("AircraftCameraController: Cannot find Camera Component.");
}

void AircraftCameraController::OnStart() {
	// set the intial look
	m_orbitYaw = m_defaultOrbitYaw;
	m_orbitPitch = m_defaultOrbitPitch;

	// convert degree to radian
	m_maxOrbitPitch = DirectX::XMConvertToRadians(m_maxOrbitPitch);
}

void AircraftCameraController::OnDestroy() {
	m_target = nullptr;
	m_mainCam = nullptr;
}

void AircraftCameraController::OnLateUpdate() {
	if (!m_mainCam || !m_target) return;

	using DirectX::SimpleMath::Vector3;

	auto pivot = m_target->GetRenderPosition();
	auto targetForward = m_target->GetRenderForward();
	auto targetUp = m_target->GetRenderUp();
	auto targetRight = m_target->GetRenderRight();

	auto& input = InputManager::Get();

	auto stickVal = input.GetGamePadStick(GamePadStick::RightStick);

	// use input to rotate around pivot
	if (stickVal.x || stickVal.y) {
		m_inputUnavailabilityTimer = 0.0f;

		m_orbitYaw += stickVal.x * m_freeLookSpeed * Time::DeltaTime();
		m_orbitPitch += stickVal.y * m_freeLookSpeed * Time::DeltaTime();
	}
	// if no input, increase timer
	else if (m_inputUnavailabilityTimer < m_inputUnavailabilityInterval) {
		m_inputUnavailabilityTimer += Time::DeltaTime();

		// once timer completes, set the reset values
		if (m_inputUnavailabilityTimer >= m_inputUnavailabilityInterval) {
			m_elapsedTimeToReset = 0.0f;

			m_lastOrbitYaw = m_orbitYaw;
			m_lastOrbitPitch = m_orbitPitch;
		}
	}
	// reset camera yaw & pitch to default rotation around pivot
	else if (m_elapsedTimeToReset <= m_timeToReset) {
		m_elapsedTimeToReset += Time::DeltaTime();

		float t = m_elapsedTimeToReset / m_timeToReset;
		t = std::clamp(t, 0.0f, 1.0f);

		m_orbitYaw = std::lerp(m_lastOrbitYaw, m_defaultOrbitYaw, t);
		m_orbitPitch = std::lerp(m_lastOrbitPitch, m_defaultOrbitPitch, t);
	}

	// limit the pitch
	m_orbitPitch = std::clamp(m_orbitPitch, -m_maxOrbitPitch, m_maxOrbitPitch);

	//auto cameraPos = GetCameraPosition(m_target);

	auto cameraPos = Vector3::Lerp(GetTransform().GetPosition(), GetCameraPosition(m_target), Time::DeltaTime() * 5.0f);

	auto lookTarget =
		pivot
		+ targetForward * m_lookAheadDistance;

	m_mainCam->LookAt(cameraPos, lookTarget, Vector3::Up);
	//m_mainCam->LookAt(cameraPos, lookTarget, targetUp);
}

DirectX::SimpleMath::Vector3 AircraftCameraController::GetCameraPosition(const Transform* target) const noexcept {
	using DirectX::SimpleMath::Vector3;

	auto pivot = target->GetRenderPosition();
	auto targetForward = target->GetRenderForward();
	auto targetUp = target->GetRenderUp();
	auto targetRight = target->GetRenderRight();

	Vector3 orbitOffset = GetOrbitCameraOffset();

	Vector3 offsetFromPivot =
		targetRight * orbitOffset.x
		+ targetUp * orbitOffset.y
		+ targetForward * orbitOffset.z;

	return pivot + offsetFromPivot;
}

DirectX::SimpleMath::Vector3 AircraftCameraController::GetOrbitCameraOffset() const noexcept {
	return {
		m_followDistance * std::sin(m_orbitYaw) * std::cos(m_orbitPitch),
		m_followDistance * std::sin(m_orbitPitch),
		m_followDistance * std::cos(m_orbitYaw) * std::cos(m_orbitPitch)
	};
}
