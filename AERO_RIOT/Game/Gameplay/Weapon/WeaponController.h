#pragma once

#include <SNX/Core/Object/Component.h>

#include <SimpleMath.h>

#include "WeaponType.h"

class Transform;

class WeaponController final : public Component {
public:
	WeaponController(GameObject& gameObject, Transform* gunMuzzle);

	void TryFire(const WeaponType weaponType);

protected:
	void OnUpdate() override;

private:
	void FireGun(
		const DirectX::SimpleMath::Vector3 spawnPosition,
		const DirectX::SimpleMath::Vector3 direction
	);

	// homing missile
	void FireMissile(const Transform* target);

private:
	Transform* m_gunMuzzle = nullptr;

	float m_gunFireInterval = 0.1f;
	float m_muzzleSpeed = 150.0f;	// bullet speed

	float m_gunFireTimer = 0.0f;
};