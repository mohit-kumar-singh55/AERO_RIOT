#include <pch.h>

#include "WeaponController.h"
#include <Game/Gameplay/Weapon/Ammunition/Bullet.h>

#include <SNX/Core/Scene/Scene.h>
#include <SNX/Core/Components/Renderer/PrimitiveRenderer.h>
#include <SNX/Graphics/DeviceResources.h>
#include <SNX/Core/Components/Kinetics/KineticBody.h>
#include <SNX/Core/Time.h>

#include <stdexcept>

WeaponController::WeaponController(
	GameObject& gameObject,
	Transform* gunMuzzle
) : Component(gameObject) {
	if (!gunMuzzle)
		throw std::invalid_argument("WeaponController::WeaponController: GunMuzzle is invalid.");

	m_gunMuzzle = gunMuzzle;
}

void WeaponController::OnUpdate() {
	if (m_gunFireTimer > 0.0f)
		m_gunFireTimer -= Time::DeltaTime();
}

void WeaponController::TryFire(const WeaponType weaponType) {
	// TODO: check if the requested weapon can be fired
	switch (weaponType)
	{
	case WeaponType::Gun: {
		if (m_gunFireTimer <= 0.0f) {
			FireGun(m_gunMuzzle->GetPosition(), m_gunMuzzle->GetForward());
			m_gunFireTimer = m_gunFireInterval;
		}

		break;
	}
	case WeaponType::Missile: {
		//FireMissile(nullptr);
		break;
	}
	default:
		break;
	}

	// TODO: fire the requested weapon
}

void WeaponController::FireGun(
	const DirectX::SimpleMath::Vector3 spawnPosition,
	const DirectX::SimpleMath::Vector3 direction
) {
	auto& bulletGO = GetScene()->GetGameObjects().CreateGameObject("Bullet");
	auto& bulletRenderer = bulletGO.AddComponent<PrimitiveRenderer>(
		GetScene()->GetContext().deviceResources.GetContext(),
		PrimitiveShape::Sphere
	);
	bulletGO.AddComponent<KineticBody>();
	auto& bullet = bulletGO.AddComponent<Bullet>();

	bulletGO.GetTransform().SetPosition(spawnPosition);

	bulletRenderer.SetColor({ 0.5f,0.9f,0.3f,1.0f });

	bullet.Launch(direction, m_muzzleSpeed);
}

void WeaponController::FireMissile(const Transform* target) {

}