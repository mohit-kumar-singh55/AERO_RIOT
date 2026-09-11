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
	m_linearAcceleration = m_accumulatedForce * m_inverseMass;
	m_linearVelocity += m_linearAcceleration * fixedDeltaTime;

	auto& transform = GetTransform();
	auto position = transform.GetPosition();

	position += m_linearVelocity * fixedDeltaTime;
	GetTransform().SetPosition(position);

	// clear the accumulated forces
	m_accumulatedForce = Vector3::Zero;
}