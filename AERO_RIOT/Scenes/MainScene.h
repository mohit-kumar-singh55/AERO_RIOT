#pragma once

#include <SNX/Core/Scene/Scene.h>

class Camera;

class MainScene final : public Scene {
public:
	MainScene(SceneManager& sceneManager, SceneContext& context) noexcept;

protected:
	void OnLoad() override;
	void OnUnload() override;

	void OnUpdate() override;

	bool BuildRenderContext(RenderContext& context) const noexcept override;

	void OnRenderUI() override;

private:
	Camera* m_camera = nullptr;

	GameObject* m_aircraftRoot = nullptr;
};