#include "pch.h"
#include "Collider.h"

#include <SNX/Core/Scene/Scene.h>
#include <SNX/Core/Object/GameObject.h>
#include <SNX/Core/Components/Kinetics/KineticBody.h>

void Collider::OnInitialize() {
	// register this component to Kinetics class
	GetScene()->GetKinetics()->RegisterCollider(this);
}

void Collider::OnDestroy() {
	// unregister this component from Kinetics class
	GetScene()->GetKinetics()->UnregisterCollider(this);
}

void Collider::OnStart() {
	// not throwing an error as kinetic body is not required on static objects
	m_kb = GetGameObject().GetComponent<KineticBody>();
}