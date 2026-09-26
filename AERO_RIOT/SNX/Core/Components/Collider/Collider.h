#pragma once

#include <SNX/Core/Object/Component.h>
#include <SNX/Core/Components/Transform.h>

#include <SimpleMath.h>

using DirectX::SimpleMath::Vector3;

class KineticBody;

enum class ColliderShape { Sphere, Box, Capsule };

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

	[[nodiscard]]
	Vector3 GetCenter() const noexcept {
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
};