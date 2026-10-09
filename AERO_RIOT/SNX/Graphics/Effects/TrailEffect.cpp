#include "pch.h"
#include "TrailEffect.h"

#include <stdexcept>

#include <SNX/Utils/ErrorHandler.h>
#include <SNX/Utils/FileHandling.h>

void TrailEffect::Initialize(
	ID3D11Device* device,
	std::string_view vsFilePath,
	std::string_view psFilePath
) {
	if (!device)
		throw std::invalid_argument("TrailEffect::Initialize: device is invalid.");

	using namespace ErrorHandler;
	using namespace FileHandling;

	// ! load shader files
	m_vsBytecode = LoadShaderBytecode(vsFilePath.data());
	auto psBytecode = LoadShaderBytecode(psFilePath.data());

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
}

void TrailEffect::Apply(ID3D11DeviceContext* context) {
	if (!context)
		throw std::invalid_argument("TrailEffect::Apply: context is invalid.");

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

	ID3D11Buffer* cTransformBuffer = m_cTransformBuffer.Get();
	ID3D11Buffer* cMaterialBuffer = m_cMaterialBuffer.Get();

	// give buffer to the shader
	context->VSSetConstantBuffers(
		0,					// register slot (b0) in the shader
		1,					// no. of buffers
		&cTransformBuffer	// buffer(s)
	);
	context->PSSetConstantBuffers(
		0,
		1,
		&cMaterialBuffer
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

void TrailEffect::GetVertexShaderBytecode(
	void const** bytecode,
	size_t* length
) {
	*bytecode = m_vsBytecode.data();
	*length = m_vsBytecode.size();
}