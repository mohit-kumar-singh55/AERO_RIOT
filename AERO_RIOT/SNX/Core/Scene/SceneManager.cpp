#include "SceneManager.h"

#include <stdexcept>

SceneManager::SceneManager(SceneContext& context) noexcept :
	m_context(context) {}

SceneManager::~SceneManager() {
	Shutdown();
}

bool SceneManager::Start(SceneId sceneId) {
	if (m_currentScene) return false;
	if (!LoadScene(sceneId)) return false;
	// immediately load the scene as game has not begun running a scene yet
	return ApplyPendingScene();
}

bool SceneManager::Start(std::string_view sceneName) {
	const auto iterator = m_sceneNames.find(std::string(sceneName));

	if (iterator == m_sceneNames.end())
		return false;

	return Start(iterator->second);
}

bool SceneManager::LoadScene(SceneId sceneId) {
	if (!m_registrations.contains(sceneId))
		return false;

	// last valid request during a frame wins
	m_pendingSceneId = sceneId;

	return true;
}

bool SceneManager::LoadScene(std::string_view sceneName) {
	const auto iterator = m_sceneNames.find(std::string(sceneName));

	if (iterator == m_sceneNames.end())
		return false;

	return LoadScene(iterator->second);
}

void SceneManager::BeginFrame() {
	if (m_currentScene)
		m_currentScene->BeginFrame();
}

void SceneManager::FixedUpdate() {
	if (m_currentScene)
		m_currentScene->FixedUpdate();
}

void SceneManager::Update() {
	if (m_currentScene)
		m_currentScene->Update();
}

void SceneManager::LateUpdate() {
	if (m_currentScene)
		m_currentScene->LateUpdate();
}

void SceneManager::RenderWorld() {
	if (m_currentScene)
		m_currentScene->RenderWorld();
}

void SceneManager::RenderUI() {
	if (m_currentScene)
		m_currentScene->RenderUI();
}

void SceneManager::EndFrame() {
	if (m_currentScene)
		m_currentScene->EndFrame();

	ApplyPendingScene();
}

void SceneManager::Shutdown() {
	m_pendingSceneId.reset();

	if (m_currentScene) {
		m_currentScene->Unload();
		m_currentScene.reset();
	}

	m_currentSceneId = InvalidSceneId;
}

std::string_view SceneManager::GetCurrentSceneName() const noexcept {
	const auto iterator = m_registrations.find(m_currentSceneId);

	if (iterator == m_registrations.end())
		return {};

	return iterator->second.name;
}

std::array<float, 4> SceneManager::GetClearColor() const noexcept {
	if (!m_currentScene)
		return { 0.0f, 0.0f, 0.0f, 1.0f };
	return m_currentScene->GetClearColor();
}

bool SceneManager::ApplyPendingScene() {
	if (!m_pendingSceneId) return false;

	const SceneId nextSceneId = *m_pendingSceneId;

	m_pendingSceneId.reset();

	const auto registrationIterator = m_registrations.find(nextSceneId);

	if (registrationIterator == m_registrations.end())
		return false;

	/*
	* construct first. If construction fails, the current
	* scene has not yet been destroyed
	*/
	std::unique_ptr<Scene> nextScene = registrationIterator->second.factory(*this, m_context);

	if (!nextScene)
		throw std::runtime_error("Scene factory returned nullptr");

	if (m_currentScene)
		m_currentScene->Unload();

	// destroy current scene
	m_currentScene.reset();
	m_currentSceneId = InvalidSceneId;

	// point to next scene (also supports restarting the current scene)
	m_currentScene = std::move(nextScene);
	m_currentSceneId = nextSceneId;

	m_currentScene->Load();

	return true;
}
