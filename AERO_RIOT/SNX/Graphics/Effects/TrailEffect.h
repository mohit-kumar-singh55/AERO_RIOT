#pragma once

#include <d3d11.h>
#include <string_view>
#include <wrl/client.h>
#include <vector>
#include <cstdint>

struct TrailTransformBuffer {
	DirectX::XMMATRIX ViewProjection;
};

struct TrailMaterialBuffer {
	DirectX::XMFLOAT4 Color{ 1.0f,1.0f,1.0f,1.0f };
};

class TrailEffect final {
public:
	TrailEffect() = default;
	~TrailEffect() = default;

	void Initialize(
		ID3D11Device* device,
		std::string_view vsFilePath,
		std::string_view psFilePath
	);

	void Apply(ID3D11DeviceContext* context);

	void GetVertexShaderBytecode(
		void const** bytecode,
		size_t* length
	);

	void SetViewProjection(DirectX::XMMATRIX wvp) noexcept {
		m_transformBuffer.ViewProjection = wvp;
	}

	void SetMaterial(DirectX::XMFLOAT4 color) noexcept {
		m_materialBuffer.Color = color;
	}

private:
	std::vector<std::uint8_t> m_vsBytecode;
	TrailTransformBuffer m_transformBuffer;
	TrailMaterialBuffer m_materialBuffer;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
	// constant buffers
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_cTransformBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_cMaterialBuffer;
};