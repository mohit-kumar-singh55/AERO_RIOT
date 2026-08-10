#include "ModelObject3D.h"

#include <DirectXColors.h>
#include <DirectXMath.h>

#include <exception>

bool ModelObject3D::LoadFromCMO(
	ID3D11Device* device,
	DirectX::IEffectFactory& effectFactory,
	const wchar_t* filePath
) noexcept {
	if (!device || !filePath) return false;

	try {
		m_model = DirectX::Model::CreateFromCMO(
			device,
			filePath,
			effectFactory
		);

		return m_model != nullptr;
	}
	catch (const std::exception&) {
		m_model.reset();
		return false;
	}
}

bool ModelObject3D::LoadFromSDKMESH(
	ID3D11Device* device,
	DirectX::IEffectFactory& effectFactory,
	const wchar_t* filePath
) noexcept {
	if (!device || !filePath) return false;

	try {
		m_model = DirectX::Model::CreateFromSDKMESH(
			device,
			filePath,
			effectFactory
		);

		return m_model != nullptr;
	}
	catch (const std::exception&) {
		m_model.reset();
		return false;
	}
}

void ModelObject3D::UpdateEffects(
	const DirectX::SimpleMath::Vector3& fogColor,
	float fogStart,
	float fogEnd
) {
	if (!m_model) return;

	m_model->UpdateEffects(
		[&](DirectX::IEffect* effect) {
			if (auto* lights = dynamic_cast<DirectX::IEffectLights*>(effect)) {
				lights->SetLightingEnabled(true);
				lights->SetPerPixelLighting(true);
				lights->EnableDefaultLighting();

				lights->SetAmbientLightColor(
					DirectX::XMVectorSet(
						0.25f,
						0.25f,
						0.25f,
						1.0f
					)
				);

				lights->SetLightEnabled(0, true);

				lights->SetLightDirection(
					0,
					DirectX::XMVector3Normalize(
						DirectX::XMVectorSet(
							0.5f,
							-1.0f,
							0.5f,
							0.0f
						)
					)
				);

				lights->SetLightDiffuseColor(0, DirectX::Colors::GhostWhite);

				lights->SetLightEnabled(1, false);
				lights->SetLightEnabled(2, false);
			}

			if (auto* fog = dynamic_cast<DirectX::IEffectFog*>(effect)) {
				fog->SetFogEnabled(true);
				fog->SetFogStart(fogStart);
				fog->SetFogEnd(fogEnd);

				fog->SetFogColor(
					DirectX::XMVectorSet(
						fogColor.x,
						fogColor.y,
						fogColor.z,
						1.0
					)
				);
			}
		}
	);
}

void ModelObject3D::Draw(
	ID3D11DeviceContext* context,
	DirectX::CommonStates& states,
	const DirectX::SimpleMath::Matrix& view,
	const DirectX::SimpleMath::Matrix& projection
) const {
	if (!context || !m_model) return;

	using DirectX::SimpleMath::Matrix;

	const Matrix world =
		Matrix::CreateScale(m_scale) *
		Matrix::CreateRotationY(m_yaw) *
		Matrix::CreateTranslation(m_position);

	m_model->Draw(
		context,
		states,
		world,
		view,
		projection
	);
}

void ModelObject3D::DrawWithTransform(
	ID3D11DeviceContext* context,
	DirectX::CommonStates& states,
	const DirectX::SimpleMath::Matrix& world,
	const DirectX::SimpleMath::Matrix& view,
	const DirectX::SimpleMath::Matrix& projection
) const {
	if (!context || !m_model) return;

	m_model->Draw(
		context,
		states,
		world,
		view,
		projection
	);
}

void ModelObject3D::SetPosition(const DirectX::SimpleMath::Vector3& position) noexcept {
	m_position = position;
}

void ModelObject3D::SetTransform(
	const DirectX::SimpleMath::Vector3& position,
	float yaw,
	float scale
) noexcept {
	m_position = position;
	m_yaw = yaw;
	m_scale = scale;
}

void ModelObject3D::SetScale(float scale) noexcept {
	m_scale = scale;
}

void ModelObject3D::SetYaw(float yaw) noexcept {
	m_yaw = yaw;
}

bool ModelObject3D::IsLoaded() const noexcept {
	return m_model != nullptr;
}