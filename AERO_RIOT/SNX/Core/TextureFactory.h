#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <cstdint>

namespace TextureFactory {
	struct ColorRGBA {
		std::uint8_t r;
		std::uint8_t g;
		std::uint8_t b;
		std::uint8_t a;
	};

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateRectangleTexture(
		ID3D11Device* device,
		int width,
		int height,
		ColorRGBA fillColor,
		ColorRGBA borderColor,
		int borderThickness
	);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> LoadTextureFromFile(
		ID3D11Device* device,
		const wchar_t* filePath
	);
}