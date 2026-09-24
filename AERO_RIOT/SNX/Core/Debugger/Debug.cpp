#include "pch.h"
#include "Debug.h"

void Debug::Update(float deltaTime) noexcept {
	// decrease timer, remove if time out
	for (auto i = m_logs.begin(); i != m_logs.end(); i++) {
		i->remainingTime -= deltaTime;

		if (i->remainingTime <= 0.0f)
			i = m_logs.erase(i);
	}
}

void Debug::Draw(
	DirectX::SpriteBatch* spriteBatch,
	DirectX::SpriteFont* font
) noexcept {
	// draw, stack, set color based on severity
	for (auto& msg : m_logs) {
		font->DrawString(
			spriteBatch,
			msg.text.c_str(),
			DirectX::SimpleMath::Vector2(
				20.0f,
				140.0f
			),
			GetColor(msg.severity)
		);
	}
}

void Debug::Log(std::wstring text, float lifeTime, DebugSeverity severity) noexcept {
	m_logs.emplace_back(text, lifeTime, severity);
}