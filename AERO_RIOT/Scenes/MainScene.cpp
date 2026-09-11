#include "MainScene.h"

#include <SNX/Core/Components/Camera/Camera.h>
#include <SNX/Core/Object/GameObject.h>

#include <SNX/Core/Components/Renderer/PrimitiveRenderer.h>
#include <SNX/Core/Components/Kinetics/KineticBody.h>

#include <SNX/Graphics/DeviceResources.h>
#include <SNX/Input/InputManager.h>

#include <Game/Gameplay/Aircraft/Aircraft.h>
#include <Game/Gameplay/Aircraft/AircraftController.h>
#include <Game/Gameplay/Cameras/AircraftCameraController.h>

#include <DirectXColors.h>
#include <Keyboard.h>
#include <SimpleMath.h>
#include <SpriteFont.h>

#include <string>
#include <time.h>

MainScene::MainScene(SceneManager& sceneManager, SceneContext& context) noexcept :
	Scene(sceneManager, context) {}

void MainScene::OnLoad() {
	using DirectX::SimpleMath::Vector3;

	auto& context = GetContext();
	auto& input = InputManager::Get();

	input.SetMouseMode(DirectX::Mouse::MODE_ABSOLUTE);
	input.Reset();

	// ! create basic aircraft hierarchy
	GameObject& aircraftRoot = GetGameObjects().CreateGameObject("AircraftRoot");
	GameObject& aircraftBody = GetGameObjects().CreateGameObject("Body");
	GameObject& aircraftBase = GetGameObjects().CreateGameObject("Base");
	GameObject& aircraftWing = GetGameObjects().CreateGameObject("Wing");
	GameObject& thirdPersonCameraAnchor = GetGameObjects().CreateGameObject("ThirdPersonCameraAnchor");

	aircraftRoot.AddComponent<Aircraft>();
	aircraftRoot.AddComponent<AircraftController>();
	aircraftRoot.AddComponent<KineticBody>();

	Transform& bodyTransform = aircraftBody.GetTransform();
	Transform& baseTransform = aircraftBase.GetTransform();
	Transform& wingTransform = aircraftWing.GetTransform();
	Transform& tpcaTransform = thirdPersonCameraAnchor.GetTransform();

	bodyTransform.SetParent(&aircraftRoot.GetTransform(), false);
	baseTransform.SetParent(&bodyTransform, false);
	wingTransform.SetParent(&bodyTransform, false);
	tpcaTransform.SetParent(&aircraftRoot.GetTransform(), false);

	tpcaTransform.SetLocalPosition({ 0.0f,0.0f,0.0f });

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

	// ! create default camera
	GameObject& cameraObject = GetGameObjects().CreateGameObject("AircraftCamera");
	Camera& camera = cameraObject.AddComponent<Camera>();
	cameraObject.AddComponent<AircraftCameraController>(&tpcaTransform);

	const float aspect =
		static_cast<float>(context.deviceResources.GetWidth()) /
		static_cast<float>(context.deviceResources.GetHeight());

	camera.SetPerspective(60.0f, aspect, 0.1f, 1000.0f);

	m_camera = &camera;

	std::srand(std::time(NULL));

	// ? just for debugging purpose
	for (int i = 0;i < 50;i++) {
		auto& cube = GetGameObjects().CreateGameObject("DEBUG_CUBE" + i);
		auto& renderer = cube.AddComponent<PrimitiveRenderer>(
			context.deviceResources.GetContext(),
			PrimitiveShape::Cube
		);
		renderer.SetColor({ 0.5f,0.2f,0.7f,1.0f });
		auto& cubeTrans = cube.GetTransform();
		cubeTrans.SetScale({ 0.2f,4.0f,20.0f });
		cubeTrans.SetPosition({ (float)(std::rand() % 10) - i,-(float)(std::rand() % 10) + i,-(float)(std::rand() % 20) - i });
	}
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
		L"AERO RIOT",
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