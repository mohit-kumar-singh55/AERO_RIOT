#pragma once

#include <Windows.h>
#include <stdexcept>

namespace ErrorHandler {
	inline void ThrowIfFailed(
		HRESULT result,
		const char* msg = "A Direct3D operation failed."
	) {
		if (FAILED(result))
			throw std::runtime_error(msg);
	}
}