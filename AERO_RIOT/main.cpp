#define WIN32_LEAN_AND_MEAN

#include "Game.h"

// window related
#include <windows.h>

#include <memory>
#include <stdexcept>

#include <filesystem>
#include <array>

#include <Keyboard.h>
#include <Mouse.h>

#include <SNX/Input/InputManager.h>

// ! IMP
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------

HWND g_window = nullptr;								// window handle, needs a window before we can render it

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

std::unique_ptr<Game> g_game;

// -----------------------------------------------------------------------------
// Window Procedure
// -----------------------------------------------------------------------------

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// DirectXTK Keyboard needs every keyboard and mouse message.
	DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
	DirectX::Mouse::ProcessMessage(message, wParam, lParam);

	switch (message)
	{
	case WM_ACTIVATEAPP:
		InputManager::Get().Reset();
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

// -----------------------------------------------------------------------------
// Create Win32 window
// -----------------------------------------------------------------------------

void CreateGameWindow(HINSTANCE instance, int showCommand) {
	const wchar_t CLASS_NAME[] = L"DXTKGameWindowClass";

	// create window class instance
	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = instance;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	RegisterClass(&wc);

	// define window styles
	DWORD windowStyle =
		WS_OVERLAPPED |
		WS_CAPTION |
		WS_SYSMENU |
		WS_MINIMIZEBOX;

	// define window rect
	RECT rect = {};
	rect.left = 0;
	rect.top = 0;
	rect.right = WINDOW_WIDTH;
	rect.bottom = WINDOW_HEIGHT;

	// resize the window
	AdjustWindowRect(&rect, windowStyle, FALSE);

	// calc window size
	int windowWidth = rect.right - rect.left;
	int windowHeight = rect.bottom - rect.top;

	// create the window based on above settings
	g_window = CreateWindowEx(
		0,
		CLASS_NAME,
		L"Prototype Alpha",
		windowStyle,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowWidth,
		windowHeight,
		nullptr,
		nullptr,
		instance,
		nullptr
	);

	// throw error if unable to create window
	if (!g_window)
		throw std::runtime_error("Failed to create window.");

	// finally, draw the window
	ShowWindow(g_window, showCommand);
}

// -----------------------------------------------------------------------------
// Set working directory to the directory where the .exe is present
// -----------------------------------------------------------------------------

void SetWorkingDirectoryToExecutable() {
	std::array<wchar_t, 32768> pathBuffer{};

	const DWORD length = GetModuleFileNameW(nullptr, pathBuffer.data(), static_cast<DWORD>(pathBuffer.size()));

	if (length == 0 || length >= pathBuffer.size())
		throw std::runtime_error("Failed to get executable path.");

	const std::filesystem::path executablePath(pathBuffer.data());

	const std::filesystem::path executableDirectory = executablePath.parent_path();

	std::filesystem::current_path(executableDirectory);
}

// -----------------------------------------------------------------------------
// WinMain
// -----------------------------------------------------------------------------

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
	try {
		SetWorkingDirectoryToExecutable();

		CreateGameWindow(instance, showCommand);

		g_game = std::make_unique<Game>();

		g_game->Initialize(g_window, WINDOW_WIDTH, WINDOW_HEIGHT);

		MSG msg = {};

		while (msg.message != WM_QUIT) {
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
				g_game->Tick();
		}

		// release gameplay, audio and graphics resources
		g_game.reset();
		// InputManager is a singleton, so release its
		// owned devices explicitly
		InputManager::Get().Shutdown();

		return static_cast<int>(msg.wParam);
	}
	catch (const std::exception& e) {
		g_game.reset();
		InputManager::Get().Shutdown();

		MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);

		return -1;
	}
}