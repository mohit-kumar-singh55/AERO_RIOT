#pragma once

#include <algorithm>

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