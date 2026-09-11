#include "pch.h"

#include "Kinetics.h"

#include <SNX/Core/Components/Kinetics/KineticBody.h>
#include <SNX/Core/Object/GameObject.h>

Kinetics::~Kinetics() {
	m_kineticBodies.clear();
}

void Kinetics::RegisterKineticBody(KineticBody* body) {
	m_kineticBodies.push_back(body);
}

void Kinetics::UnregisterKineticBody(KineticBody* body) {
	std::erase(m_kineticBodies, body);
}

void Kinetics::Integrate(float fixedDeltaTime) noexcept {
	for (const auto body : m_kineticBodies) {
		// no need to apply physics for the component which is inactive or about to be removed
		if (!body ||
			!body->IsEnabled() ||
			body->IsRemoveRequested() ||
			!body->GetGameObject().IsActiveInHierarchy())
			continue;

		body->Integrate(fixedDeltaTime);
	}
}