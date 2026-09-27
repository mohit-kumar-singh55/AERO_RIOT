#include "pch.h"

#include "Kinetics.h"

#include <SNX/Core/Components/Kinetics/KineticBody.h>
#include <SNX/Core/Object/GameObject.h>
#include <SNX/Core/Collision/CollisionDetection.h>
#include <SNX/Core/Components/Collider/Collider.h>
#include <SNX/Core/Collision/Collision.h>

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

	// remove collider if it exists in previous collision set
	std::erase_if(m_previousCollisions,
		[collider](const CollisionPair& pair) {
			return pair.Contains(collider);
		});
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

void Kinetics::DetectCollision() {
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

			// check for collision
			if (CollisionDetection::Intersects(*col_A, *col_B)) {
				CollisionPair pair(col_A, col_B);

				m_currentCollisions.insert(pair);

				Collision collision_A(*col_B);
				Collision collision_B(*col_A);

				if (!m_previousCollisions.contains(pair)) {
					// notify on collision enter
					col_A->GetGameObject().NotifyCollisionEnter(collision_A);
					col_B->GetGameObject().NotifyCollisionEnter(collision_B);
				}
				else {
					// notify on collision stay
					col_A->GetGameObject().NotifyCollisionStay(collision_A);
					col_B->GetGameObject().NotifyCollisionStay(collision_B);
				}
			}
		}
	}

	/*
	* colliders that are in previous collision set
	* but not in current collision set
	* are the one exited the collision
	*/
	for (const auto& previousCol : m_previousCollisions) {
		if (!m_currentCollisions.contains(previousCol)) {
			Collision collision_A(*previousCol.b);
			Collision collision_B(*previousCol.a);

			// notify on collision exit
			previousCol.a->GetGameObject().NotifyCollisionExit(collision_A);
			previousCol.b->GetGameObject().NotifyCollisionExit(collision_B);
		}
	}

	m_previousCollisions.swap(m_currentCollisions);
	m_currentCollisions.clear();
}