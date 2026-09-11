#include "pch.h"

#include "AudioManager.h"

void AudioManager::Initialize() {
	Shutdown();

	m_audioEngine = std::make_unique<DirectX::AudioEngine>();
}

void AudioManager::Shutdown() noexcept {
	ClearSounds();
	m_audioEngine.reset();
}

void AudioManager::Update() {
	if (!m_audioEngine) return;

	if (!m_audioEngine->Update()) {
		if (m_audioEngine->IsCriticalError()) {
			// TODO: Later: device reset / audio recovery
		}
	}
}

bool AudioManager::LoadSound(const std::string& name, const wchar_t* filePath) {
	if (!m_audioEngine || name.empty() || !filePath)
		return false;

	try {
		m_sounds[name] = std::make_unique<DirectX::SoundEffect>(
			m_audioEngine.get(),
			filePath
		);

		return true;
	}
	catch (const std::exception&) {
		return false;
	}
}

bool AudioManager::PlaySound(const std::string& name) {
	const auto iterator = m_sounds.find(name);

	if (iterator == m_sounds.end() || !iterator->second)
		return false;

	iterator->second->Play();

	return true;
}

void AudioManager::UnloadSound(const std::string& name) noexcept {
	m_sounds.erase(name);
}

void AudioManager::ClearSounds() noexcept {
	m_sounds.clear();
}

bool AudioManager::HasSound(const std::string& name) const noexcept {
	return m_sounds.find(name) != m_sounds.end();
}