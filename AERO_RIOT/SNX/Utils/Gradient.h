#pragma once

#include <SimpleMath.h>

#include <span>
#include <vector>

struct GradientKey {
	// 0.0 ~ 1.0
	float position = 0.0f;
	// RGBA
	DirectX::SimpleMath::Vector4 color = DirectX::SimpleMath::Vector4::One;
};

class Gradient final {
public:
	Gradient() = default;
	explicit Gradient(std::span<const GradientKey> keys);

	// replaces the current keys
	void SetKeys(std::span<const GradientKey> keys);

	// adds a new key, or replaces an existing key at the same position
	void AddKey(GradientKey key);

	void Clear() noexcept { m_keys.clear(); }

	[[nodiscard]]
	DirectX::SimpleMath::Vector4 Evaluate(float position) const noexcept;

private:
	std::vector<GradientKey> m_keys;
};
