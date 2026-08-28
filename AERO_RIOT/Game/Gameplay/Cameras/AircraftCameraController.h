#pragma once

#include <SNX/Core/Object/Component.h>
#include <SNX/Core/Components/Camera/Camera.h>

#include <SimpleMath.h>

class AircraftCameraController final :public Component {
public:
	AircraftCameraController(GameObject& gameObject, Transform* target) noexcept;

protected:
	void OnInitialize() override;

	void OnDestroy() override;

	void OnLateUpdate() override;

private:
	Camera* m_mainCam = nullptr;
	Transform* m_target = nullptr;

	// configurable
	float m_followDistance = 6.0f;
	float m_height = 6.0f;
	float m_lookAheadDistance = 2.0f;
};