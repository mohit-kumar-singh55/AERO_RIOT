#include "pch.h"

#include "AircraftController.h"
#include "Aircraft.h"

#include <SNX/Core/Object/GameObject.h>
#include <SNX/Input/InputManager.h>

#include <Game/Gameplay/Weapon/WeaponType.h>

#include <stdexcept>

void AircraftController::OnInitialize() {
	m_aircraft = GetGameObject().GetComponent<Aircraft>();

	if (!m_aircraft)
		throw std::runtime_error("AircraftController: Cannot find Aircraft Component.");
}

void AircraftController::OnUpdate() {
	auto& input = InputManager::Get();
	AircraftControlInput controlInput{};

	// ! setting inputs
	controlInput.throttle = input.GetGamePadTrigger(GamePadTrigger::Right);
	controlInput.airBrake = input.GetGamePadTrigger(GamePadTrigger::Left);

	auto rotVal = input.GetGamePadStick(GamePadStick::LeftStick);

	if (std::abs(rotVal.x) < m_stickMinThreshold)
		rotVal.x = 0.0f;
	if (std::abs(rotVal.y) < m_stickMinThreshold)
		rotVal.y = 0.0f;

	controlInput.pitch = -rotVal.y;
	controlInput.turn = rotVal.x;

	if (input.IsGamePadButtonPressed(GamePadButton::DPadLeft))
		controlInput.evadeRoll = EvadeRoll::Left;
	else if (input.IsGamePadButtonPressed(GamePadButton::DPadRight))
		controlInput.evadeRoll = EvadeRoll::Right;

	// transfer inputs to the aircraft
	m_aircraft->SetControlInput(controlInput);

	// ! check for firing input
	if (input.IsGamePadButtonDown(GamePadButton::LeftShoulder))
		m_aircraft->Fire(WeaponType::Gun);
	if (input.IsGamePadButtonPressed(GamePadButton::RightShoulder))
		m_aircraft->Fire(WeaponType::Missile);
}

void AircraftController::OnDestroy() {
	m_aircraft = nullptr;
}