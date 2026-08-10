#include "BasicPrimitiveMaterial.h"

#include <SNX/Graphics/RenderContext.h>

#include <algorithm>
#include <stdexcept>

void BasicPrimitiveMaterial::Initialize(
	ID3D11Device* device,
	ID3D11DeviceContext* deviceContext,
	bool fogEnabled,
	const DirectX::SimpleMath::Vector3& fogColor,
	float fogStart,
	float fogEnd
) {
	if (!device || !deviceContext)
		throw std::invalid_argument("BasicPrimitiveMatrial requires a valid device and device context");

	using DirectX::SimpleMath::Vector3;

	m_effect = std::make_unique<DirectX::BasicEffect>(device);

	// lighting settings
	m_effect->SetLightingEnabled(true);
	m_effect->SetPerPixelLighting(true);
	m_effect->EnableDefaultLighting();

	m_effect->SetAmbientLightColor(Vector3::One * 0.25f);
	m_effect->SetSpecularColor(Vector3::One * 0.3f);

	m_effect->SetSpecularPower(16.0f);

	// ! this material currently draws untextured primitives
	m_effect->SetTextureEnabled(false);
	m_effect->SetVertexColorEnabled(false);

	m_effect->SetFogEnabled(fogEnabled);

	if (fogEnabled) {
		m_effect->SetFogColor(fogColor);
		m_effect->SetFogStart(fogStart);
		m_effect->SetFogEnd(fogEnd > fogStart + 0.001f ? fogEnd : fogStart + 0.001f);
	}

	/*
	* Every DirectXTK GeometricPrimitive uses the same
	* "VertexPositionNormalTexture" vertex structure.
	*
	* Therefore, an input layout created from one cube
	* can be reused for cubes, spheres and other
	* GeometricPrimitive shapes using this effect.
	*/
	auto layoutSource = DirectX::GeometricPrimitive::CreateCube(deviceContext, 1.0f);

	if (!layoutSource)
		throw std::runtime_error("Failed to create the primitive used for the matrial input layout");

	m_inputLayout.Reset();

	layoutSource->CreateInputLayout(m_effect.get(), m_inputLayout.GetAddressOf());
}

void BasicPrimitiveMaterial::Draw(
	DirectX::GeometricPrimitive& primitive,
	const DirectX::SimpleMath::Matrix& world,
	const RenderContext& context,
	const DirectX::XMVECTORF32& diffuseColor,
	const DirectX::SimpleMath::Vector3& emissiveColor,
	bool wireframe
) {
	if (!IsInitialized() || !context.IsValid())
		return;

	using DirectX::SimpleMath::Vector3;

	const Vector3 diffuse{
		diffuseColor.f[0],
		diffuseColor.f[1],
		diffuseColor.f[2],
	};

	const float alpha = std::clamp(diffuseColor.f[3], 0.0f, 1.0f);

	m_effect->SetWorld(world);
	m_effect->SetView(context.view);
	m_effect->SetProjection(context.projection);

	m_effect->SetDiffuseColor(diffuse);
	m_effect->SetEmissiveColor(emissiveColor);
	m_effect->SetAlpha(alpha);

	primitive.Draw(
		m_effect.get(),
		m_inputLayout.Get(),
		alpha < 0.999f,
		wireframe
	);
}