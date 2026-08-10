#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <Keyboard.h>
#include <Mouse.h>
#include <SimpleMath.h>

#include <memory>

enum class MouseButton {
	Left,
	Middle,
	Right,
	X1,
	X2
};

/* Singleton class for input management */
class InputManager final {
public:
	static InputManager& Get() noexcept {
		// created the first time Get() is called
		static InputManager instance;
		return instance;
	}

	void Initialize(HWND window);
	void Shutdown() noexcept;
	void Update();
	void Reset() noexcept;

	void SetMouseMode(DirectX::Mouse::Mode mode);

	/// <summary>
	/// true every frame while holding
	/// </summary>
	/// <param name="key"></param>
	/// <returns></returns>
	[[nodiscard]]
	bool IsKeyDown(DirectX::Keyboard::Keys key) const noexcept;

	/// <summary>
	/// true only the first frame when pressed
	/// </summary>
	/// <param name="key"></param>
	/// <returns></returns>
	[[nodiscard]]
	bool IsKeyPressed(DirectX::Keyboard::Keys key) const noexcept;

	/// <summary>
	/// true only one frame when released
	/// </summary>
	/// <param name="key"></param>
	/// <returns></returns>
	[[nodiscard]]
	bool IsKeyReleased(DirectX::Keyboard::Keys key) const noexcept;

	/// <summary>
	/// true every frame while holding
	/// </summary>
	/// <param name="button"></param>
	/// <returns></returns>
	[[nodiscard]]
	bool IsMouseButtonDown(MouseButton button) const noexcept;

	/// <summary>
	/// true only the first frame when pressed
	/// </summary>
	/// <param name="button"></param>
	/// <returns></returns>
	[[nodiscard]]
	bool IsMouseButtonPressed(MouseButton button) const noexcept;

	/// <summary>
	/// true only one frame when released
	/// </summary>
	/// <param name="button"></param>
	/// <returns></returns>
	[[nodiscard]]
	bool IsMouseButtonReleased(MouseButton button) const noexcept;

	/// <summary>
	/// cursor position in ABSOLUTE MODE
	/// </summary>
	/// <returns></returns>
	[[nodiscard]]
	DirectX::SimpleMath::Vector2 GetMousePosition() const noexcept;

	/// <summary>
	/// cursor movement since the previous read in RELATIVE MOVE
	/// </summary>
	/// <returns></returns>
	[[nodiscard]]
	DirectX::SimpleMath::Vector2 GetMouseDelta() const noexcept;

	[[nodiscard]]
	int GetScrollWheelDelta() const noexcept { return m_scrollWheelDelta; }

	[[nodiscard]]
	bool IsRelativeMouseMode() const noexcept {
		return m_mouseState.positionMode == DirectX::Mouse::MODE_RELATIVE;
	}

	[[nodiscard]]
	const DirectX::Keyboard::State& GetKeyboardState() const noexcept { return m_keyboardState; }

	[[nodiscard]]
	const DirectX::Mouse::State& GetMouseState() const noexcept { return m_mouseState; }

	[[nodiscard]]
	const DirectX::Keyboard::KeyboardStateTracker& GetKeyboardTracker() const noexcept {
		return m_keyboardTracker;
	}

	[[nodiscard]]
	const DirectX::Mouse::ButtonStateTracker& GetMouseTracker() const noexcept {
		return m_mouseTracker;
	}

private:
	InputManager() = default;
	~InputManager() = default;

	// disallow to copy or move
	InputManager(const InputManager&) = delete;
	InputManager& operator=(const InputManager&) = delete;
	InputManager(InputManager&&) = delete;
	InputManager& operator=(InputManager&&) = delete;

private:
	std::unique_ptr<DirectX::Keyboard> m_keyboard;
	std::unique_ptr<DirectX::Mouse> m_mouse;

	DirectX::Keyboard::State m_keyboardState{};
	DirectX::Mouse::State m_mouseState{};

	DirectX::Keyboard::KeyboardStateTracker m_keyboardTracker;
	DirectX::Mouse::ButtonStateTracker m_mouseTracker;

	int m_scrollWheelDelta = 0;

	bool m_isInitialized = false;
};