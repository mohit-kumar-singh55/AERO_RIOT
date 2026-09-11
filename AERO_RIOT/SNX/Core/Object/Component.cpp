#include "pch.h"

#include "Component.h"
#include <SNX/Core/Object/GameObject.h>
#include <SNX/Core/Object/GameObjectManager.h>

Component::Component(GameObject& gameObject) noexcept :
	m_gameObject(&gameObject) {}

GameObject& Component::GetGameObject() const noexcept {
	return *m_gameObject;
}

Transform& Component::GetTransform() const noexcept {
	return m_gameObject->GetTransform();
}

Scene* Component::GetScene() noexcept {
	return GetGameObject().GetGameObjects()->GetScene();
}

const Scene* Component::GetScene() const noexcept {
	return GetGameObject().GetGameObjects()->GetScene();
}
