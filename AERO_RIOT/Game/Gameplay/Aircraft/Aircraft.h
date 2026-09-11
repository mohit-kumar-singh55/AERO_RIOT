#pragma once

#include <SNX/Core/Object/Component.h>

#include <SimpleMath.h>

#include <algorithm>

class KineticBody;

enum class EvadeRoll { None, Left = -1, Right = 1 };

struct AircraftControlInput final {
	// -1 ... +1
	float pitch = 0;
	float turn = 0;

	// 0 ... 1
	float throttle = 0;
	float airBrake = 0;

	EvadeRoll evadeRoll = EvadeRoll::None;

	[[nodiscard]]
	AircraftControlInput& GetNormalized() & noexcept [[msvc::lifetimebound]] {
		pitch = std::clamp(pitch, -1.0f, 1.0f);
		turn = std::clamp(turn, -1.0f, 1.0f);
		throttle = std::clamp(throttle, 0.0f, 1.0f);
		airBrake = std::clamp(airBrake, 0.0f, 1.0f);

		return *this;
	}
};

class Aircraft final : public Component {
public:
	using Component::Component;

	void SetControlInput(AircraftControlInput controlInput) noexcept {
		m_controlInput = controlInput.GetNormalized();
	}

protected:
	void OnInitialize() override;

	void OnStart() override;

	void OnFixedUpdate() override;
	void OnLateUpdate() override;

private:
	void PerformEvadeRoll() noexcept;

private:
	AircraftControlInput m_controlInput;
	Transform* m_aircraftBody = nullptr;		// visual child
	KineticBody* m_kb = nullptr;

	// configurable
	float m_maxThrust = 20.0f;
	float m_airDrag = 1.0f;		// aerodynamic drag coefficient
	float m_airBrakePower = 4.0f;	// additional drag

	float m_rotationSpeed = 60.0f;	// degree/s

	float m_evadeRollDuration = 0.3f;
	float m_evadeRollAngle = 360.0f;	// degrees
	float m_evadeDistance = 5.0f;		// amount of displacement when doing evade roll

	// evade roll purpose **
	bool m_isEvadeRolling = false;
	EvadeRoll m_currentRollingDir = EvadeRoll::None;
	DirectX::SimpleMath::Quaternion m_startRotation;
	DirectX::SimpleMath::Vector3 m_startRight;
	float m_evadeRollElapsedTime = 0.0f;
	float m_previousDisplaceOffset = 0.0f;
	// *********************
};