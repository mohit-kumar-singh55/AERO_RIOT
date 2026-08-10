#pragma once

#include <Audio.h>

#include <memory>
#include <string>
#include <unordered_map>

class AudioManager {
public:
	AudioManager() = default;
	~AudioManager() { Shutdown(); }

	// disallowing to copy or move
	AudioManager(const AudioManager&) = delete;
	AudioManager& operator=(const AudioManager&) = delete;
	AudioManager(AudioManager&&) = delete;
	AudioManager& operator=(AudioManager&&) = delete;

	void Initialize();
	void Shutdown() noexcept;

	void Update();

	bool LoadSound(const std::string& name, const wchar_t* filePath);
	bool PlaySound(const std::string& name);
	void UnloadSound(const std::string& name) noexcept;
	void ClearSounds() noexcept;

	[[nodiscard]]
	bool HasSound(const std::string& name) const noexcept;

private:
	std::unique_ptr<DirectX::AudioEngine> m_audioEngine;

	std::unordered_map<
		std::string,
		std::unique_ptr<DirectX::SoundEffect>
	> m_sounds;
};