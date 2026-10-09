#pragma once

#include <vector>
#include <stdexcept>
#include <fstream>

namespace FileHandling {
	inline std::vector<uint8_t> LoadShaderBytecode(const char* fileName) {
		std::ifstream file(
			fileName,
			std::ios::binary | std::ios::ate
		);

		if (!file)
			throw std::runtime_error("FileHandling::LoadShaderByteCode: Shader file not found.");

		const std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		if (size <= 0)
			throw std::runtime_error("FileHandling::LoadShaderByteCode: Shader file is empty.");

		std::vector<std::uint8_t> bytecode(size);

		file.read(
			reinterpret_cast<char*>(bytecode.data()),
			size
		);

		return bytecode;
	};
}