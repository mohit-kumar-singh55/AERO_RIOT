#pragma once

#include "IPrimitiveMaterial.h"

#include <SNX/Graphics/Effects/UnlitEffect.h>

#include <wrl/client.h>
#include <d3d11.h>
#include <memory>

#include <DirectXMath.h>
#include <SimpleMath.h>
#include <GeometricPrimitive.h>

struct RenderContext;

class UnlitMaterial final : public IPrimitiveMaterial {
public:
	UnlitMaterial() = default;
	~UnlitMaterial() = default;

	// disallow to copy or more
	UnlitMaterial(const UnlitMaterial&) = delete;
	UnlitMaterial& operator=(const UnlitMaterial&) = delete;
	UnlitMaterial(UnlitMaterial&&) = delete;
	UnlitMaterial& operator=(UnlitMaterial&&) = delete;

	void Initialize(
		ID3D11Device* device,
		ID3D11DeviceContext* deviceContext,
		bool fogEnabled = false,
		const DirectX::SimpleMath::Vector3& fogColor = DirectX::SimpleMath::Vector3::One,
		float fogStart = 0.0f,
		float fogEnd = 1.0f
	);

	void Draw(
		DirectX::GeometricPrimitive& primitive,
		const DirectX::SimpleMath::Matrix& world,
		const RenderContext& context,
		const DirectX::XMVECTORF32& diffuseColor,
		const DirectX::SimpleMath::Vector3& emissiveColor,
		bool wireframe = false
	) override;

	bool IsInitialized() const noexcept {
		return
			m_effect != nullptr &&
			m_inputLayout != nullptr;
	}

private:
	std::unique_ptr<UnlitEffect> m_effect;

	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
};