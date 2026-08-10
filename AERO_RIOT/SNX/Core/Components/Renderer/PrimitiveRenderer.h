#pragma once

#include "Renderer.h"
#include <SNX/Core/Materials/BasicPrimitiveMaterial.h>

#include <DirectXMath.h>
#include <GeometricPrimitive.h>

#include <d3d11.h>

#include <memory>

enum class PrimitiveShape { Cube, Sphere };

class PrimitiveRenderer final : public Renderer {
public:
	PrimitiveRenderer(
		GameObject& gameObject,
		ID3D11DeviceContext* deviceContext,
		PrimitiveShape shape = PrimitiveShape::Cube,
		std::shared_ptr<BasicPrimitiveMaterial> material = nullptr
	) noexcept;

	void SetColor(const DirectX::XMVECTORF32& color) { m_color = color; }

	[[nodiscard]]
	const DirectX::XMVECTORF32& GetColor() const noexcept { return m_color; }

	void SetMaterial(std::shared_ptr<BasicPrimitiveMaterial> material) noexcept {
		m_material = std::move(material);
	}

	void SetEmissiveColor(const DirectX::SimpleMath::Vector3& emissiveColor) noexcept {
		m_emissiveColor = emissiveColor;
	}

	const DirectX::SimpleMath::Vector3& GetEmissiveColor() const noexcept {
		return m_emissiveColor;
	}

	void SetWireframe(bool wireframe) noexcept {
		m_wireframe = wireframe;
	}

	[[nodiscard]]
	bool IsWireframe() const noexcept {
		return m_wireframe;
	}

protected:
	void OnInitialize() override;

	void Draw(const RenderContext& context) override;

	void OnDestroy() override;

private:
	ID3D11DeviceContext* m_creationContext = nullptr;

	PrimitiveShape m_shape = PrimitiveShape::Cube;

	std::unique_ptr<DirectX::GeometricPrimitive> m_primitive;

	std::shared_ptr<BasicPrimitiveMaterial> m_material;

	DirectX::XMVECTORF32 m_color{ 1.0f, 1.0f, 1.0f, 1.0f };

	DirectX::SimpleMath::Vector3 m_emissiveColor = DirectX::SimpleMath::Vector3::Zero;

	bool m_wireframe = false;
};