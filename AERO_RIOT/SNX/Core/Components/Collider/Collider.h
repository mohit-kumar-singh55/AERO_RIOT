#pragma once

#include <SNX/Core/Object/Component.h>
#include <SNX/Core/Components/Transform.h>

#include <SimpleMath.h>

using DirectX::SimpleMath::Vector3;

class KineticBody;

enum class ColliderShape { Sphere, Box, Capsule };

enum class CollisionDetectionMode {
	Discrete,
	Continuous
};

class Collider : public Component {
public:
	using Component::Component;

	[[nodiscard]]
	KineticBody* GetKineticBody() noexcept { return m_kb; }

	[[nodiscard]]
	bool IsTrigger() const noexcept { return m_isTrigger; }

	void SetIsTrigger(bool trigger) noexcept { m_isTrigger = trigger; }

	[[nodiscard]]
	virtual ColliderShape GetShape() const noexcept = 0;

	// local-space offset
	[[nodiscard]]
	Vector3 GetOffset() const noexcept { return m_offset; }

	// local-space offset
	void SetOffset(Vector3 localOffset) noexcept { m_offset = localOffset; }

	// current world center
	[[nodiscard]]
	Vector3 GetCenter() const noexcept;

	// previous physics-step world center
	[[nodiscard]]
	Vector3 GetPreviousCenter() const noexcept;

	[[nodiscard]]
	CollisionDetectionMode GetDetectionMode() const noexcept { return m_detectionMode; }

	void SetDetectionMode(CollisionDetectionMode detectionMode) noexcept {
		m_detectionMode = detectionMode;
	}

	// world-space
	Quaternion GetRotation() const noexcept {
		return GetTransform().GetRotation();
	}

protected:
	void OnInitialize() override;
	void OnDestroy() override;

	void OnStart() override;

protected:
	KineticBody* m_kb = nullptr;

	bool m_isTrigger = false;
	/*
	* local-space
	* offset from the position of the game object, it is attached to
	*/
	Vector3 m_offset = Vector3::Zero;

	CollisionDetectionMode m_detectionMode = CollisionDetectionMode::Discrete;
};