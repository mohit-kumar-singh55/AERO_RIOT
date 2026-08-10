#include "GameObjectManager.h"

GameObjectManager::~GameObjectManager() {
	Clear();
}

GameObject& GameObjectManager::CreateGameObject(std::string name) {
	auto gameObject = std::make_unique<GameObject>(std::move(name));

	GameObject& reference = *gameObject;

	m_pendingObjects.push_back(std::move(gameObject));

	return reference;
}

void GameObjectManager::BeginFrame() {
	AddPendingObjects();
}

void GameObjectManager::FixedUpdate() {
	for (const auto& gameObject : m_objects) {
		if (!gameObject || gameObject->IsDestroyRequested())
			continue;

		gameObject->FixedUpdate();
	}
}

void GameObjectManager::Update() {
	for (const auto& gameObject : m_objects) {
		if (!gameObject || gameObject->IsDestroyRequested())
			continue;

		gameObject->Update();
	}
}

void GameObjectManager::LateUpdate() {
	for (const auto& gameObject : m_objects) {
		if (!gameObject || gameObject->IsDestroyRequested())
			continue;

		gameObject->LateUpdate();
	}
}

void GameObjectManager::Render(const RenderContext& context) {
	for (const auto& gameObject : m_objects) {
		if (!gameObject || gameObject->IsDestroyRequested())
			continue;

		gameObject->Render(context);
	}
}

void GameObjectManager::EndFrame() {
	// remove gameobjects' components if requested any
	for (const auto& gameObject : m_objects)
		if (gameObject && !gameObject->IsDestroyRequested())
			gameObject->EndFrame();

	for (const auto& gameObject : m_pendingObjects)
		if (gameObject && !gameObject->IsDestroyRequested())
			gameObject->EndFrame();

	// remove gameobjects requested for destroy
	RemoveDestroyedObjects();
}

void GameObjectManager::Destroy(GameObject& gameObject) noexcept {
	gameObject.RequestDestroy();
}

void GameObjectManager::DestroyAll() noexcept {
	for (const auto& gameObject : m_objects)
		if (gameObject)
			gameObject->RequestDestroy();

	for (const auto& gameObject : m_pendingObjects)
		if (gameObject)
			gameObject->RequestDestroy();
}

void GameObjectManager::Clear() noexcept {
	m_pendingObjects.clear();
	m_objects.clear();
}

GameObject* GameObjectManager::Find(std::string_view name) noexcept {
	for (const auto& gameObject : m_objects)
		if (gameObject && gameObject->GetName() == name)
			return gameObject.get();

	for (const auto& gameObject : m_pendingObjects)
		if (gameObject && gameObject->GetName() == name)
			return gameObject.get();

	return nullptr;
}

const GameObject* GameObjectManager::Find(std::string_view name) const noexcept {
	for (const auto& gameObject : m_objects)
		if (gameObject && gameObject->GetName() == name)
			return gameObject.get();

	for (const auto& gameObject : m_pendingObjects)
		if (gameObject && gameObject->GetName() == name)
			return gameObject.get();

	return nullptr;
}

void GameObjectManager::AddPendingObjects() {
	if (m_pendingObjects.empty()) return;

	/*
	* move the current pending list into a local variable
	* if a component's initialization creates move objects, those new
	* objects remain pending until the next frame
	*/
	auto pendingObjects = std::move(m_pendingObjects);

	m_pendingObjects.clear();

	for (auto& gameObject : pendingObjects) {
		if (!gameObject) continue;

		if (gameObject->IsDestroyRequested())
			continue;

		gameObject->Initialize();

		m_objects.push_back(std::move(gameObject));
	}
}

void GameObjectManager::RemoveDestroyedObjects() noexcept {
	std::erase_if(
		m_objects,
		[](const std::unique_ptr<GameObject>& gameObject) {
			return !gameObject || gameObject->IsDestroyRequested();
		}
	);

	std::erase_if(
		m_pendingObjects,
		[](const std::unique_ptr<GameObject>& gameObject) {
			return !gameObject || gameObject->IsDestroyRequested();
		}
	);
}