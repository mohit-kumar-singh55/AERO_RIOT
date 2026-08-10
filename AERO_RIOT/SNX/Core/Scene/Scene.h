#pragma once

#include "SceneTypes.h"
#include "SceneContext.h"

#include <SNX/Graphics/RenderContext.h>
#include <SNX/Core/Object/GameObjectManager.h>

#include <array>
#include <string_view>

class SceneManager;

class Scene {
public:
	Scene(SceneManager& sceneManager, SceneContext& context) noexcept;
	virtual ~Scene() = default;

	// disallow to copy or move
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;
	Scene(Scene&&) = delete;
	Scene& operator=(Scene&&) = delete;

	void Load();
	void Unload();

	void BeginFrame();

	void FixedUpdate();
	void Update();
	void LateUpdate();

	void RenderWorld();
	void RenderUI();

	void EndFrame();

	[[nodiscard]]
	bool IsLoaded() const noexcept { return m_loaded; }

	[[nodiscard]]
	std::array<float, 4> GetClearColor() const noexcept { return OnGetClearColor(); }

protected:
	[[nodiscard]]
	SceneManager& GetSceneManager() noexcept { return m_sceneManager; }
	[[nodiscard]]
	const SceneManager& GetSceneManager() const noexcept { return m_sceneManager; }

	[[nodiscard]]
	SceneContext& GetContext() noexcept { return m_context; }
	[[nodiscard]]
	const SceneContext& GetContext() const noexcept { return m_context; }

	[[nodiscard]]
	GameObjectManager& GetGameObjects() noexcept { return m_gameObjects; }
	[[nodiscard]]
	const GameObjectManager& GetGameObjects() const noexcept { return m_gameObjects; }

	bool RequestSceneLoad(SceneId sceneId);
	bool RequestSceneLoad(std::string_view sceneName);

	void RequestQuit() const;

protected:
	virtual void OnLoad() {}

	virtual void OnUnload() {}

	virtual void OnFixedUpdate() {}

	virtual void OnUpdate() {}

	virtual void OnLateUpdate() {}

	/*
	* Some camera related fields will be filled in the render context in every 3D
	*
	* By default returns false
	* because scenes with no 3D object, doesn't require a 3D camera as well
	* so those (2D) scenes will not need to implement this method
	*/
	[[nodiscard]]
	virtual bool BuildRenderContext(RenderContext& context) const noexcept { return false; }

	virtual void OnRenderWorld() {}

	virtual void OnRenderUI() {}

	[[nodiscard]]
	virtual bool ShouldUpdateGameObjects() const noexcept { return true; }

	[[nodiscard]]
	virtual std::array<float, 4> OnGetClearColor() const noexcept {
		return { 0.08f, 0.08f, 0.10f, 1.0f };
	}

private:
	SceneManager& m_sceneManager;
	SceneContext& m_context;

	RenderContext m_renderContext;

	GameObjectManager m_gameObjects;

	bool m_loaded = false;
};