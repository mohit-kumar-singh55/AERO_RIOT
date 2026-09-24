#include "pch.h"

#include "AircraftKinetics.h"

#include <SNX/Core/Object/GameObject.h>
#include <SNX/Core/Components/Kinetics/KineticBody.h>

#include <stdexcept>

void AircraftKinetics::OnInitialize() {
	m_kb = GetGameObject().GetComponent<KineticBody>();

	if (!m_kb)
		throw std::runtime_error("AircraftKinetics::OnInitialize: Cannot find KineticBody component.");

	// not using the default damping provided by the physics engine
	m_kb->SetUseLinearDamping(false);
	m_kb->SetUseAngularDamping(false);
	m_kb->SetMaxLinearVelocity(m_maxLinearVelocity);
}

void AircraftKinetics::Apply(const AircraftControlInput& controlInput) noexcept {
	const auto& transform = GetTransform();

	Quaternion localRotation;
	transform.GetRotation().Inverse(localRotation);

	// convert world angular vel. to local-space
	const Vector3 localAngularVelocity = Vector3::Transform(m_kb->GetAngularVelocity(), localRotation);

	// ! apply rotation
	float currentPitchRate = localAngularVelocity.x;
	float currentYawRate = -localAngularVelocity.y;

	float targetPitchRate = controlInput.pitch * m_maxPitchRate;
	float pitchError = targetPitchRate - currentPitchRate;

	float targetYawRate = controlInput.turn * m_maxYawRate;
	float yawError = targetYawRate - currentYawRate;

	const Vector3 localTorque = {
	   std::clamp(m_pitchRateKp * pitchError, -m_maxPitchTorque, m_maxPitchTorque),
	   -std::clamp(m_pitchYawKp * yawError, -m_maxYawTorque, m_maxYawTorque),
	   0
	};
	// convert to world-space
	const Vector3 worldTorque = Vector3::Transform(localTorque, transform.GetRotation());
	m_kb->AddTorque(worldTorque);

	// ! calc. angular damping
	const float dynamicPressure =
		0.5f
		* m_airDensity
		* m_kb->GetLinearVelocity().LengthSquared();

	const Vector3 localDampingTorque =
		-Vector3(m_pitchDamping, m_yawDamping, m_rollDamping)
		* localAngularVelocity
		* dynamicPressure;

	// convert to world-space
	const Vector3 worldDampingTorque = Vector3::Transform(localDampingTorque, transform.GetRotation());
	m_kb->AddTorque(worldDampingTorque);

	// ! bank-assist (auto-level)
	auto bank = CalculateBankAngle();
	/*
	* skip bank assist if aircraft up is
	* perpendicular or opposite to the world up
	* it is required to stop assist, otherwise
	* aircraft wouldn't be able to perform backflip
	*/
	//bool shouldBankAssist = transform.GetUp().Dot(Vector3::Up) > 0;

	//if (bank && shouldBankAssist) {
	if (bank) {
		float turn = controlInput.turn;
		float currentBankAngle = *bank;
		float targetBankAngle = turn * m_maxBankAngle;
		float bankError = std::atan2(
			std::sin(targetBankAngle - currentBankAngle),
			std::cos(targetBankAngle - currentBankAngle)
		);
		float bankRate = -localAngularVelocity.z;

		// PD controller (roll torque to level the aircraft)
		float rollCommand = m_bankKp * bankError - m_bankKd * bankRate;
		const Vector3 localRollTorque = { 0.0f,0.0f,-rollCommand };
		const Vector3 worldRollTorque = Vector3::Transform(localRollTorque, transform.GetRotation());

		m_kb->AddTorque(worldRollTorque);
	}

	// ! apply thrust
	const Vector3 thrust =
		transform.GetForward()
		* m_maxThrust
		* controlInput.throttle;

	m_kb->AddForce(thrust);

	// ! calc. directional aerodynamic drag
	const Vector3 velocity = m_kb->GetLinearVelocity();

	const Vector3 aircraftForward = transform.GetForward();
	const Vector3 aircraftRight = transform.GetRight();
	const Vector3 aircraftUp = transform.GetUp();

	const float forwardSpeed = velocity.Dot(aircraftForward);
	const float sideSpeed = velocity.Dot(aircraftRight);
	const float verticalSpeed = velocity.Dot(aircraftUp);

	const float effectiveForwardDrag = std::lerp(m_glideForwardDrag, m_normalForwardDrag, controlInput.throttle);

	const Vector3 forwardDrag =
		-aircraftForward
		//* m_normalForwardDrag
		* effectiveForwardDrag
		* forwardSpeed
		* std::abs(forwardSpeed);

	const Vector3 sideDrag =
		-aircraftRight
		* m_sideDrag
		* sideSpeed
		* std::abs(sideSpeed);

	const Vector3 verticalDrag =
		-aircraftUp
		* m_verticalDrag
		* verticalSpeed
		* std::abs(verticalSpeed);

	Vector3 totalDrag = forwardDrag + sideDrag + verticalDrag;

	// ? TEMP
	// TODO: calc. proper airbrake drag
	totalDrag *= 1.0f + controlInput.airBrake * m_airBrakePower;

	m_kb->AddForce(totalDrag);

	// ! calc. lift force
	float liftForce = 0.0f;
	Vector3 liftDir = Vector3::Zero;
	float pitchSpeedSquared = forwardSpeed * forwardSpeed + verticalSpeed * verticalSpeed;

	if (pitchSpeedSquared <= 0.000001f)
		liftForce = 0.0f;
	else {
		float angleOfAttack = -std::atan2(verticalSpeed, forwardSpeed);
		float liftCoef = CalculateLiftCoefficient(angleOfAttack);

		liftForce =
			0.5f
			* m_airDensity
			* pitchSpeedSquared
			* m_wingArea
			* liftCoef;

		float maxLiftForce = m_kb->GetMass() * 40.0f;
		liftForce = std::clamp(liftForce, -maxLiftForce, maxLiftForce);

		// calc. lift dir
		const Vector3 pitchVelocity = aircraftForward * forwardSpeed + aircraftUp * verticalSpeed;
		liftDir = aircraftRight.Cross(pitchVelocity);
		liftDir.Normalize();
	}

	m_kb->AddForce(liftDir * liftForce);

	// ! speed-assist
	float speedDeficit = m_minForwardSpeed - forwardSpeed;
	float assistAcceleration = speedDeficit * m_minSpeedGain;
	assistAcceleration = std::clamp(assistAcceleration, 0.0f, m_maxSpeedAssistAcceleration);
	auto speedAssistForce = aircraftForward * assistAcceleration * m_kb->GetMass();
	m_kb->AddForce(speedAssistForce);
}

float AircraftKinetics::CalculateLiftCoefficient(const float angleOfAttack) const noexcept {
	const float absAoA = std::abs(angleOfAttack);

	const float maxAngle = DirectX::g_XMHalfPi.f[0];	// 90.0f

	if (absAoA <= m_stallAngle)
		return m_liftSlope * angleOfAttack;
	else if (absAoA > m_stallAngle && absAoA < maxAngle) {
		float maxLiftCoef = m_liftSlope * m_stallAngle;
		float postStallT = (absAoA - m_stallAngle) / (maxAngle - m_stallAngle);
		float liftRemaining = 1 - postStallT;
		return (angleOfAttack >= 0.0f ? 1.0f : -1.0f) * maxLiftCoef * liftRemaining;
	}

	// above 90.0f
	return 0.0f;
}

std::optional<float> AircraftKinetics::CalculateBankAngle() const noexcept {
	const auto& transform = GetTransform();

	const auto up = transform.GetUp();
	const auto forward = transform.GetForward();

	// up dir of the plane we want the aircraft to be leveled to
	auto levelUp =
		Vector3::Up
		- forward
		* forward.Dot(Vector3::Up);

	if (levelUp.LengthSquared() < 0.01f)
		return std::nullopt;

	levelUp.Normalize();

	float bank = std::atan2(
		forward.Dot(levelUp.Cross(up)),
		levelUp.Dot(up)
	);

	return bank;
}