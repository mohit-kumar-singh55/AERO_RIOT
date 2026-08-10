#include "InputManager.h"

#include <stdexcept>

void InputManager::Initialize(HWND window) {
	if (!window)
		throw std::invalid_argument("InputManager requires a valid window");

	if (!m_isInitialized) {
		m_keyboard = std::make_unique<DirectX::Keyboard>();
		m_mouse = std::make_unique<DirectX::Mouse>();

		m_isInitialized = true;
	}

	m_mouse->SetWindow(window);

	// without resetting after the application regains focus, 
	// an old press could sometimes be interpreted as a new action
	Reset();
}

void InputManager::Shutdown() noexcept {
	Reset();

	m_mouse.reset();
	m_keyboard.reset();

	m_isInitialized = false;
}

void InputManager::Update() {
	if (!m_isInitialized)
		throw std::logic_error("InputManager must be initialized before Update");

	// read keyboard only once for this frame
	m_keyboardState = m_keyboard->GetState();
	m_keyboardTracker.Update(m_keyboardState);

	// this is especially important in relative mode:
	// GetState should only be called once per frame
	m_mouseState = m_mouse->GetState();
	m_mouseTracker.Update(m_mouseState);

	// DXTK accumulates the wheel value until reset
	// cache it for this frame, then reset the device value
	m_scrollWheelDelta = m_mouseState.scrollWheelValue;

	m_mouse->ResetScrollWheelValue();
}

void InputManager::Reset() noexcept {
	m_keyboardState = {};
	m_mouseState = {};

	m_keyboardTracker.Reset();
	m_mouseTracker.Reset();

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
