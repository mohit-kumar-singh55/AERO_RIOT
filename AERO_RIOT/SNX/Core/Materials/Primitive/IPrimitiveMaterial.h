#pragma once

#include <GeometricPrimitive.h>
#include <SimpleMath.h>

#include <SNX/Graphics/RenderContext.h>

// common interface for the materials used by DXTK's primitive shapes
class IPrimitiveMaterial {
public:
	virtual ~IPrimitiveMaterial() = default;

	virtual void Draw(
		DirectX::GeometricPrimitive& primitive,
		const DirectX::SimpleMath::Matrix& world,
		const RenderContext& context,
		const DirectX::XMVECTORF32& diffuseColor,
		const DirectX::SimpleMath::Vector3& emissiveColor,
		bool wireframe = false
	) = 0;
};