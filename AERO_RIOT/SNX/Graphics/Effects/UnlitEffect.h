#pragma once

#include <Effects.h>

class UnlitEffect : public DirectX::IEffect {
public:
	~UnlitEffect() = default;

	void Apply(ID3D11DeviceContext* context) override;

	void GetVertexShaderBytecode(
		void const** bytecode,
		size_t* length
	) override;

private:
	const void* bytecode;

	struct CameraMatrix {
		DirectX::XMMATRIX WVP;
	};
};