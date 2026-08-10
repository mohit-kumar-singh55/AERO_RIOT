#include "Time.h"

#include <algorithm>

Time::Clock::time_point Time::s_previousTime{};

float Time::s_deltaTime = 0.0f;
float Time::s_unscaledDeltaTime = 0.0f;
float Time::s_fixedDeltaTime = 1.0f / 60.0f;
float Time::s_fixedAccumulator = 0.0f;
float Time::s_timeScale = 1.0f;

double Time::s_totalTime = 0.0;
std::uint64_t Time::s_frameCount = 0;

bool Time::s_initialized = false;

void Time::Initialize() noexcept {
	Reset();

	//s_previousTime = Clock::now();
	s_initialized = true;
}

void Time::BeginFrame() noexcept {
	// initialize if not
	if (!s_initialized) {
		Initialize();
		return;
	}

	// calculate unscaled delta time
	const Clock::time_point currentTime = Clock::now();

	float realDeltaTime = std::chrono::duration<float>(
		currentTime - s_previousTime
	).count();

	s_previousTime = currentTime;

	// ! safety clamp
	// ! this prevents a huge jump if the program freezes for a moment
	realDeltaTime = std::clamp(realDeltaTime, 0.0f, MaxDeltaTime);

	s_unscaledDeltaTime = realDeltaTime;
	s_deltaTime = realDeltaTime * s_timeScale;

	s_totalTime += static_cast<double>(s_deltaTime);

	s_frameCount++;

	const float maximumAccumulator = s_fixedDeltaTime * static_cast<float>(MaxFixedStepsPerFrame);
	s_fixedAccumulator = std::min(s_fixedAccumulator + s_deltaTime, maximumAccumulator);
}

void Time::Reset() noexcept {
	s_deltaTime = 0.0f;
	s_unscaledDeltaTime = 0.0f;
	s_fixedAccumulator = 0.0f;

	s_totalTime = 0.0f;
	s_frameCount = 0;

	s_previousTime = Clock::now();
}

void Time::SetTimeScale(float timeScale) noexcept {
	s_timeScale = std::max(0.0f, timeScale);
}

void Time::SetFixedDeltaTime(float fixedDeltaTime) noexcept {
	if (fixedDeltaTime <= 0.0f)
		return;

	s_fixedDeltaTime = fixedDeltaTime;
	s_fixedAccumulator = 0.0f;
}

bool Time::HasFixedStep() noexcept {
	return s_fixedAccumulator >= s_fixedDeltaTime;
}

void Time::ConsumeFixedStep() noexcept {
	if (!HasFixedStep())
		return;

	s_fixedAccumulator -= s_fixedDeltaTime;
}

float Time::FixedInterpolationAlpha() noexcept {
	if (s_fixedDeltaTime <= 0.0f)
		return 0.0f;

	return std::clamp(s_fixedAccumulator / s_fixedDeltaTime, 0.0f, 1.0f);
}
