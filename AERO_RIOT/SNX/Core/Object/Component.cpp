#include "Component.h"
#include <SNX/Core/Object/GameObject.h>

Component::Component(GameObject& gameObject) noexcept :
	m_gameObject(&gameObject) {}

GameObject& Component::GetGameObject() const noexcept {
	return *m_gameObject;
}

Transform& Component::GetTransform() const noexcept {
	return m_gameObject->GetTransform();
}
