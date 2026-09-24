#pragma once

#include <SNX/Core/Object/Component.h>

#include <SimpleMath.h>

class KineticBody;

class Bullet final : public Component {
public:
	using Component::Component;

	void Launch(
		const DirectX::SimpleMath::Vector3 direction,
		float speed
	);

protected:
	void OnStart() override;
	void OnUpdate() override;

private:
	KineticBody* m_kb;

	float m_lifeTime = 2.0f;

	float m_lifeTimeTimer = 0.0f;
};