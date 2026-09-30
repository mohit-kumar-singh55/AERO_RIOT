#pragma once

#include <Effects.h>

#include <vector>
#include <wrl/client.h>

struct TransformBuffer {
	DirectX::XMMATRIX WVP;
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

private:
	std::vector<std::uint8_t> m_vsBytecode;
	TransformBuffer m_transformBuffer;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_cTransformBuffer;				// constant buffer
};