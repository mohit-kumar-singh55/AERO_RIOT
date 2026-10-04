#include "pch.h"
#include "Gradient.h"

#include <algorithm>

Gradient::Gradient(std::span<const GradientKey> keys) {
	SetKeys(keys);
}

void Gradient::SetKeys(std::span<const GradientKey> keys) {
	m_keys.clear();
	m_keys.reserve(keys.size());

	for (const auto& key : keys)
		AddKey(key);
}

void Gradient::AddKey(GradientKey key) {
	key.position = std::clamp(key.position, 0.0f, 1.0f);

	// If a key already exists at this position, the newest value replaces it.
	auto existing = std::find_if(
		m_keys.begin(),
		m_keys.end(),
		[&key](const GradientKey& current) {
			return current.position == key.position;
		}
	);

	if (existing != m_keys.end())
		*existing = key;
	else
		m_keys.push_back(key);

	std::sort(
		m_keys.begin(),
		m_keys.end(),
		[](const GradientKey& a, const GradientKey& b) {
			return a.position < b.position;
		}
	);
}

DirectX::SimpleMath::Vector4 Gradient::Evaluate(float position) const noexcept {
	using DirectX::SimpleMath::Vector4;

	// default to white
	if (m_keys.empty())
		return Vector4::One;

	position = std::clamp(position, 0.0f, 1.0f);

	if (position <= m_keys.front().position)
		return m_keys.front().color;

	if (position >= m_keys.back().position)
		return m_keys.back().color;

	for (std::size_t i = 1; i < m_keys.size(); ++i) {
		const GradientKey& keyAfter = m_keys[i];

		if (position > keyAfter.position)
			continue;

		const GradientKey& keyBefore = m_keys[i - 1];

		// convert the gradient-wide position into a 0..1 value between these two keys.
		const float localT =
			(position - keyBefore.position)
			/
			(keyAfter.position - keyBefore.position);

		return Vector4::Lerp(keyBefore.color, keyAfter.color, localT);
	}

	// defensive fallback; the boundary checks above should normally handle this.
	return m_keys.back().color;
}
