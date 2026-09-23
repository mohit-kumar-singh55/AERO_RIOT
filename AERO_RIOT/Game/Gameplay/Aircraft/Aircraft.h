#pragma once

#include "AircraftControlInput.h"

#include <SNX/Core/Object/Component.h>

#include <Game/Gameplay/Weapon/WeaponType.h>

#include <SimpleMath.h>

class KineticBody;
class AircraftKinetics;
class WeaponController;

class Aircraft final : public Component {
public:
	using Component::Component;

	void SetControlInput(AircraftControlInput controlInput) noexcept {
		m_controlInput = controlInput.GetNormalized();
	}

	void Fire(const WeaponType weaponType) const noexcept;

protected:
	void OnInitialize() override;

	void OnStart() override;

	void OnFixedUpdate() override;
	void OnLateUpdate() override;

private:
	void PerformEvadeRoll() noexcept;

private:
	AircraftControlInput m_controlInput;
	Transform* m_aircraftBody = nullptr;		// visual child	
	AircraftKinetics* m_aircraftKinetics = nullptr;
	WeaponController* m_weaponController;

	float m_rotationSpeed = 60.0f;	// degree/s

	float m_evadeRollDuration = 0.3f;
	float m_evadeRollAngle = 360.0f;	// degrees
	float m_evadeDistance = 5.0f;		// amount of displacement when evade rolling

	// evade roll purpose **
	bool m_isEvadeRolling = false;
	EvadeRoll m_currentRollingDir = EvadeRoll::None;
	DirectX::SimpleMath::Quaternion m_startRotation;
	DirectX::SimpleMath::Vector3 m_startRight;
	float m_evadeRollElapsedTime = 0.0f;
	float m_previousDisplaceOffset = 0.0f;
	// *********************
};