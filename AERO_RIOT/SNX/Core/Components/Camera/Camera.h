#pragma once

#include <SNX/Core/Object/Component.h>

#include <SimpleMath.h>

class Camera : public Component {
public:
	explicit Camera(GameObject& gameObject) noexcept : Component(gameObject) {}
	virtual ~Camera() = default;

	// disallow to copy or move
	Camera(const Camera&) = delete;
	Camera& operator=(const Camera&) = delete;
	Camera(Camera&&) = delete;
	Camera& operator=(Camera&&) = delete;

	void SetPerspective(
		float fovDegrees,
		float aspectRatio,
		float nearPlane,
		float farPlane
	) noexcept;

	void SetOrthographic(
		float width,
		float height,
		float nearPlane,
		float farPlane
	) noexcept;

	/*
	* Move + rotate this camera GameObject so that
	* it looks at the supplied world-space target.
	*/
	bool LookAt(
		const DirectX::SimpleMath::Vector3& position,
		const DirectX::SimpleMath::Vector3& target,
		const DirectX::SimpleMath::Vector3& up = DirectX::SimpleMath::Vector3::Up
	) noexcept;

	/*
	* Look from the camera's current Transform position.
	*/
	bool LookAtFromCurrentPosition(
		const DirectX::SimpleMath::Vector3& target,
		const DirectX::SimpleMath::Vector3& up = DirectX::SimpleMath::Vector3::Up
	) noexcept;

	[[nodiscard]]
	DirectX::SimpleMath::Matrix GetView() const noexcept;

	[[nodiscard]]
	const DirectX::SimpleMath::Matrix& GetProjection() const noexcept { return m_projection; }

	[[nodiscard]]
	DirectX::SimpleMath::Vector3 GetPosition() const noexcept;

	[[nodiscard]]
	DirectX::SimpleMath::Vector3 GetForward() const noexcept;

	[[nodiscard]]
	DirectX::SimpleMath::Vector3 GetUp() const noexcept;

private:
	DirectX::SimpleMath::Matrix m_projection = DirectX::SimpleMath::Matrix::Identity;
};