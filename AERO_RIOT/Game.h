#pragma once

#define WIN32_LEAN_AND_MEAN

// window related
#include <windows.h>

// DirectXTK
#include <SpriteBatch.h>
#include <SpriteFont.h>

// game related
#include <SNX/Audio/AudioManager.h>
#include <SNX/Core/Scene/SceneManager.h>
#include <SNX/Graphics/DeviceResources.h>

class Game final {
public:
	Game() = default;
	~Game() = default;

	// disallowing to copy or move
	Game(const Game&) = delete;
	Game& operator=(const Game&) = delete;
	Game(Game&&) = delete;
	Game& operator=(Game&&) = delete;

	void Initialize(HWND window, int width, int height);

	void Tick();

private:
	void InitializeGameResources();
	void InitializeScenes();

	void Render();

private:
	// device related
	DeviceResources m_deviceResources;

	// UI objects renderer
	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
	std::unique_ptr<DirectX::SpriteFont> m_font;	// ui text renderer

	AudioManager m_audioManager;

	// must remain alive while SceneManager exists
	std::unique_ptr<SceneContext> m_sceneContext;

	// declared last so it is destroyed first
	std::unique_ptr<SceneManager> m_sceneManager;
};