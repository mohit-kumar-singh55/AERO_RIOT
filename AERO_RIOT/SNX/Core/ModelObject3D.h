#pragma once

#include <CommonStates.h>
#include <Effects.h>
#include <Model.h>
#include <SimpleMath.h>

#include <d3d11.h>

#include <memory>

class ModelObject3D {
public:
	bool LoadFromCMO(
		ID3D11Device* device,
		DirectX::IEffectFactory& effectFactory,
		const wchar_t* filePath
	) noexcept;

	bool LoadFromSDKMESH(
		ID3D11Device* device,
		DirectX::IEffectFactory& effectFactory,
		const wchar_t* filePath
	) noexcept;

	void UpdateEffects(
		const DirectX::SimpleMath::Vector3& fogColor,
		float fogStart,
		float fogEnd
	);

	void Draw(
		ID3D11DeviceContext* context,
		DirectX::CommonStates& states,
		const DirectX::SimpleMath::Matrix& view,
		const DirectX::SimpleMath::Matrix& projection
	) const;

	void DrawWithTransform(
		ID3D11DeviceContext* context,
		DirectX::CommonStates& states,
		const DirectX::SimpleMath::Matrix& world,
		const DirectX::SimpleMath::Matrix& view,
		const DirectX::SimpleMath::Matrix& projection
	) const;

	void SetPosition(
		const DirectX::SimpleMath::Vector3& position
	) noexcept;

	void SetTransform(
		const DirectX::SimpleMath::Vector3& position,
		float yaw,
		float scale
	) noexcept;

	void SetScale(float scale) noexcept;

	void SetYaw(float yaw) noexcept;

	[[nodiscard]]
	bool IsLoaded() const noexcept;

private:
	std::unique_ptr<DirectX::Model> m_model;

	DirectX::SimpleMath::Vector3 m_position = DirectX::SimpleMath::Vector3::Zero;

	float m_scale = 1.0f;
	float m_yaw = 0.0f;
};