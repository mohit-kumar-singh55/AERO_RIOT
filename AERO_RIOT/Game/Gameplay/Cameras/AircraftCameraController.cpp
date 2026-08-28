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

	m_mainCam->LookAt(
		m_target->GetPosition() + m_cameraOffset,
		m_target->GetPosition() + m_lookAheadOffset
	);
}