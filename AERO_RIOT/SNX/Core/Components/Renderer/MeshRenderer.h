#pragma once

#include <SNX/Core/Components/Renderer/Renderer.h>
#include <SNX/Core/ModelObject3D.h>

#include <CommonStates.h>
#include <Effects.h>
#include <SimpleMath.h>

#include <d3d11.h>

#include <memory>
#include <string>

enum class MeshFileFormat { CMO, SDKMesh };

class MeshRenderer final : public Renderer {
public:
	MeshRenderer(
		GameObject& gameObject,
		ID3D11Device* device,
		std::shared_ptr<DirectX::CommonStates> commonStates,
		std::shared_ptr<DirectX::IEffectFactory> effectFactory,
		std::wstring meshPath,
		MeshFileFormat fileFormat
	);

	void ConfigureEffects(
		const DirectX::SimpleMath::Vector3& fogColor,
		float fogStart,
		float fogEnd
	);

	[[nodiscard]]
	bool IsLoaded() const noexcept { return m_mesh.IsLoaded(); }

	[[nodiscard]]
	bool HasLoadFailed() const noexcept { return m_loadFailed; }

protected:
	void OnInitialize() override;

	void Draw(const RenderContext& context) override;

	void OnDestroy() override;

private:
	ID3D11Device* m_device = nullptr;

	std::shared_ptr<DirectX::CommonStates> m_commonStates;
	std::shared_ptr<DirectX::IEffectFactory> m_effectFactory;

	std::wstring m_meshPath;

	MeshFileFormat m_fileFormat;

	ModelObject3D m_mesh;

	DirectX::SimpleMath::Vector3 m_fogColor = DirectX::SimpleMath::Vector3::One;

	float m_fogStart = 0.0f;
	float m_fogEnd = 1.0f;

	bool m_configureEffects = false;
	bool m_loadFailed = false;
};