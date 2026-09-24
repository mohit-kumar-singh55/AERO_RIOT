#pragma once

#include <string>
#include <vector>

#include <SpriteBatch.h>
#include <SpriteFont.h>

enum class DebugSeverity {
	Log,
	Warning,
	Error
};

struct DebugMessage {
	DebugMessage(std::wstring str, float lifeTime, DebugSeverity level) {
		text = str;
		remainingTime = lifeTime;
		severity = level;
	}

	std::wstring text;
	float remainingTime;
	DebugSeverity severity;
};

class Debug final {
public:
	Debug() = delete;
	~Debug() {
		m_logs.clear();
	}

	static void Log(
		std::wstring text,
		float lifeTime = 1.0f,
		DebugSeverity severity = DebugSeverity::Log
	);

	static void LogWarning(std::wstring text, float lifeTime = 1.0f) {
		Log(text, lifeTime, DebugSeverity::Warning);
	}

	static void LogError(std::wstring text, float lifeTime = 1.0f) {
		Log(text, lifeTime, DebugSeverity::Error);
	}

private:
	static void Update(float deltaTime) noexcept;

	static void Draw(
		DirectX::SpriteBatch* spriteBatch,
		DirectX::SpriteFont* font
	) noexcept;

	static DirectX::XMVECTORF32 GetColor(DebugSeverity severity) noexcept {
		switch (severity) {
		case DebugSeverity::Error:
			return { 1.0f, 0.1f, 0.1f, 1.0f };
		case DebugSeverity::Warning:
			return { 1.0f, 1.0f, 0.1f, 1.0f };
		case DebugSeverity::Log:
		default:
			return { 1.0f, 1.0f, 1.0f, 1.0f };
		}
	}

private:
	static std::vector<DebugMessage> m_logs;

	static constexpr float m_xPos = 20.0f;
	static constexpr float m_startY = 100.0f;
	static constexpr float m_lineHeight = 40.0f;

	friend class Game;
};