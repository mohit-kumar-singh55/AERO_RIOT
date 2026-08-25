#include "InputManager.h"

#include <stdexcept>

void InputManager::OnAppActivationChanged(bool active) {
	m_isActive = active;

	if (!m_gamepad) return;

	/*
	* unlike keyboard or mouse,
	* gamepad could receive global input
	* so even if the game window is not in focus,
	* there is a chance that its handling the input
	* so suspend the gamepad until the window is not in focus again
	*/
	active ? m_gamepad->Resume() : m_gamepad->Suspend();

	Reset();
}

void InputManager::Initialize(HWND window) {
	if (!window)
		throw std::invalid_argument("InputManager requires a valid window");

	if (!m_isInitialized) {
		m_keyboard = std::make_unique<DirectX::Keyboard>();
		m_mouse = std::make_unique<DirectX::Mouse>();
		m_gamepad = std::make_unique<DirectX::GamePad>();

		m_isInitialized = true;
	}

	m_mouse->SetWindow(window);

	/*
	* there is a chance that:
	*	- window is inactive but gamepad is still active
	*	- window is active but gamepad is still inactive
	*/
	if (m_gamepad)
		m_isActive ? m_gamepad->Resume() : m_gamepad->Suspend();

	/*
	* without resetting after the application regains focus,
	* an old press could sometimes be interpreted as a new action
	*/
	Reset();
}

void InputManager::Shutdown() noexcept {
	Reset();

	m_mouse.reset();
	m_keyboard.reset();
	m_gamepad.reset();

	m_isActive = false;
	m_isInitialized = false;
}

void InputManager::Update() {
	if (!m_isInitialized)
		throw std::logic_error("InputManager must be initialized before Update");

	// window is not in focus
	if (!m_isActive) return;

	// read keyboard only once for this frame
	m_keyboardState = m_keyboard->GetState();
	m_keyboardTracker.Update(m_keyboardState);

	// this is especially important in relative mode:
	// GetState should only be called once per frame
	m_mouseState = m_mouse->GetState();
	m_mouseTracker.Update(m_mouseState);

	// ! read gamepad only for one controller (only for now)
	m_gamepadState = m_gamepad->GetState(0, DirectX::GamePad::DEAD_ZONE_CIRCULAR);
	m_isGamePadConnected = m_gamepadState.connected;
	if (m_isGamePadConnected)
		m_gamepadTracker.Update(m_gamepadState);
	else
		m_gamepadTracker.Reset();

	// DXTK accumulates the wheel value until reset
	// cache it for this frame, then reset the device value
	m_scrollWheelDelta = m_mouseState.scrollWheelValue;

	m_mouse->ResetScrollWheelValue();
}

void InputManager::Reset() noexcept {
	m_keyboardState = {};
	m_mouseState = {};
	m_gamepadState = {};

	m_keyboardTracker.Reset();
	m_mouseTracker.Reset();
	m_gamepadTracker.Reset();

	m_isGamePadConnected = false;

	m_scrollWheelDelta = 0;

	if (m_mouse)
		m_mouse->ResetScrollWheelValue();
}

void InputManager::SetMouseMode(DirectX::Mouse::Mode mode) {
	if (!m_isInitialized || !m_mouse)
		throw std::logic_error("InputManager must be initialized before setting mouse mode");

	m_mouse->SetMode(mode);
}

bool InputManager::IsKeyDown(DirectX::Keyboard::Keys key) const noexcept {
	return m_keyboardState.IsKeyDown(key);
}

bool InputManager::IsKeyPressed(DirectX::Keyboard::Keys key) const noexcept {
	return m_keyboardTracker.IsKeyPressed(key);
}

bool InputManager::IsKeyReleased(DirectX::Keyboard::Keys key) const noexcept {
	return m_keyboardTracker.IsKeyReleased(key);
}

bool InputManager::IsMouseButtonDown(MouseButton button) const noexcept {
	switch (button) {
	case MouseButton::Left:
		return m_mouseState.leftButton;
	case MouseButton::Middle:
		return m_mouseState.middleButton;
	case MouseButton::Right:
		return m_mouseState.rightButton;
	case MouseButton::X1:
		return m_mouseState.xButton1;
	case MouseButton::X2:
		return m_mouseState.xButton2;
	default:
		return false;
	}
}

bool InputManager::IsMouseButtonPressed(MouseButton button) const noexcept {
	using ButtonState = DirectX::Mouse::ButtonStateTracker::ButtonState;

	switch (button) {
	case MouseButton::Left:
		return m_mouseTracker.leftButton == ButtonState::PRESSED;
	case MouseButton::Middle:
		return m_mouseTracker.middleButton == ButtonState::PRESSED;
	case MouseButton::Right:
		return m_mouseTracker.rightButton == ButtonState::PRESSED;
	case MouseButton::X1:
		return m_mouseTracker.xButton1 == ButtonState::PRESSED;
	case MouseButton::X2:
		return m_mouseTracker.xButton2 == ButtonState::PRESSED;
	default:
		return false;
	}
}

bool InputManager::IsMouseButtonReleased(MouseButton button) const noexcept {
	using ButtonState = DirectX::Mouse::ButtonStateTracker::ButtonState;

	switch (button) {
	case MouseButton::Left:
		return m_mouseTracker.leftButton == ButtonState::RELEASED;
	case MouseButton::Middle:
		return m_mouseTracker.middleButton == ButtonState::RELEASED;
	case MouseButton::Right:
		return m_mouseTracker.rightButton == ButtonState::RELEASED;
	case MouseButton::X1:
		return m_mouseTracker.xButton1 == ButtonState::RELEASED;
	case MouseButton::X2:
		return m_mouseTracker.xButton2 == ButtonState::RELEASED;
	default:
		return false;
	}
}

DirectX::SimpleMath::Vector2 InputManager::GetMousePosition() const noexcept {
	if (m_mouseState.positionMode != DirectX::Mouse::MODE_ABSOLUTE)
		return DirectX::SimpleMath::Vector2::Zero;

	return {
		static_cast<float>(m_mouseState.x),
		static_cast<float>(m_mouseState.y)
	};
}

DirectX::SimpleMath::Vector2 InputManager::GetMouseDelta() const noexcept {
	if (m_mouseState.positionMode != DirectX::Mouse::MODE_RELATIVE)
		return DirectX::SimpleMath::Vector2::Zero;

	return {
		static_cast<float>(m_mouseState.x),
		static_cast<float>(m_mouseState.y)
	};
}

bool InputManager::IsGamePadButtonDown(GamePadButton button) const noexcept {
	switch (button) {
	case GamePadButton::A:
		return m_gamepadState.buttons.a;
	case GamePadButton::B:
		return m_gamepadState.buttons.b;
	case GamePadButton::X:
		return m_gamepadState.buttons.x;
	case GamePadButton::Y:
		return m_gamepadState.buttons.y;
	case GamePadButton::Menu:
		return m_gamepadState.buttons.menu;
	case GamePadButton::Back:
		return m_gamepadState.buttons.back;
	case GamePadButton::Start:
		return m_gamepadState.buttons.start;
	case GamePadButton::View:
		return m_gamepadState.buttons.view;
	case GamePadButton::LeftShoulder:
		return m_gamepadState.buttons.leftShoulder;
	case GamePadButton::RightShoulder:
		return m_gamepadState.buttons.rightShoulder;
	case GamePadButton::LeftStick:
		return m_gamepadState.buttons.leftStick;
	case GamePadButton::RightStick:
		return m_gamepadState.buttons.rightStick;
	case GamePadButton::DPadLeft:
		return m_gamepadState.dpad.left;
	case GamePadButton::DPadRight:
		return m_gamepadState.dpad.right;
	case GamePadButton::DPadUp:
		return m_gamepadState.dpad.up;
	case GamePadButton::DPadDown:
		return m_gamepadState.dpad.down;
	default:
		return false;
	}
}

bool InputManager::IsGamePadButtonPressed(GamePadButton button) const noexcept {
	using ButtonState = DirectX::GamePad::ButtonStateTracker::ButtonState;

	switch (button) {
	case GamePadButton::A:
		return m_gamepadTracker.a == ButtonState::PRESSED;
	case GamePadButton::B:
		return m_gamepadTracker.b == ButtonState::PRESSED;
	case GamePadButton::X:
		return m_gamepadTracker.x == ButtonState::PRESSED;
	case GamePadButton::Y:
		return m_gamepadTracker.y == ButtonState::PRESSED;
	case GamePadButton::Menu:
		return m_gamepadTracker.menu == ButtonState::PRESSED;
	case GamePadButton::Back:
		return m_gamepadTracker.back == ButtonState::PRESSED;
	case GamePadButton::Start:
		return m_gamepadTracker.start == ButtonState::PRESSED;
	case GamePadButton::View:
		return m_gamepadTracker.view == ButtonState::PRESSED;
	case GamePadButton::LeftShoulder:
		return m_gamepadTracker.leftShoulder == ButtonState::PRESSED;
	case GamePadButton::RightShoulder:
		return m_gamepadTracker.rightShoulder == ButtonState::PRESSED;
	case GamePadButton::LeftStick:
		return m_gamepadTracker.leftStick == ButtonState::PRESSED;
	case GamePadButton::RightStick:
		return m_gamepadTracker.rightStick == ButtonState::PRESSED;
	case GamePadButton::DPadLeft:
		return m_gamepadTracker.dpadLeft == ButtonState::PRESSED;
	case GamePadButton::DPadRight:
		return m_gamepadTracker.dpadRight == ButtonState::PRESSED;
	case GamePadButton::DPadUp:
		return m_gamepadTracker.dpadUp == ButtonState::PRESSED;
	case GamePadButton::DPadDown:
		return m_gamepadTracker.dpadDown == ButtonState::PRESSED;
	default:
		return false;
	}
}

bool InputManager::IsGamePadButtonReleased(GamePadButton button) const noexcept {
	using ButtonState = DirectX::GamePad::ButtonStateTracker::ButtonState;

	switch (button) {
	case GamePadButton::A:
		return m_gamepadTracker.a == ButtonState::RELEASED;
	case GamePadButton::B:
		return m_gamepadTracker.b == ButtonState::RELEASED;
	case GamePadButton::X:
		return m_gamepadTracker.x == ButtonState::RELEASED;
	case GamePadButton::Y:
		return m_gamepadTracker.y == ButtonState::RELEASED;
	case GamePadButton::Menu:
		return m_gamepadTracker.menu == ButtonState::RELEASED;
	case GamePadButton::Back:
		return m_gamepadTracker.back == ButtonState::RELEASED;
	case GamePadButton::Start:
		return m_gamepadTracker.start == ButtonState::RELEASED;
	case GamePadButton::View:
		return m_gamepadTracker.view == ButtonState::RELEASED;
	case GamePadButton::LeftShoulder:
		return m_gamepadTracker.leftShoulder == ButtonState::RELEASED;
	case GamePadButton::RightShoulder:
		return m_gamepadTracker.rightShoulder == ButtonState::RELEASED;
	case GamePadButton::LeftStick:
		return m_gamepadTracker.leftStick == ButtonState::RELEASED;
	case GamePadButton::RightStick:
		return m_gamepadTracker.rightStick == ButtonState::RELEASED;
	case GamePadButton::DPadLeft:
		return m_gamepadTracker.dpadLeft == ButtonState::RELEASED;
	case GamePadButton::DPadRight:
		return m_gamepadTracker.dpadRight == ButtonState::RELEASED;
	case GamePadButton::DPadUp:
		return m_gamepadTracker.dpadUp == ButtonState::RELEASED;
	case GamePadButton::DPadDown:
		return m_gamepadTracker.dpadDown == ButtonState::RELEASED;
	default:
		return false;
	}
}

DirectX::SimpleMath::Vector2 InputManager::GetGamePadStick(GamePadStick stick) const noexcept {
	switch (stick) {
	case GamePadStick::LeftStick:
		return {
			m_gamepadState.thumbSticks.leftX,
			m_gamepadState.thumbSticks.leftY
		};
	case GamePadStick::RightStick:
		return {
			m_gamepadState.thumbSticks.rightX,
			m_gamepadState.thumbSticks.rightY
		};
	default:
		return  DirectX::SimpleMath::Vector2::Zero;
	}
}

float InputManager::GetGamePadTrigger(GamePadTrigger trigger) const noexcept {
	switch (trigger) {
	case GamePadTrigger::Left:
		return m_gamepadState.triggers.left;
	case GamePadTrigger::Right:
		return m_gamepadState.triggers.right;
	default:
		return 0.0f;
	}
}