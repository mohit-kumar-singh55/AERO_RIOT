#include "TextureFactory.h"

#include <stdexcept>
#include <vector>

#include <WICTextureLoader.h>

namespace TextureFactory {
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateRectangleTexture(
		ID3D11Device* device,
		int width,
		int height,
		ColorRGBA fillColor,
		ColorRGBA borderColor,
		int borderThickness
	) {
		// checks
		if (!device)
			throw std::invalid_argument("CreateRectangle received a null device");
		if (width <= 0 || height <= 0)
			throw std::invalid_argument("Texture width and height must be greater than 0");
		if (borderThickness < 0)
			borderThickness = 0;

		// create texture
		std::vector<ColorRGBA> pixels(static_cast<size_t>(width * height));

		// set the color
		for (int y = 0;y < height;y++) {
			for (int x = 0;x < width;x++) {
				// making the 4px border around player
				const bool isBorder =
					x < borderThickness ||
					x >= width - borderThickness ||
					y < borderThickness ||
					y >= height - borderThickness;

				pixels[y * width + x] =
					isBorder ? borderColor : fillColor;
			}
		}

		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = pixels.data();
		initData.SysMemPitch = width * sizeof(ColorRGBA);

		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;

		// create 2d texture with the above data
		HRESULT res = device->CreateTexture2D(
			&textureDesc,
			&initData,
			&texture
		);

		if (FAILED(res))
			throw std::runtime_error("Failed to create Texture2D");

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureView;

		// create shader resorce view (sprite batch uses shader resource view, not ID3D11Texture2D)
		res = device->CreateShaderResourceView(
			texture.Get(),
			&srvDesc,
			&textureView
		);

		if (FAILED(res))
			throw std::runtime_error("Failed to create ShaderResourceView");

		return textureView;
	}

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> LoadTextureFromFile(
		ID3D11Device* device,
		const wchar_t* filePath
	) {
		if (!device)
			throw std::invalid_argument("LoadTextureFromFile received a null device");
		if (!filePath)
			throw std::invalid_argument("LoadTextureFromFile received a null file path");

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureView;

		HRESULT res = DirectX::CreateWICTextureFromFile(
			device,
			filePath,
			nullptr,
			&textureView
		);

		if (FAILED(res))
			throw std::runtime_error("Failed to load texture from file");

		return textureView;
	}
}