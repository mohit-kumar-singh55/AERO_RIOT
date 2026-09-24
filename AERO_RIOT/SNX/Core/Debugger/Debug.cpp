#include "pch.h"
#include "Debug.h"

std::vector<DebugMessage> Debug::m_logs{};

void Debug::Update(float deltaTime) noexcept {
	// decrease timer, remove if time out
	for (auto i = m_logs.begin(); i != m_logs.end();) {
		i->remainingTime -= deltaTime;

		if (i->remainingTime <= 0.0f)
			i = m_logs.erase(i);
		else
			i++;
	}
}

void Debug::Draw(
	DirectX::SpriteBatch* spriteBatch,
	DirectX::SpriteFont* font
) noexcept {
	int count = 0;

	// draw, stack, set color based on severity
	for (auto& msg : m_logs) {
		// on-screen logs
		font->DrawString(
			spriteBatch,
			msg.text.c_str(),
			DirectX::SimpleMath::Vector2(
				m_xPos,
				m_startY + count * m_lineHeight
			),
			GetColor(msg.severity)
		);

		// visual studio console logs
		OutputDebugStringW(msg.text.c_str());
		OutputDebugStringW(L"\n");

		count++;
	}
}

void Debug::Log(std::wstring text, float lifeTime, DebugSeverity severity) {
	m_logs.emplace_back(text, lifeTime, severity);
}