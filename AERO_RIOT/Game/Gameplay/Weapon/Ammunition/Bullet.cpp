#include <pch.h>

#include "Bullet.h"

#include <SNX/Core/Components/Kinetics/KineticBody.h>
#include <SNX/Core/Object/GameObject.h>
#include <SNX/Core/Time.h>

void Bullet::OnStart() {
	m_kb = GetGameObject().GetComponent<KineticBody>();

	if (!m_kb)
		throw std::runtime_error("Bullet::OnStart: Cannot find KineticBody component.");

	m_kb->SetUseGravity(false);
	m_kb->SetUseLinearDamping(false);
	m_kb->SetUseAngularDamping(false);
}

void Bullet::OnUpdate() {
	if (m_lifeTimeTimer > 0.0f) {
		m_lifeTimeTimer -= Time::DeltaTime();

		if (m_lifeTimeTimer <= 0.0f)
			RequestRemove();
	}
}

void Bullet::Launch(
	const DirectX::SimpleMath::Vector3 direction,
	float speed
) {
	m_lifeTimeTimer = m_lifeTime;
	m_kb->SetLinearVelocity(direction * speed);
}