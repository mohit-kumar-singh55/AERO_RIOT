#include "pch.h"
#include "UnlitEffect.h"

#include <stdexcept>
#include <fstream>

#include <SNX/Utils/ErrorHandler.h>

void UnlitEffect::Initialize(
	ID3D11Device* device,
	std::string_view vsFilePath,
	std::string_view psFilePath
) {
	if (!device)
		throw std::invalid_argument("UnlitEffect::Initialize: device is invalid.");

	using namespace ErrorHandler;

	auto loadShaderBytecode = [](const char* fileName) {
		std::ifstream file(
			fileName,
			std::ios::binary | std::ios::ate
		);

		if (!file)
			throw std::runtime_error("UnlitEffect::Initialize: Shader file not found.");

		const std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		if (size <= 0)
			throw std::runtime_error("UnlitEffect::Initialize: Shader file is empty.");

		std::vector<std::uint8_t> bytecode(size);

		file.read(
			reinterpret_cast<char*>(bytecode.data()),
			size
		);

		return bytecode;
		};

	// ! load shader files
	m_vsBytecode = loadShaderBytecode(vsFilePath.data());
	auto psBytecode = loadShaderBytecode(psFilePath.data());

	// ! create shaders using the bytecodes
	ThrowIfFailed(
		device->CreateVertexShader(
			m_vsBytecode.data(),
			m_vsBytecode.size(),
			nullptr,
			&m_vertexShader
		),
		"Unable to create Vertex Shader"
	);
	ThrowIfFailed(
		device->CreatePixelShader(
			psBytecode.data(),
			psBytecode.size(),
			nullptr,
			&m_pixelShader
		),
		"Unable to create Pixel Shader"
	);

	// ! create constant buffer to pass in the shader
	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.ByteWidth = sizeof(m_transformBuffer);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	ThrowIfFailed(
		device->CreateBuffer(
			&bufferDesc,
			nullptr,
			&m_cTransformBuffer
		),
		"Unable to create constant transform buffer"
	);

	bufferDesc.ByteWidth = sizeof(m_materialBuffer);
	ThrowIfFailed(
		device->CreateBuffer(
			&bufferDesc,
			nullptr,
			&m_cMaterialBuffer
		),
		"Unable to create constant material buffer"
	);

	bufferDesc.ByteWidth = sizeof(m_frameBuffer);
	ThrowIfFailed(
		device->CreateBuffer(
			&bufferDesc,
			nullptr,
			&m_cFrameBuffer
		),
		"Unable to create constant frame buffer"
	);
}

void UnlitEffect::Apply(ID3D11DeviceContext* context) {
	if (!context)
		throw std::invalid_argument("UnlitEffect::Apply: context is invalid.");

	// copy cpu data to gpu (put data into buffer)
	context->UpdateSubresource(
		m_cTransformBuffer.Get(),	// kind of data
		0,
		nullptr,
		&m_transformBuffer,			// actual data
		0,
		0
	);
	context->UpdateSubresource(
		m_cMaterialBuffer.Get(),
		0,
		nullptr,
		&m_materialBuffer,
		0,
		0
	);
	context->UpdateSubresource(
		m_cFrameBuffer.Get(),
		0,
		nullptr,
		&m_frameBuffer,
		0,
		0
	);

	ID3D11Buffer* cTransformBuffer = m_cTransformBuffer.Get();
	ID3D11Buffer* cPSBuffers[] = {
		m_cMaterialBuffer.Get(),
		m_cFrameBuffer.Get()
	};

	// give buffer to the shader
	context->VSSetConstantBuffers(
		0,					// register slot (b0) in the shader
		1,					// no. of buffers
		&cTransformBuffer	// buffer(s)
	);
	context->PSSetConstantBuffers(
		0,
		2,
		cPSBuffers
	);

	// set which VS to use
	context->VSSetShader(
		m_vertexShader.Get(),
		nullptr,
		0
	);

	// set which PS to use
	context->PSSetShader(
		m_pixelShader.Get(),
		nullptr,
		0
	);
}

void UnlitEffect::GetVertexShaderBytecode(
	void const** bytecode,
	size_t* length
) {
	*bytecode = m_vsBytecode.data();
	*length = m_vsBytecode.size();
}