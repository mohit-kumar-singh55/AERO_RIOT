#pragma once

#include <SNX/Core/Object/Component.h>

#include <algorithm>

enum class EvadeRoll { None, Left = -360, Right = 360 };

struct AircraftControlInput final {
	// -1 ... +1
	float pitch;
	float turn;

	// 0 ... 1
	float throttle;
	float airBrake;

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

	void SetControlInput(AircraftControlInput& controlInput) noexcept {
		m_controlInput = controlInput.GetNormalized();
	}

protected:
	void OnUpdate() override;

private:
	void PerformEvadeRoll() noexcept;

private:
	AircraftControlInput m_controlInput;

	// configurable
	float m_speed = 5.0f;
	float m_rotationSpeed = 5.0f;
	float m_evadeRollRate = 10.0f;

	bool m_isEvadeRolling = false;
	EvadeRoll m_currentRollingDir = EvadeRoll::None;
};