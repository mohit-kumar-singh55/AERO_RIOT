#pragma once

#include <SNX/Core/Object/Component.h>
#include <SNX/Core/Components/Camera/Camera.h>

class AircraftCameraController final :public Component {
public:
	AircraftCameraController(GameObject& gameObject, Transform* target) noexcept;

protected:
	void OnInitialize() override;

	void OnStart() override;

	void OnDestroy() override;

	void OnLateUpdate() override;

private:
	Camera* m_mainCam = nullptr;
	Transform* m_target = nullptr;

	float m_orbitYaw = 0.0f;
	float m_orbitPitch = 0.0f;
	// captured yaw & pitch when re-centering started
	float m_lastOrbitYaw = 0.0f;
	float m_lastOrbitPitch = 0.0f;

	// orbit settings
	// initial & default orbit yar & pitch
	float m_defaultOrbitYaw = DirectX::g_XMPi.f[0];
	float m_defaultOrbitPitch = 0.5f;
	float m_freeLookSpeed = 2.0f;	// radians/s
	float m_maxOrbitPitch = 60.0f;	// degrees
	float m_followDistance = 6.0f;	// orbit radius

	// auto re-center (auto reset the camera's rotation around the pivot to its inital state)
	float m_inputUnavailabilityInterval = 1.0f;		// time after which, auto re-center will start if no rotational input is provided between this interval
	float m_timeToReset = 2.0f;		// total time taken to go from current position to the initial position
	float m_inputUnavailabilityTimer = 0.0f;
	float m_elapsedTimeToReset = 0.0f;

	float m_lookAheadDistance = 2.0f;
};