#pragma once

#include "Scene.h"
#include "SceneTypes.h"

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <stdexcept>

class SceneManager final {
public:
	explicit SceneManager(SceneContext& context) noexcept;
	~SceneManager();

	// disallow to copy or move
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;
	SceneManager(SceneManager&&) = delete;
	SceneManager& operator=(SceneManager&&) = delete;

	template<typename T>
	void RegisterScene(SceneId sceneId, std::string sceneName) {
		static_assert(std::is_base_of_v<Scene, T>, "T must inherit from Scene");
		static_assert(std::is_constructible_v<T, SceneManager&, SceneContext&>, "Scene must be constructible with SceneManager& and SceneContext&");

		if (sceneId == InvalidSceneId)
			throw std::invalid_argument("InvalidSceneId cannot be registered");

		if (sceneName.empty())
			throw std::invalid_argument("Scene name cannot be empty");

		if (m_registrations.contains(sceneId))
			throw std::logic_error("A scene with this ID is already registered");

		if (m_sceneNames.contains(sceneName))
			throw std::logic_error("A scene with this name is already registered");

		SceneFactory factory = [](SceneManager& manager, SceneContext& context) {
			return std::make_unique<T>(manager, context);
			};

		m_sceneNames.emplace(sceneName, sceneId);
		m_registrations.emplace(sceneId, SceneRegistration{ std::move(sceneName),std::move(factory) });
	}

	/// <summary>
	/// Immediately loads the first scene
	/// </summary>
	/// <param name="sceneId"></param>
	/// <returns>bool</returns>
	bool Start(SceneId sceneId);
	bool Start(std::string_view sceneName);

	/// <summary>
	/// Requests a deferred transition
	/// </summary>
	/// <param name="sceneId"></param>
	/// <returns>bool</returns>
	bool LoadScene(SceneId sceneId);
	bool LoadScene(std::string_view sceneName);

	void BeginFrame();

	void FixedUpdate();
	void Update();
	void LateUpdate();

	void RenderWorld();
	void RenderUI();

	void EndFrame();

	void Shutdown();

	[[nodiscard]]
	Scene* GetCurrentScene() noexcept { return m_currentScene.get(); }

	[[nodiscard]]
	const Scene* GetCurrentScene() const noexcept { return m_currentScene.get(); }

	[[nodiscard]]
	SceneId GetCurrentSceneId() const noexcept { return m_currentSceneId; }

	[[nodiscard]]
	std::string_view GetCurrentSceneName() const noexcept;

	[[nodiscard]]
	bool IsTransitionPending() const noexcept { return m_pendingSceneId.has_value(); }

	[[nodiscard]]
	std::array<float, 4> GetClearColor() const noexcept;

private:
	using SceneFactory = std::function<std::unique_ptr<Scene>(SceneManager&, SceneContext&)>;

	struct SceneRegistration final {
		std::string name;
		SceneFactory factory;
	};

	bool ApplyPendingScene();

private:
	SceneContext& m_context;

	std::unordered_map<SceneId, SceneRegistration> m_registrations;
	std::unordered_map<std::string, SceneId> m_sceneNames;

	std::unique_ptr<Scene> m_currentScene;

	SceneId m_currentSceneId = InvalidSceneId;

	std::optional<SceneId> m_pendingSceneId;
};