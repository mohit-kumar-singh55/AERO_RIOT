#include "pch.h"

#include "Kinetics.h"

#include <SNX/Core/Components/Kinetics/KineticBody.h>
#include <SNX/Core/Object/GameObject.h>
#include <SNX/Core/Components/Collider/CollisionDetection.h>

#include <SNX/Core/Debugger/Debug.h>
#include <SNX/Utils/Conversion.h>

Kinetics::~Kinetics() {
	m_kineticBodies.clear();
}

void Kinetics::RegisterKineticBody(KineticBody* body) {
	m_kineticBodies.push_back(body);
}

void Kinetics::UnregisterKineticBody(KineticBody* body) {
	std::erase(m_kineticBodies, body);
}

void Kinetics::RegisterCollider(Collider* collider) {
	m_colliders.push_back(collider);
}

void Kinetics::UnregisterCollider(Collider* collider) {
	std::erase(m_colliders, collider);
}

void Kinetics::Integrate(float fixedDeltaTime) noexcept {
	for (const auto body : m_kineticBodies) {
		// no need to apply physics for the component which is inactive or about to be removed
		if (!body ||
			!body->IsEnabled() ||
			body->IsRemoveRequested() ||
			!body->GetGameObject().IsActiveInHierarchy())
			continue;

		// apply gravity
		if (body->GetUseGravity())
			body->AddForce(body->GetMass() * m_gravity * body->GetGravityScale());

		body->Integrate(fixedDeltaTime);
	}
}

void Kinetics::UpdateInterpolation(float alpha) noexcept {
	for (const auto body : m_kineticBodies) {
		// check if inactive or about to be removed
		if (!body ||
			!body->IsEnabled() ||
			body->IsRemoveRequested() ||
			!body->GetGameObject().IsActiveInHierarchy())
			continue;


		body->UpdateInterpolation(alpha);
	}
}

void Kinetics::DetectCollision() noexcept {
	for (size_t i = 0; i < m_colliders.size(); i++) {
		for (size_t j = i + 1; j < m_colliders.size(); j++) {
			auto col_A = m_colliders[i];
			auto col_B = m_colliders[j];

			// no need to detect collision for the component which is inactive or about to be removed
			if (!col_A || !col_B ||
				!col_A->IsEnabled() || !col_B->IsEnabled() ||
				col_A->IsRemoveRequested() || col_B->IsRemoveRequested() ||
				!col_A->GetGameObject().IsActiveInHierarchy() || !col_B->GetGameObject().IsActiveInHierarchy())
				continue;

			// skip if both are static objects
			if (!col_A->GetKineticBody()
				&& !col_B->GetKineticBody())
				continue;

			if (CollisionDetection::Intersects(*col_A, *col_B))
				Debug::LogWarning(
					Utils::Conversion::ToWString(col_A->GetGameObject().GetName())
					+ L" Collided with " +
					Utils::Conversion::ToWString(col_B->GetGameObject().GetName()),
					5.0f
				);
		}
	}
}