#include "MainScene.h"

#include <SNX/Core/Components/Camera/Camera.h>
#include <SNX/Core/Object/GameObject.h>

#include <SNX/Core/Components/Renderer/PrimitiveRenderer.h>

#include <SNX/Graphics/DeviceResources.h>
#include <SNX/Input/InputManager.h>

#include <DirectXColors.h>
#include <Keyboard.h>
#include <SimpleMath.h>
#include <SpriteFont.h>

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

	Transform& bodyTransform = aircraftBody.GetTransform();
	bodyTransform.SetParent(&aircraftRoot.GetTransform(), false);
	bodyTransform.SetLocalScale({ 1.0f,5.0f,1.0f });
	bodyTransform.RotateEulerDegrees({ -90.0f,0.0f,0.0f });

	auto& renderer = aircraftBody.AddComponent<PrimitiveRenderer>(
		context.deviceResources.GetContext(),
		PrimitiveShape::Cone
	);

	renderer.SetColor({ 1.0f,0.5f,0.0f,1.0f });

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
}