#pragma once

#define WIN32_LEAN_AND_MEAN

// window related
#include <windows.h>
#include <wrl/client.h>

#include <d3d11.h>
#include <dxgi.h>

class DeviceResources final {
public:
	DeviceResources() = default;
	~DeviceResources();

	// disallowing to copy or move 
	// as this class contains graphic/window related code/pointers
	DeviceResources(const DeviceResources&) = delete;
	DeviceResources& operator=(const DeviceResources&) = delete;
	DeviceResources(DeviceResources&&) = delete;
	DeviceResources& operator=(DeviceResources&&) = delete;

	void Initialize(HWND window, int width, int height);

	void BeginFrame(const float clearColor[4]);

	void Present(bool useVSync = true);

	[[nodiscard]]
	ID3D11Device* GetDevice() const noexcept { return m_device.Get(); }

	[[nodiscard]]
	ID3D11DeviceContext* GetContext() const noexcept { return m_context.Get(); }

	[[nodiscard]]
	IDXGISwapChain* GetSwapChain() const noexcept { return m_swapChain.Get(); }

	[[nodiscard]]
	HWND GetWindow() const noexcept { return m_window; }

	[[nodiscard]]
	int GetWidth() const noexcept { return m_width; }

	[[nodiscard]]
	int GetHeight() const noexcept { return m_height; }

	[[nodiscard]]
	D3D_FEATURE_LEVEL GetFeatureLevel() const noexcept { return m_featureLevel; }

private:
	void CreateDeviceAndSwapChain();
	void CreateRenderTarget();
	void CreateDepthBuffer();

	static void ThrowIfFailed(HRESULT result);

private:
	HWND m_window = nullptr;				// window handle, needs a window before we can render it

	int m_width = 0;
	int m_height = 0;

	D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;

	Microsoft::WRL::ComPtr<ID3D11Device> m_device;							// device creates GPU resources			(context sends commands to the GPU)
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;					// context sends commands to the GPU		(Context = GPU command sender)
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;						// swap chain owns the screen buffers	(Swap chain = screen buffer manager)
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;		// This is the image we are drawing into	(In Unity terms, it is similar to )
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;		// In 3d, depth buffer decides which object is in front and which object is behind
};