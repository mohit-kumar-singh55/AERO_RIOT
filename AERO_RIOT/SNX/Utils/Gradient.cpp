#include "pch.h"
#include "Gradient.h"

#include <algorithm>

Gradient::Gradient(std::span<GradientKey> keys) {
	SetKeys(keys);
}

void Gradient::SetKeys(std::span<GradientKey> keys) {
	m_keys.insert(m_keys.end(), keys.begin(), keys.end());

	// sort in accending order of position
	std::sort(
		m_keys.begin(), m_keys.end(),
		[](const GradientKey& a, const GradientKey& b) {
			return a.position < b.position;
		}
	);

	// remove duplicates (based on position) (keeps the last "latest" key among the duplicates)
	for (auto i = m_keys.begin(); i != m_keys.end();) {
		auto next = std::next(i);

		if (next != m_keys.end() && i->position == next->position)
			i = m_keys.erase(i);
		else
			++i;
	}
}

DirectX::SimpleMath::Vector4 Gradient::Evaluate(float position) const noexcept {
	using DirectX::SimpleMath::Vector4;

	position = std::clamp(position, 0.0f, 1.0f);

	GradientKey keyBefore;
	GradientKey keyAfter;
	for (auto& key : m_keys) {
		if (key.position == position)
			return key.color;

		else if (key.position < position)
			keyBefore = key;

		else if (key.position > position) {
			keyAfter = key;
			break;
		}
	}

	// calculate where "position" lies b/w "keyBefore" it and "keyAfter" it
	float localT =
		(position - keyBefore.position)
		/
		(keyAfter.position - keyBefore.position);

	// calculate the color at that position
	return Vector4::Lerp(keyBefore.color, keyAfter.color, localT);
}
