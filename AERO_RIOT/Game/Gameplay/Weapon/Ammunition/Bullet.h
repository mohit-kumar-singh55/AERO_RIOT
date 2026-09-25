#pragma once

#include <SNX/Core/Object/Component.h>

#include <SimpleMath.h>

class KineticBody;
class SphereCollider;

class Bullet final : public Component {
public:
	using Component::Component;

	void RequestLaunch(
		const DirectX::SimpleMath::Vector3 direction,
		float speed
	) noexcept;

protected:
	void OnStart() override;
	void OnUpdate() override;

private:
	void Launch() noexcept;

private:
	KineticBody* m_kb = nullptr;
	SphereCollider* m_col= nullptr;

	float m_lifeTime = 4.0f;

	float m_lifeTimeTimer = 0.0f;

	bool m_launchRequested = false;
	float m_launchSpeed = 0.0f;
	DirectX::SimpleMath::Vector3 m_launchDirection;
};