#pragma once

#include <DirectXMath.h>
#include <Effects.h>
#include <GeometricPrimitive.h>
#include <SimpleMath.h>

#include <d3d11.h>
#include <wrl/client.h>

#include <memory>

struct RenderContext;

class BasicPrimitiveMaterial final {
public:
	BasicPrimitiveMaterial() = default;
	~BasicPrimitiveMaterial() = default;

	// disallow to copy or more
	BasicPrimitiveMaterial(const BasicPrimitiveMaterial&) = delete;
	BasicPrimitiveMaterial& operator=(const BasicPrimitiveMaterial&) = delete;
	BasicPrimitiveMaterial(BasicPrimitiveMaterial&&) = delete;
	BasicPrimitiveMaterial& operator=(BasicPrimitiveMaterial&&) = delete;

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
	);

	bool IsInitialized() const noexcept {
		return
			m_effect != nullptr &&
			m_inputLayout != nullptr;
	}

private:
	std::unique_ptr<DirectX::BasicEffect> m_effect;

	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
};