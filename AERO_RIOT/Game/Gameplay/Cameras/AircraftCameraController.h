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
	DirectX::SimpleMath::Vector3 m_cameraOffset{ 0.0f,4.0f,10.0f };		// camera offset from the target
	DirectX::SimpleMath::Vector3 m_lookAheadOffset{ 0.0f,0.0f,-2.0f };		// look offset from the target
};