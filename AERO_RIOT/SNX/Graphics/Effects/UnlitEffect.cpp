#include "pch.h"
#include "UnlitEffect.h"

#include <stdexcept>
#include <fstream>

void UnlitEffect::Initialize(ID3D11Device* device) {
	if (!device)
		throw std::invalid_argument("UnlitEffect::Initialize: device is invalid.");

	auto file = [](const char* fileName) {
		std::ifstream file(
			fileName,
			std::ios::binary | std::ios::ate
		);

		if (!file)
			throw std::runtime_error("UnlitEffect::Initialize: Shader file not found.");

		file.seekg(0, std::ios::beg);

		return file;
		};

	// ! load shader files
	auto vsFile = file("Shaders\\UnlitVS.cso");
	auto psFile = file("Shaders\\UnlitPS.cso");

	m_vsBytecode.resize(static_cast<size_t>(vsFile.tellg()));
	std::vector<std::uint8_t> psBytecode(psFile.tellg());

	vsFile.read(
		reinterpret_cast<char*>(m_vsBytecode.data()),
		vsFile.tellg()
	);
	psFile.read(
		reinterpret_cast<char*>(psBytecode.data()),
		psFile.tellg()
	);

	// ! create shaders using the bytecodes
	device->CreateVertexShader(
		m_vsBytecode.data(),
		m_vsBytecode.size(),
		nullptr,
		&m_vertexShader
	);
	device->CreatePixelShader(
		psBytecode.data(),
		psBytecode.size(),
		nullptr,
		&m_pixelShader
	);

	// ! create constant buffer to pass in the shader
	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.ByteWidth = sizeof(m_transformBuffer);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	device->CreateBuffer(
		&bufferDesc,
		nullptr,
		&m_cTransformBuffer
	);
}

void UnlitEffect::Apply(ID3D11DeviceContext* context) {
	if (!context)
		throw std::invalid_argument("UnlitEffect::Apply: context is invalid.");


}

void UnlitEffect::GetVertexShaderBytecode(
	void const** bytecode,
	size_t* length
) {

}