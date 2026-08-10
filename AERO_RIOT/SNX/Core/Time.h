#pragma once

#include <chrono>
#include <cstdint>

class Time final {
public:
	Time() = delete;

	static void Initialize() noexcept;
	static void BeginFrame() noexcept;
	static void Reset() noexcept;

	[[nodiscard]]
	static float DeltaTime() noexcept { return s_deltaTime; }

	[[nodiscard]]
	static float UnscaledDeltaTime() noexcept { return s_unscaledDeltaTime; }

	[[nodiscard]]
	static float FixedDeltaTime() noexcept { return s_fixedDeltaTime; }

	[[nodiscard]]
	static double TotalTime() noexcept { return s_totalTime; }

	[[nodiscard]]
	static std::uint64_t FrameCound() noexcept { return s_frameCount; }

	[[nodiscard]]
	static float TimeScale() noexcept { return s_timeScale; }

	static void SetTimeScale(float timeScale) noexcept;
	static void SetFixedDeltaTime(float fixedDeltaTime) noexcept;

	[[nodiscard]]
	static bool HasFixedStep() noexcept;

	static void ConsumeFixedStep() noexcept;

	[[nodiscard]]
	static float FixedInterpolationAlpha() noexcept;

private:
	using Clock = std::chrono::steady_clock;

	static Clock::time_point s_previousTime;

	static float s_deltaTime;
	static float s_unscaledDeltaTime;
	static float s_fixedDeltaTime;
	static float s_fixedAccumulator;		//  stores how much fixed-update time has been collected but not yet simulated
	static float s_timeScale;

	static double s_totalTime;
	static std::uint64_t s_frameCount;

	static bool s_initialized;

	static constexpr float MaxDeltaTime = 0.1f;
	static constexpr int MaxFixedStepsPerFrame = 5;
};