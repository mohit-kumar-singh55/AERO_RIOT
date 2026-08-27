#include "Aircraft.h"
#include "AircraftController.h"

#include <SNX/Core/Object/GameObject.h>
#include <SNX/Input/InputManager.h>

#include <stdexcept>

void AircraftController::OnInitialize() {
	m_aircraft = GetGameObject().GetComponent<Aircraft>();

	if (!m_aircraft)
		throw std::runtime_error("AircraftController: Cannot find Aircraft Component.");
}

void AircraftController::OnUpdate() {
	auto& input = InputManager::Get();
	AircraftControlInput controlInput{};

	// setting inputs
	controlInput.throttle = input.GetGamePadTrigger(GamePadTrigger::Right);
	controlInput.airBrake = input.GetGamePadTrigger(GamePadTrigger::Left);

	auto rotVal = input.GetGamePadStick(GamePadStick::LeftStick);
	controlInput.pitch = rotVal.y;
	controlInput.turn = rotVal.x;

	if (input.IsGamePadButtonPressed(GamePadButton::DPadLeft))
		controlInput.evadeRoll = EvadeRoll::Left;
	else if (input.IsGamePadButtonPressed(GamePadButton::DPadRight))
		controlInput.evadeRoll = EvadeRoll::Right;

	// transfer inputs to the aircraft
	m_aircraft->SetControlInput(controlInput);
}

void AircraftController::OnDestroy() {
	m_aircraft = nullptr;
}