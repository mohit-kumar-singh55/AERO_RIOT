#pragma once

class GameObject;
class Transform;

struct RenderContext;

class Component {
public:
	explicit Component(GameObject& gameObject) noexcept;
	virtual ~Component() = default;

	// disallow to copy or move
	Component(const Component&) = delete;
	Component& operator=(const Component&) = delete;
	Component(Component&&) = delete;
	Component& operator=(Component&&) = delete;

	[[nodiscard]]
	GameObject& GetGameObject() const noexcept;

	[[nodiscard]]
	Transform& GetTransform() const noexcept;

	[[nodiscard]]
	bool IsEnabled() const noexcept { return m_enabled; }

	void SetEnabled(bool enabled) noexcept { m_enabled = enabled; }

	void RequestRemove() noexcept { m_removeRequested = true; }

	[[nodiscard]]
	bool IsRemoveRequested() const noexcept { return m_removeRequested; }

protected:
	// called once after the component is created
	virtual void OnInitialize() {}

	// called once before its first update
	virtual void OnStart() {}

	// called at the fixed simulation frequency
	virtual void OnFixedUpdate() {}

	// called once per rendered frame
	virtual void OnUpdate() {}

	// called after all normal updates
	virtual void OnLateUpdate() {}

	// temporary rendering callback
	virtual void OnRender(const RenderContext& context) {}

	// called before the component is removed
	virtual void OnDestroy() {}

private:
	GameObject* m_gameObject = nullptr;

	bool m_enabled = true;
	bool m_initialized = false;
	bool m_started = false;

	bool m_removeRequested = false;
	bool m_destroyed = false;

	friend class GameObject;
};