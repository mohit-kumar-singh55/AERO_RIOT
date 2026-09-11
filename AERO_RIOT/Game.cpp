#include "pch.h"

#include "Game.h"

#include <Scenes/SceneIds.h>
#include <Scenes/MainScene.h>

#include <SNX/Core/Time.h>
#include <SNX/Input/InputManager.h>

#include <stdexcept>

void Game::Initialize(HWND window, const AppConfig& appConfig) {
	m_appConfig = appConfig;

	m_deviceResources.Initialize(window, m_appConfig.windowWidth, m_appConfig.windowHeight);

	// init singleton input manager
	InputManager::Get().Initialize(window);

	InitializeGameResources();

	m_audioManager.Initialize();

	Time::Initialize();

	// initialize manager and register scenes
	InitializeScenes();
}

void Game::Tick() {
	Time::BeginFrame();

	// read hardware input exactly once per rendered frame
	InputManager::Get().Update();

	if (!m_sceneManager) return;

	// register objects created before this frame
	m_sceneManager->BeginFrame();

	/*
	* a slow rendered frame may required multiple fixed updates
	* fixedupdate may run zero or multiple times,
	* because physics requires to run exactly 1/60 s per update (as we defined in fixedDeltaTime)
	* but each frame's deltaTime is different
	*/
	while (Time::HasFixedStep()) {
		m_sceneManager->FixedUpdate();

		Time::ConsumeFixedStep();
	}

	m_sceneManager->Update();

	m_sceneManager->LateUpdate();

	m_audioManager.Update();

	Render();

	/*
	* gameobject, component destruction and scene transition
	* are safely finalized here
	*/
	m_sceneManager->EndFrame();
}

void Game::InitializeGameResources() {
	ID3D11Device* device = m_deviceResources.GetDevice();
	ID3D11DeviceContext* context = m_deviceResources.GetContext();

	// create DirectXTK objects
	m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);
	m_font = std::make_unique<DirectX::SpriteFont>(device, L"Assets/gamefont.spritefont");
}

void Game::InitializeScenes() {
	if (!m_spriteBatch || !m_font)
		throw std::runtime_error("Scene system requires SpriteBatch and SpriteFont to be initialized.");

	m_sceneContext = std::make_unique<SceneContext>(
		m_deviceResources,
		*m_spriteBatch,
		*m_font,
		m_audioManager,
		[]() { PostQuitMessage(0); }	// quit callback
	);

	m_sceneManager = std::make_unique<SceneManager>(*m_sceneContext);

	// ! register scenes
	m_sceneManager->RegisterScene<MainScene>(SceneIds::Main, "Main");

	// ! load the first scene
	if (!m_sceneManager->Start(SceneIds::Main))
		throw std::runtime_error("Failed to start the initial scene");
}

void Game::Render() {
	if (!m_sceneManager || !m_spriteBatch || !m_font)
		return;

	// bg color
	const auto clearColor = m_sceneManager->GetClearColor();

	// bind render targets, sets viewport and clears both buffers
	m_deviceResources.BeginFrame(clearColor.data());

	// 3D/world phase
	m_sceneManager->RenderWorld();

	// 2D/UI phase
	m_spriteBatch->Begin();

	m_sceneManager->RenderUI();

	m_spriteBatch->End();
	// ***********

	// swap chain present
	m_deviceResources.Present(m_appConfig.useVSync);
}