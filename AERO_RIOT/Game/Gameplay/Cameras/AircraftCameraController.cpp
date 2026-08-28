#include "AircraftCameraController.h"

#include <SNX/Core/Object/GameObject.h>

#include <stdexcept>

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

void AircraftCameraController::OnDestroy() {
	m_target = nullptr;
	m_mainCam = nullptr;
}

void AircraftCameraController::OnLateUpdate() {
	if (!m_mainCam || !m_target) return;

	using DirectX::SimpleMath::Vector3;

	auto pivot = m_target->GetPosition();

	auto cameraPos =
		pivot
		- m_target->GetForward() * m_followDistance
		+ Vector3::Up * m_height;

	auto lookTarget =
		pivot
		+ m_target->GetForward() * m_lookAheadDistance;

	m_mainCam->LookAt(cameraPos, lookTarget, Vector3::Up);
}