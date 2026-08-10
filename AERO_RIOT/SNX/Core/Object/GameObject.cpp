#include "GameObject.h"

GameObject::GameObject(std::string name) :
	m_name(name) {
	m_transform.SetGameObject(this);
}

GameObject::~GameObject() {
	DestroyComponents();
}

void GameObject::Initialize() {
	if (m_initialized) return;

	for (const auto& component : m_components) {
		if (component->m_initialized)
			continue;

		component->OnInitialize();
		component->m_initialized = true;
	}

	m_initialized = true;
}

bool GameObject::IsActiveInHierarchy() const noexcept {
	if (!m_active || m_destroyRequested)
		return false;

	const Transform* parentTransform = m_transform.GetParent();

	while (parentTransform) {
		const GameObject* parentObject = parentTransform->GetGameObject();

		/*
		* every gameobject transform should have an owner
		* failing safely here prevents an invalid hierarchy
		* from continuing to update
		*/
		if (!parentObject || !parentObject->m_active || parentObject->m_destroyRequested)
			return false;

		parentTransform = parentTransform->GetParent();
	}

	return true;
}

void GameObject::EnsureComponentStarted(Component& component) {
	if (!component.m_enabled || component.m_started || component.m_removeRequested)
		return;

	component.OnStart();
	component.m_started = true;
}

void GameObject::RemoveRequestedComponents() noexcept {
	std::erase_if(
		m_components,
		[](const std::unique_ptr<Component>& component) {
			if (!component) return true;
			if (!component->m_removeRequested)
				return false;

			// if remove is requested but has not destryoed yet
			// so first call ondestroy of the component and then remove it
			if (!component->m_destroyed) {
				component->OnDestroy();
				component->m_destroyed = true;
			}

			return true;
		}
	);
}

void GameObject::FixedUpdate() {
	if (!IsActiveInHierarchy()) return;

	Initialize();

	for (const auto& component : m_components) {
		if (!component || !component->m_enabled || component->m_removeRequested)
			continue;

		EnsureComponentStarted(*component);

		if (component->m_removeRequested)
			continue;

		component->OnFixedUpdate();

		// ! stop immediately if destruction is requested
		if (m_destroyRequested)
			break;
	}
}

void GameObject::Update() {
	if (!IsActiveInHierarchy()) return;

	Initialize();

	for (const auto& component : m_components) {
		if (!component || !component->m_enabled || component->m_removeRequested)
			continue;

		EnsureComponentStarted(*component);

		if (component->m_removeRequested)
			continue;

		component->OnUpdate();

		// ! stop immediately if destruction is requested
		if (m_destroyRequested)
			break;
	}
}

void GameObject::LateUpdate() {
	if (!IsActiveInHierarchy()) return;

	for (const auto& component : m_components) {
		if (!component || !component->m_enabled || component->m_removeRequested)
			continue;

		component->OnLateUpdate();

		// ! stop immediately if destruction is requested
		if (m_destroyRequested)
			break;
	}
}

void GameObject::Render(const RenderContext& context) {
	if (!IsActiveInHierarchy()) return;

	for (const auto& component : m_components) {
		if (!component || !component->m_enabled || component->m_removeRequested)
			continue;

		component->OnRender(context);

		// ! stop immediately if destruction is requested
		if (m_destroyRequested)
			break;
	}
}

void GameObject::EndFrame() {
	RemoveRequestedComponents();
}

void GameObject::RequestDestroy() noexcept {
	if (m_destroyRequested) return;

	/*
	* mark this object first
	*
	* doing this before recursively visiting children
	* also prevents accidental recursion from returning
	* to this object in a corrupted hierarchy
	*/
	m_destroyRequested = true;

	Transform& transform = GetTransform();

	const std::size_t childCount = transform.GetChildCount();

	for (std::size_t index = 0;index < childCount;index++) {
		Transform* childTransform = transform.GetChild(index);

		if (!childTransform) continue;

		GameObject* childObject = childTransform->GetGameObject();

		if (!childObject) continue;

		/*
		* child objects will repeat this process
		* for their childs, till the very last object in hierarchy is marked for destruction
		*/
		childObject->RequestDestroy();
	}
}

void GameObject::DestroyComponents() noexcept {
	if (m_componentsDestroyed) return;

	// destroy in reverse order
	// as later components might be dependent on components added before them
	for (
		auto iterator = m_components.rbegin();
		iterator != m_components.rend();
		iterator++
		) {
		if (!(*iterator) || (*iterator)->m_destroyed)
			continue;

		(*iterator)->OnDestroy();
		(*iterator)->m_destroyed = true;
	}

	m_components.clear();

	m_componentsDestroyed = true;
}