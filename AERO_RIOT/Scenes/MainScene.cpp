#include "pch.h"

#include "MainScene.h"

#include <SNX/Core/Components/Camera/Camera.h>
#include <SNX/Core/Object/GameObject.h>

#include <SNX/Core/Components/Renderer/PrimitiveRenderer.h>
#include <SNX/Core/Components/Kinetics/KineticBody.h>
#include <SNX/Core/Components/Collider/SphereCollider.h>
#include <SNX/Core/Components/Collider/BoxCollider.h>
#include <SNX/Core/Materials/Primitive/UnlitMaterial.h>
#include <SNX/Core/Components/Renderer/TrailRenderer.h>

#include <SNX/Graphics/DeviceResources.h>
#include <SNX/Input/InputManager.h>

#include <Game/Gameplay/Aircraft/Aircraft.h>
#include <Game/Gameplay/Aircraft/AircraftController.h>
#include <Game/Gameplay/Aircraft/AircraftKinetics.h>
#include <Game/Gameplay/Cameras/AircraftCameraController.h>
#include <Game/Gameplay/Weapon/WeaponController.h>

#include <DirectXColors.h>
#include <Keyboard.h>
#include <SimpleMath.h>
#include <SpriteFont.h>

#include <time.h>
#include <memory>

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
	GameObject& gunMuzzle = GetGameObjects().CreateGameObject("GunMuzzle");
	GameObject& thirdPersonCameraAnchor = GetGameObjects().CreateGameObject("ThirdPersonCameraAnchor");
	GameObject& engineExhaust = GetGameObjects().CreateGameObject("EngineExhaustGlow");
	GameObject& wingTipLeft = GetGameObjects().CreateGameObject("WingTipLeft");
	GameObject& wingTipRight = GetGameObjects().CreateGameObject("WingTipRight");

	Transform& bodyTransform = aircraftBody.GetTransform();
	Transform& baseTransform = aircraftBase.GetTransform();
	Transform& wingTransform = aircraftWing.GetTransform();
	Transform& tpcaTransform = thirdPersonCameraAnchor.GetTransform();
	Transform& gunMuzzleTransform = gunMuzzle.GetTransform();
	Transform& engineExhaustTransform = engineExhaust.GetTransform();
	Transform& wingTipLeftTransform = wingTipLeft.GetTransform();
	Transform& wingTipRightTransform = wingTipRight.GetTransform();

	bodyTransform.SetParent(&aircraftRoot.GetTransform(), false);
	baseTransform.SetParent(&bodyTransform, false);
	wingTransform.SetParent(&bodyTransform, false);
	tpcaTransform.SetParent(&aircraftRoot.GetTransform(), false);
	gunMuzzleTransform.SetParent(&bodyTransform, false);
	engineExhaustTransform.SetParent(&bodyTransform, false);
	wingTipLeftTransform.SetParent(&wingTransform, false);
	wingTipRightTransform.SetParent(&wingTransform, false);

	aircraftRoot.AddComponent<Aircraft>();
	aircraftRoot.AddComponent<AircraftController>();
	aircraftRoot.AddComponent<KineticBody>();
	aircraftRoot.AddComponent<AircraftKinetics>();
	aircraftRoot.AddComponent<WeaponController>(&gunMuzzleTransform);

	tpcaTransform.SetLocalPosition({ 0.0f,0.0f,0.0f });

	baseTransform.SetLocalScale({ 1.0f,5.0f,1.0f });
	baseTransform.RotateEulerDegrees({ -90.0f,0.0f,0.0f });

	wingTransform.SetLocalScale({ 5.0f,0.1f,0.8f });

	gunMuzzleTransform.SetLocalPosition({ 0.0f,0.0f,-2.5f });

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

	engineExhaustTransform.SetPosition({ 0.0f, 0.0f, 2.5f });
	engineExhaustTransform.SetLocalScale({ 0.9f, 0.9f, 0.9f });

	auto& exhaustTrailRenderer = engineExhaust.AddComponent<TrailRenderer>();
	GradientKey exhaustTrailGradientkeys[] = {
		// tail
		{ 0.00f, { 0.70f, 0.00f, 0.00f, 0.00f } }, // transparent dark red
		{ 0.12f, { 1.00f, 0.03f, 0.00f, 0.25f } }, // red
		{ 0.30f, { 1.00f, 0.12f, 0.00f, 0.60f } }, // red-orange
		{ 0.48f, { 1.00f, 0.42f, 0.02f, 0.90f } }, // orange
		{ 0.63f, { 1.00f, 0.90f, 0.15f, 1.00f } }, // yellow
		{ 0.74f, { 1.00f, 1.00f, 0.65f, 1.00f } }, // hot yellow/white
		{ 0.86f, { 0.10f, 0.65f, 1.00f, 0.80f } }, // electric blue
		{ 1.00f, { 0.55f, 0.95f, 1.00f, 0.00f } }, // blue-white core near engine
	};
	exhaustTrailRenderer.GetGradient().SetKeys(exhaustTrailGradientkeys);

	auto engineGlowMaterial = std::make_shared<UnlitMaterial>();

	engineGlowMaterial->SetPixelShader("EngineGlowPS");
	engineGlowMaterial->Initialize(
		context.deviceResources.GetDevice(),
		context.deviceResources.GetContext()
	);

	auto& engineExhaustRenderer = engineExhaust.AddComponent<PrimitiveRenderer>(
		context.deviceResources.GetContext(),
		PrimitiveShape::Sphere,
		engineGlowMaterial
	);
	engineExhaustRenderer.SetColor({ 0.0f, 1.0f, 1.0f, 1.0f });
	engineExhaustRenderer.SetEmissiveColor({ 1.0f, 0.0f, 0.0f });

	wingTipLeftTransform.SetPosition({ -2.5f,0.0f,0.35f });
	wingTipRightTransform.SetPosition({ 2.5f,0.0f,0.35f });

	auto& wingTipLeftTrailRenderer = wingTipLeft.AddComponent<TrailRenderer>();
	auto& wingTipRightTrailRenderer = wingTipRight.AddComponent<TrailRenderer>();
	wingTipLeftTrailRenderer.SetTrailWidth(0.05f);
	wingTipRightTrailRenderer.SetTrailWidth(0.05f);
	GradientKey WingTipTrailGradientkeys[] = {
	{ 0.00f, { 0.25f, 0.45f, 0.70f, 0.00f } }, // invisible cool-blue tail
	{ 0.15f, { 0.35f, 0.65f, 0.95f, 0.18f } }, // faint blue
	{ 0.40f, { 0.55f, 0.85f, 1.00f, 0.40f } }, // light cyan
	{ 0.70f, { 0.80f, 0.95f, 1.00f, 0.65f } }, // pale cyan-white
	{ 1.00f, { 1.00f, 1.00f, 1.00f, 0.85f } }, // bright white at wing tip
	};
	wingTipLeftTrailRenderer.GetGradient().SetKeys(WingTipTrailGradientkeys);
	wingTipRightTrailRenderer.GetGradient().SetKeys(WingTipTrailGradientkeys);

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

	// ! supply camera info only
	context.view = m_camera->GetView();
	context.projection = m_camera->GetProjection();
	context.cameraPosition = m_camera->GetPosition();

	return context.IsValid();
}

void MainScene::OnRenderUI() {
	// ! temp UI
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
}