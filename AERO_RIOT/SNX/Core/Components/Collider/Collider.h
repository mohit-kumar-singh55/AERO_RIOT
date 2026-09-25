#pragma once

#include <SNX/Core/Object/Component.h>
#include <SNX/Core/Components/Transform.h>

#include <SimpleMath.h>

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
	ColliderShape GetShape() const noexcept { return m_shape; }

	[[nodiscard]]
	DirectX::SimpleMath::Vector3 GetCenter() const noexcept {
		return GetTransform().GetPosition() + m_offset;
	}

protected:
	void OnInitialize() override;
	void OnDestroy() override;

	void OnStart() override;

protected:
	KineticBody* m_kb = nullptr;

	bool m_isTrigger = false;
	ColliderShape m_shape = ColliderShape::Sphere;
	// offset from the position of the game object, it is attached to
	DirectX::SimpleMath::Vector3 m_offset = DirectX::SimpleMath::Vector3::Zero;
};