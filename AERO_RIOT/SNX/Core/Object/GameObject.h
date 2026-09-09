#pragma once

#include <SNX/Core/Object/Component.h>
#include <SNX/Core/Components/Transform.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class Scene;
struct RenderContext;

class GameObject final {
public:
	explicit GameObject(Scene* scene, std::string name = "GameObject");
	~GameObject();

	// disallow to copy or move
	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;
	GameObject(GameObject&&) = delete;
	GameObject& operator=(GameObject&&) = delete;

	template<typename T, typename... Arguments>
	T& AddComponent(Arguments&&... arguments) {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		auto component = std::make_unique<T>(*this, std::forward<Arguments>(arguments)...);

		T& componentReference = *component;

		m_components.push_back(std::move(component));

		if (m_initialized) {
			/*
			* access through Component instead of the derived type
			* (as GameObject class is the friend of Component Class only, not its derived classes
			* so, GameObject cannot directly access private and protected members of child classes of Component class)
			* OnInitialize is virtual, so the derived implementation
			* will still be called
			*/
			Component& baseComponent = *m_components.back();

			baseComponent.OnInitialize();
			baseComponent.m_initialized = true;
		}

		return componentReference;
	}

	template<typename T>
	[[nodiscard]]
	T* GetComponent() noexcept {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		for (const auto& component : m_components) {
			if (!component || component->IsRemoveRequested())
				continue;

			if (T* result = dynamic_cast<T*>(component.get()))
				return result;
		}

		return nullptr;
	}

	template<typename T>
	bool RemoveComponent() noexcept {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		for (const auto& component : m_components) {
			if (!component || component->IsRemoveRequested())
				continue;

			if (dynamic_cast<T*>(component.get())) {
				component->RequestRemove();
				return true;
			}
		}

		return false;
	}

	bool RemoveComponent(Component& target) noexcept {
		for (const auto& component : m_components) {
			if (component.get() != &target || component->IsRemoveRequested())
				continue;

			component->RequestRemove();
			return true;
		}

		return false;
	}

	void Initialize();

	void FixedUpdate();
	void Update();
	void LateUpdate();
	void Render(const RenderContext& context);

	void EndFrame();

	/// <summary>
	/// Delete this gameobject and the whole hierarchy below it
	/// </summary>
	void RequestDestroy() noexcept;

	[[nodiscard]]
	bool IsDestroyRequested() const noexcept { return m_destroyRequested; }

	[[nodiscard]]
	bool IsActiveSelf() const noexcept { return m_active; }

	[[nodiscard]]
	bool IsActiveInHierarchy() const noexcept;

	void SetActive(bool active) noexcept { m_active = active; }

	[[nodiscard]]
	const std::string& GetName() const noexcept { return m_name; }

	[[nodiscard]]
	Transform& GetTransform()  noexcept { return m_transform; }
	[[nodiscard]]
	const Transform& GetTransform() const noexcept { return m_transform; }

	// get current scene
	[[nodiscard]]
	Scene* GetScene() noexcept { return m_scene; }
	[[nodiscard]]
	const Scene* GetScene() const noexcept { return m_scene; }

private:
	void EnsureComponentStarted(Component& component);

	void RemoveRequestedComponents() noexcept;

	void DestroyComponents() noexcept;

private:
	std::string m_name;

	Scene* m_scene;

	Transform m_transform;

	std::vector<std::unique_ptr<Component>> m_components;

	bool m_initialized = false;
	bool m_active = true;
	bool m_destroyRequested = false;
	bool m_componentsDestroyed = false;
};