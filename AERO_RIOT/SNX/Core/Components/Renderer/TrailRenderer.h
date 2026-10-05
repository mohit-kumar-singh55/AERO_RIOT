#pragma once

#include "Renderer.h"

#include <SimpleMath.h>

#include <vector>
#include <wrl/client.h>
#include <d3d11.h>
#include <memory>
#include <algorithm>

#include <SNX/Graphics/Effects/TrailEffect.h>
#include <SNX/Utils/Gradient.h>

struct TrailPoint {
	DirectX::SimpleMath::Vector3 position;
	float age = 0.0f;
};

struct TrailVertex {
	DirectX::SimpleMath::Vector3 position;
	DirectX::SimpleMath::Vector4 color;
};

class TrailRenderer final : public Renderer {
public:
	explicit TrailRenderer(GameObject& gameObject) noexcept;

	[[nodiscard]]
	RenderPass GetRenderPass() const noexcept override { return RenderPass::Transparent; }

	Gradient& GetGradient() noexcept { return m_gradient; }

	[[nodiscard]]
	float GetLifeTime() const noexcept { return m_lifeTime; }

	void SetLifeTime(float lifeTime) noexcept { m_lifeTime = std::abs(lifeTime); }

	[[nodiscard]]
	float GetTrailWidth() const noexcept { return m_width; }

	void SetTrailWidth(float width) noexcept { m_width = std::abs(width); }

protected:
	void OnUpdate() override;
	void Draw(const RenderContext& context) override;

private:
	void CreateEffectAndInputLayout(const RenderContext& context);
	void EnsureVertexBufferCapacity(const RenderContext& context);

private:
	std::vector<TrailPoint> m_points;
	std::vector<TrailVertex> m_vertices;

	float m_lifeTime = 0.2f;						// total life time of each trail point
	float m_minPointDistanceSquared = 0.1f * 0.1f;	// min distance b/w each recorded trail point
	float m_width = 0.5f;							// width of ribbon (distance b/w right and left vertices of a trail point)
	float m_halfWidth = m_width * 0.5f;
	Gradient m_gradient;

	std::unique_ptr<TrailEffect> m_trailEffect;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_trailRasterizerState;
	Microsoft::WRL::ComPtr<ID3D11BlendState> m_trailBlendState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_trailDepthState;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;	// dynamic buffer
	std::size_t m_vertexCapacity = 0;
};