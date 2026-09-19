#include "pch.h"

#include "KineticBody.h"

#include <SNX/Core/Components/Transform.h>
#include <SNX/Core/Scene/Scene.h>

#include <stdexcept>

void KineticBody::OnInitialize() {
	/*
	* a gameobject can have only a single kinetic body attached to it.
	* check if this gameobject has no other kinetic body already attached.
	* if so, don't allow to attach this one and remove itself
	*/
	auto* existing = GetGameObject().GetComponent<KineticBody>();
	if (existing && existing != this)
		throw std::runtime_error("KineticBody::OnInitialize(): A gameobject cannot have multiple KineticBody components.");

	// register this component to Kinetics class
	GetScene()->GetKinetics()->RegisterKineticBody(this);
}

void KineticBody::OnDestroy() {
	// unregister this component from Kinetics class
	GetScene()->GetKinetics()->UnregisterKineticBody(this);
}

void KineticBody::Integrate(float fixedDeltaTime) noexcept {
	auto& transform = GetTransform();
	auto position = transform.GetPosition();

	// ! linear
	// apply linear damping
	if (m_useLinearDamping) {
		const Vector3 dampingForce =
			-m_linearVelocity
			* m_mass
			* m_linearDamping;

		AddForce(dampingForce);
	}

	m_linearAcceleration = m_accumulatedForce * m_inverseMass;
	m_linearVelocity += m_linearAcceleration * fixedDeltaTime;

	// clamp to max linear velocity
	if (m_maxLinearVelocity > 0.0f) {
		auto clampedLinearVel = DirectX::XMVector3ClampLength(m_linearVelocity, 0.0f, m_maxLinearVelocity);
		m_linearVelocity = Vector3(clampedLinearVel);
	}

	position += m_linearVelocity * fixedDeltaTime;
	GetTransform().SetPosition(position);

	// ! angular
	/*
	* because moi is in local space
	* we first need to convert torque into local space,
	* then calc. local angular acc. and convert it back to world space
	*/
	Quaternion worldRotation = transform.GetRotation();
	Quaternion inverseRotation;
	worldRotation.Inverse(inverseRotation);

	// apply angular damping
	if (m_useAngularDamping) {
		const Vector3 localAngularVelocity = Vector3::Transform(m_angularVelocity, inverseRotation);
		const Vector3 localDampingTorque = -localAngularVelocity * m_angularDamping;
		const Vector3 worldDampingTorque = Vector3::Transform(localDampingTorque, worldRotation);
		AddTorque(worldDampingTorque);	// add the damping torque to the accumulated torque
	}

	const Vector3 localTorque = Vector3::Transform(m_accumulatedTorque, inverseRotation);
	const Vector3 localAngularAcc = localTorque * m_inverseMomentOfInertia;

	// convert back to world angular acc.
	m_angularAcceleration = Vector3::Transform(localAngularAcc, worldRotation);
	m_angularVelocity += m_angularAcceleration * fixedDeltaTime;

	float angularVelocitySquared = m_angularVelocity.Dot(m_angularVelocity);
	if (angularVelocitySquared >= 0.000001f) {
		float angularSpeed = std::sqrt(angularVelocitySquared);
		float angle = angularSpeed * fixedDeltaTime;
		Vector3 rotationAxis = m_angularVelocity / angularSpeed;	// normalize
		Quaternion deltaRotation = Quaternion::CreateFromAxisAngle(rotationAxis, angle);
		transform.SetRotation(Quaternion::Concatenate(deltaRotation, transform.GetRotation()));
	}

	// ! clear the accumulated forces
	m_accumulatedForce = Vector3::Zero;
	m_accumulatedTorque = Vector3::Zero;
}