#pragma once

#include <d3d11.h>
#include <SimpleMath.h>

struct RenderContext final {
	/*
	* Non-owning pointers
	* DeviceResources remains their owner
	*/
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* deviceContext = nullptr;

	DirectX::SimpleMath::Matrix view = DirectX::SimpleMath::Matrix::Identity;
	DirectX::SimpleMath::Matrix projection = DirectX::SimpleMath::Matrix::Identity;
	DirectX::SimpleMath::Vector3 cameraPosition = DirectX::SimpleMath::Vector3::Zero;

	int viewportWidth = 0;
	int viewportHeight = 0;

	/*
	* Used to interpolate physics transforms
	* between fixed simulation steps
	*/
	float fixedInterpolationAlpha = 0.0f;

	[[nodiscard]]
	bool IsValid() const noexcept {
		return
			device != nullptr &&
			deviceContext != nullptr &&
			viewportWidth > 0 &&
			viewportHeight > 0;
	}
};