#include "MainScene.h"

#include <SNX/Core/Components/Camera/Camera.h>
#include <SNX/Core/Object/GameObject.h>

#include <SNX/Core/Components/Renderer/PrimitiveRenderer.h>

#include <SNX/Graphics/DeviceResources.h>
#include <SNX/Input/InputManager.h>

#include <Game/Gameplay/Aircraft/Aircraft.h>
#include <Game/Gameplay/Aircraft/AircraftController.h>

#include <DirectXColors.h>
#include <Keyboard.h>
#include <SimpleMath.h>
#include <SpriteFont.h>

#include <string>

MainScene::MainScene(SceneManager& sceneManager, SceneContext& context) noexcept :
	Scene(sceneManager, context) {}

void MainScene::OnLoad() {
	using DirectX::SimpleMath::Vector3;

	auto& context = GetContext();
	auto& input = InputManager::Get();

	input.SetMouseMode(DirectX::Mouse::MODE_ABSOLUTE);
	input.Reset();

	// ! create default camera
	GameObject& cameraObject = GetGameObjects().CreateGameObject("Main Camera");
	Camera& camera = cameraObject.AddComponent<Camera>();

	const float aspect =
		static_cast<float>(context.deviceResources.GetWidth()) /
		static_cast<float>(context.deviceResources.GetHeight());

	camera.SetPerspective(60.0f, aspect, 0.1f, 1000.0f);
	camera.LookAt(Vector3(0.0f, 4.0f, 8.0f), Vector3::Zero);

	m_camera = &camera;

	// ! create basic aircraft hierarchy
	GameObject& aircraftRoot = GetGameObjects().CreateGameObject("AircraftRoot");
	GameObject& aircraftBody = GetGameObjects().CreateGameObject("AircraftBody");
	GameObject& aircraftBase = GetGameObjects().CreateGameObject("AircraftBase");
	GameObject& aircraftWing = GetGameObjects().CreateGameObject("AircraftWing");

	aircraftRoot.AddComponent<Aircraft>();
	aircraftRoot.AddComponent<AircraftController>();

	Transform& bodyTransform = aircraftBody.GetTransform();
	Transform& baseTransform = aircraftBase.GetTransform();
	Transform& wingTransform = aircraftWing.GetTransform();

	bodyTransform.SetParent(&aircraftRoot.GetTransform(), false);
	baseTransform.SetParent(&bodyTransform, false);
	wingTransform.SetParent(&bodyTransform, false);

	baseTransform.SetLocalScale({ 1.0f,5.0f,1.0f });
	baseTransform.RotateEulerDegrees({ -90.0f,0.0f,0.0f });

	wingTransform.SetLocalScale({ 5.0f,0.1f,0.8f });

	auto& baseRenderer = aircraftBase.AddComponent<PrimitiveRenderer>(
		context.deviceResources.GetContext(),
		PrimitiveShape::Cone
	);

	auto& wingRenderer = aircraftWing.AddComponent<PrimitiveRenderer>(
		context.deviceResources.GetContext(),
		PrimitiveShape::Cube
	);

	baseRenderer.SetColor({ 1.0f,0.5f,0.0f,1.0f });
	wingRenderer.SetColor({ 0.0f,0.5f,1.0f,1.0f });

	m_aircraftRoot = &aircraftRoot;

	camera.LookAtFromCurrentPosition(aircraftRoot.GetTransform().GetPosition());
}

void MainScene::OnUnload() {
	m_camera = nullptr;
	m_aircraftRoot = nullptr;
}

void MainScene::OnUpdate() {
	// ! close the window
	if (InputManager::Get().IsKeyPressed(DirectX::Keyboard::Escape))
		RequestQuit();
}

bool MainScene::BuildRenderContext(RenderContext& context) const noexcept {
	if (!m_camera)
		return false;

	// ! supply camera info
	context.view = m_camera->GetView();
	context.projection = m_camera->GetProjection();
	context.cameraPosition = m_camera->GetPosition();

	return context.IsValid();
}

void MainScene::OnRenderUI() {
	// ! simple temp UI
	GetContext().font.DrawString(
		&GetContext().spriteBatch,
		L"SNX TEMPLATE",
		DirectX::SimpleMath::Vector2(
			20.0f,
			20.0f
		),
		DirectX::Colors::White
	);

	GetContext().font.DrawString(
		&GetContext().spriteBatch,
		L"ESC : Quit",
		DirectX::SimpleMath::Vector2(
			20.0f,
			60.0f
		),
		DirectX::Colors::White
	);

	if (InputManager::Get().IsGamePadConnected()) {
		auto& input = InputManager::Get();

		GetContext().font.DrawString(
			&GetContext().spriteBatch,
			std::to_wstring(input.GetGamePadTrigger(GamePadTrigger::Right)).c_str(),
			DirectX::SimpleMath::Vector2(
				20.0f,
				100.0f
			),
			DirectX::Colors::Green
		);
	}
}