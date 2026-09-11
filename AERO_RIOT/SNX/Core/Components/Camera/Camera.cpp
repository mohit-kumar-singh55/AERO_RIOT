#include "pch.h"

#include "Camera.h"

#include <SNX/Core/Components/Transform.h>

void Camera::SetPerspective(
	float fovDegrees,
	float aspectRatio,
	float nearPlane,
	float farPlane
) noexcept {
	m_projection = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
		DirectX::XMConvertToRadians(fovDegrees),
		aspectRatio,
		nearPlane,
		farPlane
	);
}

void Camera::SetOrthographic(
	float width,
	float height,
	float nearPlane,
	float farPlane
) noexcept {
	m_projection = DirectX::SimpleMath::Matrix::CreateOrthographic(
		width,
		height,
		nearPlane,
		farPlane
	);
}

bool Camera::LookAt(
	const DirectX::SimpleMath::Vector3& position,
	const DirectX::SimpleMath::Vector3& target,
	const DirectX::SimpleMath::Vector3& up
) noexcept {
	using namespace DirectX::SimpleMath;

	const Vector3 dir = target - position;

	if (dir.LengthSquared() <= 0.000001f)
		return false;

	// local
	const Matrix view = Matrix::CreateLookAt(position, target, up);

	const Matrix world = view.Invert();

	Vector3 scale, translation;
	Quaternion rotation;

	Matrix worldCopy = world;

	if (!worldCopy.Decompose(scale, rotation, translation))
		return false;

	Transform& transform = GetTransform();

	if (!transform.SetPosition(position))
		return false;

	if (!transform.SetRotation(rotation))
		return false;

	return true;
}

bool Camera::LookAtFromCurrentPosition(const DirectX::SimpleMath::Vector3& target, const DirectX::SimpleMath::Vector3& up) noexcept {
	return LookAt(GetTransform().GetPosition(), target, up);
}

DirectX::SimpleMath::Matrix Camera::GetView() const noexcept {
	const Transform& transform = GetTransform();

	const auto pos = transform.GetPosition();
	const auto forward = transform.GetForward();
	const auto up = transform.GetUp();

	return DirectX::SimpleMath::Matrix::CreateLookAt(pos, pos + forward, up);
}

DirectX::SimpleMath::Vector3 Camera::GetPosition() const noexcept {
	return GetTransform().GetPosition();
}

DirectX::SimpleMath::Vector3 Camera::GetForward() const noexcept {
	return GetTransform().GetForward();
}

DirectX::SimpleMath::Vector3 Camera::GetUp() const noexcept {
	return GetTransform().GetUp();
}
