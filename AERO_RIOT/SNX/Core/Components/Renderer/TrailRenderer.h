#pragma once

#include "Renderer.h"

#include <SimpleMath.h>

#include <vector>
#include <wrl/client.h>
#include <d3d11.h>
#include <memory>

#include <SNX/Graphics/Effects/TrailEffect.h>

struct TrailPoint {
	DirectX::SimpleMath::Vector3 position;
	float age = 0.0f;
};

struct TrailVertex {
	DirectX::SimpleMath::Vector3 position;
	float alpha;
};

class TrailRenderer final : public Renderer {
public:
	explicit TrailRenderer(GameObject& gameObject) noexcept;

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
	float m_minPointDistanceSquared = 0.2f * 0.2f;	// min distance b/w each recorded trail point
	float m_width = 0.5f;							// width of ribbon (distance b/w right and left vertices of a trail point)
	float m_halfWidth = m_width * 0.5f;
	float m_fadeStart = 0.2f;						// from which point of age, start fading

	std::unique_ptr<TrailEffect> m_trailEffect;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_trailRasterizerState;
	Microsoft::WRL::ComPtr<ID3D11BlendState> m_trailBlendState;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;	// dynamic buffer
	std::size_t m_vertexCapacity = 0;
};