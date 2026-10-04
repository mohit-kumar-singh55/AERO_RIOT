#pragma once

#include <SimpleMath.h>

#include <vector>
#include <span>

struct GradientKey {
	// 0.0 ~ 1.0
	float position;
	// RGBA
	DirectX::SimpleMath::Vector4 color = DirectX::SimpleMath::Vector4::One;
};

class Gradient final {
public:
	Gradient() = default;
	Gradient(std::span<GradientKey> keys);

	// appends to the current keys
	void SetKeys(std::span<GradientKey> keys);

	void Clear() noexcept {
		m_keys.clear();
		// keep default keys
		m_keys.assign({
			{ 0.0f, { 1.0f, 1.0f, 1.0f, 1.0f } },
			{ 1.0f, { 1.0f, 1.0f, 1.0f, 1.0f } }
			});
	}

	DirectX::SimpleMath::Vector4 Evaluate(float position) const noexcept;

private:
	// default to white
	std::vector<GradientKey> m_keys{
		{ 0.0f, { 1.0f, 1.0f, 1.0f, 1.0f } },
		{ 1.0f, { 1.0f, 1.0f, 1.0f, 1.0f } }
	};
};