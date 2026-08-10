#pragma once

#include <SNX/Core/Object/GameObject.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct RenderContext;

class GameObjectManager final {
public:
	GameObjectManager() = default;
	~GameObjectManager();

	// disallow to copy or move
	GameObjectManager(const GameObjectManager&) = delete;
	GameObjectManager& operator=(const GameObjectManager&) = delete;
	GameObjectManager(GameObjectManager&&) = delete;
	GameObjectManager& operator=(GameObjectManager&&) = delete;

	GameObject& CreateGameObject(std::string name = "GameObject");

	// called once at the beginning of a rendered frame
	void BeginFrame();

	void FixedUpdate();
	void Update();
	void LateUpdate();
	void Render(const RenderContext& context);

	// called after rendering
	void EndFrame();

	void Destroy(GameObject& gameObject) noexcept;
	void DestroyAll() noexcept;

	// ! immediately removes everything
	// ! use when unloading a game or scene
	void Clear() noexcept;

	[[nodiscard]]
	GameObject* Find(std::string_view name)  noexcept;

	[[nodiscard]]
	const GameObject* Find(std::string_view name) const noexcept;

	std::size_t GetObjectCount() const noexcept {
		return m_objects.size() + m_pendingObjects.size();
	}

private:
	void AddPendingObjects();
	void RemoveDestroyedObjects() noexcept;

private:
	std::vector<std::unique_ptr<GameObject>> m_objects;
	std::vector<std::unique_ptr<GameObject>> m_pendingObjects;
};