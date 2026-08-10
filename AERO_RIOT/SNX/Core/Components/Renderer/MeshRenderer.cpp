#include "MeshRenderer.h"

#include <SNX/Core/Components/Transform.h>
#include <SNX/Graphics/RenderContext.h>

#include <Windows.h>

#include <utility>

MeshRenderer::MeshRenderer(
	GameObject& gameObject,
	ID3D11Device* device,
	std::shared_ptr<DirectX::CommonStates> commonStates,
	std::shared_ptr<DirectX::IEffectFactory> effectFactory,
	std::wstring meshPath,
	MeshFileFormat fileFormat
) :
	Renderer(gameObject),
	m_device(device),
	m_commonStates(std::move(commonStates)),
	m_effectFactory(std::move(effectFactory)),
	m_meshPath(std::move(meshPath)),
	m_fileFormat(fileFormat) {}

void MeshRenderer::ConfigureEffects(
	const DirectX::SimpleMath::Vector3& fogColor,
	float fogStart,
	float fogEnd
) {
	m_fogColor = fogColor;
	m_fogStart = fogStart;
	m_fogEnd = fogEnd;

	m_configureEffects = true;

	if (m_mesh.IsLoaded())
		m_mesh.UpdateEffects(m_fogColor, m_fogStart, m_fogEnd);

	// ! lighting is configure by the ModelObject3D
}

void MeshRenderer::OnInitialize() {
	m_loadFailed = false;

	if (!m_device || !m_commonStates || !m_effectFactory || m_meshPath.empty()) {
		m_loadFailed = true;

#ifdef _DEBUG
		OutputDebugStringA("MeshRenderer received invalid initialization data");
#endif

		return;
	}

	bool loaded = false;

	switch (m_fileFormat) {
	case MeshFileFormat::CMO:
		loaded = m_mesh.LoadFromCMO(
			m_device,
			*m_effectFactory,
			m_meshPath.c_str()
		);

		break;

	case MeshFileFormat::SDKMesh:
	default:
		loaded = m_mesh.LoadFromSDKMESH(
			m_device,
			*m_effectFactory,
			m_meshPath.c_str()
		);
	}

	if (!loaded) {
		m_loadFailed = true;

#ifdef _DEBUG
		std::wstring message = L"Failed to load mesh: " + m_meshPath + L"\n";
		OutputDebugStringW(message.c_str());
#endif

		return;
	}

	if (m_configureEffects)
		m_mesh.UpdateEffects(m_fogColor, m_fogStart, m_fogEnd);
}

void MeshRenderer::Draw(const RenderContext& context) {
	if (!m_mesh.IsLoaded() || !m_commonStates || !context.deviceContext)
		return;

	m_mesh.DrawWithTransform(
		context.deviceContext,
		*m_commonStates,
		GetTransform().GetWorldMatrix(),
		context.view,
		context.projection
	);
}

void MeshRenderer::OnDestroy() {
	m_effectFactory.reset();
	m_commonStates.reset();

	m_device = nullptr;
}