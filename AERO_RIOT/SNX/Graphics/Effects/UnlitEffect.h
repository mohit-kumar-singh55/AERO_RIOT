#pragma once

#include <Effects.h>

#include <vector>
#include <wrl/client.h>

struct TransformBuffer {
	DirectX::XMMATRIX WVP;
};

struct MaterialBuffer {
	DirectX::XMFLOAT4 Color{ 1.0f, 1.0f, 1.0f,1.0f };
	DirectX::XMFLOAT4 Emission{ 0.0f, 0.0f, 0.0f, 0.0f };
};

class UnlitEffect : public DirectX::IEffect {
public:
	~UnlitEffect() = default;

	void Initialize(ID3D11Device* device);

	void Apply(ID3D11DeviceContext* context) override;

	void GetVertexShaderBytecode(
		void const** bytecode,
		size_t* length
	) override;

	void SetWorldViewProjection(DirectX::XMMATRIX wvp) noexcept {
		m_transformBuffer.WVP = wvp;
	}

	void SetMaterial(
		DirectX::XMFLOAT4 color,
		DirectX::XMFLOAT4 emission = { 0.0f, 0.0f, 0.0f, 0.0f }
	) noexcept {
		m_materialBuffer.Color = color;
		m_materialBuffer.Emission = emission;
	}

private:
	std::vector<std::uint8_t> m_vsBytecode;
	TransformBuffer m_transformBuffer;
	MaterialBuffer m_materialBuffer;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_cTransformBuffer;	// constant buffer
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_cMaterialBuffer;		// constant buffer
};