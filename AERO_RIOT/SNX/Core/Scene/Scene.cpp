#include "Scene.h"
#include "SceneManager.h"

#include <SNX/Core/Time.h>
#include <SNX/Graphics/DeviceResources.h>

Scene::Scene(SceneManager& sceneManager, SceneContext& context) noexcept :
	m_sceneManager(sceneManager),
	m_context(context) {
	// initial render context setup
	auto& deviceResources = GetContext().deviceResources;
	m_renderContext.device = deviceResources.GetDevice();
	m_renderContext.deviceContext = deviceResources.GetContext();
	m_renderContext.viewportWidth = deviceResources.GetWidth();
	m_renderContext.viewportHeight = deviceResources.GetHeight();
}

void Scene::Load() {
	if (m_loaded) return;

	m_loaded = true;
	OnLoad();
}

void Scene::Unload() {
	if (!m_loaded) return;

	/*
	* allow the derived scene to release its
	* scene-specific non-gameobject resources first
	*/
	OnUnload();

	// all objects belonging to this scene disappear
	m_gameObjects.Clear();
	m_loaded = false;
}

void Scene::BeginFrame() {
	if (!m_loaded) return;

	m_gameObjects.BeginFrame();
}

void Scene::FixedUpdate() {
	if (!m_loaded) return;

	OnFixedUpdate();

	if (ShouldUpdateGameObjects())
		m_gameObjects.FixedUpdate();
}

void Scene::Update() {
	if (!m_loaded) return;

	OnUpdate();

	if (ShouldUpdateGameObjects())
		m_gameObjects.Update();
}

void Scene::LateUpdate() {
	if (!m_loaded) return;

	OnLateUpdate();

	if (ShouldUpdateGameObjects())
		m_gameObjects.LateUpdate();
}

void Scene::RenderWorld() {
	if (!m_loaded) return;

	// filling the remaining fields of render context that might change in every life cycle
	m_renderContext.fixedInterpolationAlpha = Time::FixedInterpolationAlpha();
	if (BuildRenderContext(m_renderContext))
		// render all the gameobjects of the current scene
		m_gameObjects.Render(m_renderContext);

	OnRenderWorld();
}

void Scene::RenderUI() {
	if (!m_loaded) return;

	OnRenderUI();
}

void Scene::EndFrame() {
	if (!m_loaded) return;

	m_gameObjects.EndFrame();
}

bool Scene::RequestSceneLoad(SceneId sceneId) {
	return m_sceneManager.LoadScene(sceneId);
}

bool Scene::RequestSceneLoad(std::string_view sceneName) {
	return m_sceneManager.LoadScene(sceneName);
}

void Scene::RequestQuit() const {
	m_context.RequestQuit();
}