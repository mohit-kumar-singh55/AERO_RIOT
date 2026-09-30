#include "pch.h"
#include "UnlitEffect.h"

#include <stdexcept>

void UnlitEffect::Apply(ID3D11DeviceContext* context) {
	if (!context)
		throw std::invalid_argument("UnlitEffect::Apply: context is invalid.");

	
}

void UnlitEffect::GetVertexShaderBytecode(
	void const** bytecode,
	size_t* length
) {

}