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

Vector3 Collider::GetCenter() const noexcept {
	/*
	* as m_offset is in local-space,
	* convert to world-space to apply automatically apply
	* scale, rotation and translation
	*/
	return Vector3::Transform(
		m_offset,
		GetTransform().GetWorldMatrix()
	);
}

Vector3 Collider::GetPreviousCenter() const noexcept {
	if (!m_kb) return GetCenter();

	using DirectX::SimpleMath::Matrix;

	Matrix previousMatrix =
		Matrix::CreateScale(GetTransform().GetScale())
		* Matrix::CreateFromQuaternion(m_kb->GetPreviousRotation())
		* Matrix::CreateTranslation(m_kb->GetPreviousPosition());

	// convert to world-space
	return Vector3::Transform(
		m_offset,
		previousMatrix
	);
}