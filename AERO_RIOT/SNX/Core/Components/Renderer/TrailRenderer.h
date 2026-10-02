#pragma once

#include "Renderer.h"

#include <SimpleMath.h>

#include <vector>
#include <wrl/client.h>
#include <d3d11.h>

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
	void EnsureVertexBufferCapacity(const RenderContext& context);

private:
	std::vector<TrailPoint> m_points;
	std::vector<TrailVertex> m_vertices;

	float m_lifeTime = 1.0f;						// total life time of each trail point
	float m_minPointDistanceSquared = 0.3f * 0.3f;	// min distance b/w each recorded trail point
	float m_width = 0.5f;							// width of ribbon (distance b/w right and left vertices of a trail point)
	float m_halfWidth = m_width * 0.5f;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;	// dynamic buffer
	std::size_t m_vertexCapacity = 0;
};