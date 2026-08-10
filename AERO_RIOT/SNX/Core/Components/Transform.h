#pragma once

#include <SimpleMath.h>

#include <vector>

class GameObject;

enum class TransformSpace {
	Local,
	World
};

class Transform final {
public:
	Transform() = default;
	~Transform();

	// disallow to copy or move
	Transform(const Transform&) = delete;
	Transform& operator=(const Transform&) = delete;
	Transform(Transform&&) = delete;
	Transform& operator=(Transform&&) = delete;

	// --------------------------------------------------
	// Local transform
	// --------------------------------------------------

	[[nodiscard]]
	const DirectX::SimpleMath::Vector3& GetLocalPosition() const noexcept {
		return m_localPosition;
	}

	[[nodiscard]]
	const DirectX::SimpleMath::Quaternion& GetLocalRotation() const noexcept {
		return m_localRotation;
	}

	[[nodiscard]]
	const DirectX::SimpleMath::Vector3& GetLocalScale() const noexcept {
		return m_localScale;
	}

	void SetLocalPosition(const DirectX::SimpleMath::Vector3& position)  noexcept;

	void SetLocalRotation(const DirectX::SimpleMath::Quaternion& rotation)  noexcept;

	void SetLocalScale(const DirectX::SimpleMath::Vector3& scale)  noexcept;

	void SetLocalEulerDegrees(const DirectX::SimpleMath::Vector3& eulerDegrees)  noexcept;

	[[nodiscard]]
	DirectX::SimpleMath::Vector3 GetLocalEulerDegrees() const noexcept;

	// --------------------------------------------------
	// World transform
	// Unity-like names: position, rotation, lossyScale
	// --------------------------------------------------

	[[nodiscard]]
	DirectX::SimpleMath::Vector3 GetPosition() const noexcept;

	[[nodiscard]]
	DirectX::SimpleMath::Quaternion GetRotation() const noexcept;

	[[nodiscard]]
	DirectX::SimpleMath::Vector3 GetScale() const noexcept;

	[[nodiscard]]
	DirectX::SimpleMath::Vector3 GetEulerDegrees() const noexcept;

	bool SetPosition(const DirectX::SimpleMath::Vector3& position)  noexcept;

	bool SetRotation(const DirectX::SimpleMath::Quaternion& rotation)  noexcept;

	bool SetScale(const DirectX::SimpleMath::Vector3& scale)  noexcept;

	bool SetEulerDegrees(const DirectX::SimpleMath::Vector3& eulerDegrees)  noexcept;

	// --------------------------------------------------
	// Direction vectors
	// --------------------------------------------------

	[[nodiscard]]
	DirectX::SimpleMath::Vector3 GetForward() const noexcept;

	[[nodiscard]]
	DirectX::SimpleMath::Vector3 GetRight() const noexcept;

	[[nodiscard]]
	DirectX::SimpleMath::Vector3 GetUp() const noexcept;

	// --------------------------------------------------
	// Movement helpers
	// --------------------------------------------------

	bool Translate(
		const DirectX::SimpleMath::Vector3& translation,
		TransformSpace space = TransformSpace::World
	);

	bool RotateEulerDegrees(
		const DirectX::SimpleMath::Vector3& eulerDegrees,
		TransformSpace space = TransformSpace::Local
	);

	// --------------------------------------------------
	// Matrices
	// --------------------------------------------------

	[[nodiscard]]
	const DirectX::SimpleMath::Matrix& GetLocalMatrix() const noexcept;

	[[nodiscard]]
	const DirectX::SimpleMath::Matrix& GetWorldMatrix() const noexcept;

	// --------------------------------------------------
	// Hierarchy
	// --------------------------------------------------

	bool SetParent(Transform* parent, bool keepWorldTransform = true) noexcept;

	bool DetachFromParent(bool keepWorldTransform = true) noexcept {
		return SetParent(nullptr, keepWorldTransform);
	}

	[[nodiscard]]
	Transform* GetParent() noexcept { return m_parent; }

	[[nodiscard]]
	const Transform* GetParent() const noexcept { return m_parent; }

	[[nodiscard]]
	std::size_t GetChildCount() const noexcept { return m_children.size(); }

	[[nodiscard]]
	Transform* GetChild(std::size_t index) noexcept;

	[[nodiscard]]
	const Transform* GetChild(std::size_t index) const noexcept;

	[[nodiscard]]
	bool IsChildOf(const Transform* possibleParent) const noexcept;

	[[nodiscard]]
	GameObject* GetGameObject() noexcept { return m_gameObject; }

	[[nodiscard]]
	const GameObject* GetGameObject() const noexcept { return m_gameObject; }

private:
	bool TrySetLocalFromMatrix(const DirectX::SimpleMath::Matrix& matrix) noexcept;

	bool TrySetWorldFromMatrix(const DirectX::SimpleMath::Matrix& matrix) noexcept;

	void RemoveChild(Transform* child) noexcept;

	void MarkLocalDirty() noexcept;
	void MarkWorldDirty() noexcept;

	void SetGameObject(GameObject* gameObject) noexcept { m_gameObject = gameObject; }

	static DirectX::SimpleMath::Vector3 DegreesToRadians(const DirectX::SimpleMath::Vector3& degrees) noexcept;
	static DirectX::SimpleMath::Vector3 RadiansToDegrees(const DirectX::SimpleMath::Vector3& radians) noexcept;
	static DirectX::SimpleMath::Quaternion NormalizeRotation(DirectX::SimpleMath::Quaternion rotation) noexcept;

private:
	GameObject* m_gameObject = nullptr;

	DirectX::SimpleMath::Vector3 m_localPosition = DirectX::SimpleMath::Vector3::Zero;
	DirectX::SimpleMath::Quaternion m_localRotation = DirectX::SimpleMath::Quaternion::Identity;
	DirectX::SimpleMath::Vector3 m_localScale = DirectX::SimpleMath::Vector3::One;

	Transform* m_parent = nullptr;
	std::vector<Transform*> m_children;

	mutable DirectX::SimpleMath::Matrix m_localMatrix = DirectX::SimpleMath::Matrix::Identity;
	mutable DirectX::SimpleMath::Matrix m_worldMatrix = DirectX::SimpleMath::Matrix::Identity;

	mutable bool m_localMatrixDirty = true;
	mutable bool m_worldMatrixDirty = true;

	static constexpr float MinimumDeterminant = 0.000001f;

	// ! Redefining World Directions (same as DXTK's built-in Z direction)
	static constexpr DirectX::SimpleMath::Vector3 Forward{ 0.0f,0.0f,-1.0f };
	static constexpr DirectX::SimpleMath::Vector3 Right{ 1.0f,0.0f,0.0f };
	static constexpr DirectX::SimpleMath::Vector3 Up{ 0.0f,1.0f,0.0f };

	friend class GameObject;
};