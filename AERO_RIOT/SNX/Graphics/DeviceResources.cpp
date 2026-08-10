#include "DeviceResources.h"

#include <stdexcept>
#include <iterator>

// ! safe shutdown (Direct3D 11 cleanup pattern)
DeviceResources::~DeviceResources() {
	if (!m_context) return;

	// remove resources still bound to this graphics pipeline
	m_context->ClearState();

	// submit any remaining commands
	m_context->Flush();

	// ? COMptr will release automatically
}

void DeviceResources::Initialize(HWND window, int width, int height) {
	if (!window)
		throw std::invalid_argument("DeviceResources requires a valid window");

	if (width <= 0 || height <= 0)
		throw std::invalid_argument("DevicedResources requires a valid size");

	m_window = window;
	m_width = width;
	m_height = height;

	CreateDeviceAndSwapChain();
	CreateRenderTarget();
	CreateDepthBuffer();
}

/* bind render targets, sets viewport and clears both buffers */
void DeviceResources::BeginFrame(const float clearColor[4]) {
	if (!clearColor)
		throw std::invalid_argument("BeginFrame requires a clear color");

	/*
	* can have Multiple Render Targets(MRT)
	* commonly used in deferred rendering.
	* instead of rendering the scene several times,
	* one pass fills multiple textures (called a G-buffer).
	*/
	ID3D11RenderTargetView* renderTargets[]{
		m_renderTargetView.Get()
	};

	m_context->OMSetRenderTargets(
		1,
		renderTargets,
		m_depthStencilView.Get()
	);

	// set the viewport
	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(m_width);
	viewport.Height = static_cast<float>(m_height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	m_context->RSSetViewports(1, &viewport);

	// clear render target view
	m_context->ClearRenderTargetView(
		m_renderTargetView.Get(),
		clearColor
	);

	// clear depth stencil view
	m_context->ClearDepthStencilView(
		m_depthStencilView.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0
	);
}

void DeviceResources::Present(bool useVSync) {
	const UINT syncInterval = useVSync ? 1u : 0u;

	ThrowIfFailed(m_swapChain->Present(syncInterval, 0));
}

void DeviceResources::CreateDeviceAndSwapChain() {
	// create swap chain settings instance
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

	// swap chain buffer settings
	swapChainDesc.BufferDesc.Width = static_cast<UINT>(m_width);
	swapChainDesc.BufferDesc.Height = static_cast<UINT>(m_height);
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;			// ! FPS
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;

	// swap chain sample settings
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// swap chain settings
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = m_window;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// define feature levels
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_0,
	};

	// create device and swap chain
	ThrowIfFailed(
		D3D11CreateDeviceAndSwapChain(
			nullptr,                    // Adapter (means GPU) if nullptr -> DX will choose itself
			D3D_DRIVER_TYPE_HARDWARE,   // Use GPU hardware
			nullptr,                    // Software rasterizer (Rasterizer is the stage in the graphics pipeline that converts triangles into pixels (fragments))
			0,                          // Flags
			featureLevels,
			static_cast<UINT>(std::size(featureLevels)),
			D3D11_SDK_VERSION,
			&swapChainDesc,
			m_swapChain.GetAddressOf(),
			m_device.GetAddressOf(),
			&m_featureLevel,
			m_context.GetAddressOf()
		)
	);
}

void DeviceResources::CreateRenderTarget() {
	// Get back buffer from swap chain
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;

	ThrowIfFailed(
		m_swapChain->GetBuffer(
			0,
			IID_PPV_ARGS(backBuffer.GetAddressOf())
		)
	);

	// Create render target view
	ThrowIfFailed(
		m_device->CreateRenderTargetView(
			backBuffer.Get(),
			nullptr,
			m_renderTargetView.GetAddressOf()
		)
	);
}

/*
* Render target = color image (what color is this pixel?)
* Depth buffer  = distance image (how far away is this pixel?)
*/
void DeviceResources::CreateDepthBuffer() {
	D3D11_TEXTURE2D_DESC depthTextureDesc = {};

	depthTextureDesc.Width = static_cast<UINT>(m_width);
	depthTextureDesc.Height = static_cast<UINT>(m_height);
	depthTextureDesc.MipLevels = 1;
	depthTextureDesc.ArraySize = 1;
	depthTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthTextureDesc.SampleDesc.Count = 1;
	depthTextureDesc.SampleDesc.Quality = 0;
	depthTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthTexture;

	ThrowIfFailed(
		m_device->CreateTexture2D(
			&depthTextureDesc,
			nullptr,
			depthTexture.GetAddressOf()
		)
	);

	ThrowIfFailed(
		m_device->CreateDepthStencilView(
			depthTexture.Get(),
			nullptr,
			m_depthStencilView.GetAddressOf()
		)
	);
}

void DeviceResources::ThrowIfFailed(HRESULT result) {
	if (FAILED(result)) {
		throw std::runtime_error("A Direct3D operation failed.");
	}
}