#include <pch.h>

#include "WeaponController.h"
#include <Game/Gameplay/Weapon/Ammunition/Bullet.h>

#include <SNX/Core/Scene/Scene.h>
#include <SNX/Core/Components/Renderer/PrimitiveRenderer.h>
#include <SNX/Graphics/DeviceResources.h>
#include <SNX/Core/Components/Kinetics/KineticBody.h>
#include <SNX/Core/Components/Collider/SphereCollider.h>
#include <SNX/Core/Time.h>
#include <SNX/Core/Components/Renderer/TrailRenderer.h>

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
	bulletGO.AddComponent<SphereCollider>()
		.SetDetectionMode(CollisionDetectionMode::Continuous);

	auto& bulletTrail = bulletGO.AddComponent<TrailRenderer>();
	GradientKey bulletTracerGradientKeys[] = {
	{ 0.00f, { 1.00f, 0.95f, 0.70f, 0.00f } }, // transparent
	{ 0.05f, { 1.00f, 0.95f, 0.65f, 0.95f } }, // hot white-yellow
	{ 0.15f, { 1.00f, 0.75f, 0.20f, 1.00f } }, // yellow-orange
	{ 0.30f, { 1.00f, 0.30f, 0.02f, 0.95f } }, // orange
	{ 0.48f, { 0.90f, 0.08f, 0.01f, 0.70f } }, // red
	{ 0.68f, { 0.35f, 0.01f, 0.00f, 0.40f } }, // dark red
	{ 0.85f, { 0.08f, 0.00f, 0.00f, 0.12f } }, // faint red
	{ 1.00f, { 0.00f, 0.00f, 0.00f, 0.00f } }, // transparent
	};
	bulletTrail.GetGradient().SetKeys(bulletTracerGradientKeys);
	bulletTrail.SetTrailWidth(0.05f);

	auto& bullet = bulletGO.AddComponent<Bullet>();

	bulletGO.GetTransform().SetLocalScale({ 0.3f,0.3f,0.3f });
	bulletGO.GetTransform().SetPosition(spawnPosition);

	bulletRenderer.SetColor({ 0.90f, 0.08f, 0.01f, 1.0f });

	bullet.RequestLaunch(direction, m_muzzleSpeed);
}

void WeaponController::FireMissile(const Transform* target) {

}