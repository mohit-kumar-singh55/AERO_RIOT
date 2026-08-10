#include "Transform.h"

#include <cmath>

Transform::~Transform() {
	// detach from our parent
	if (m_parent) {
		m_parent->RemoveChild(this);
		m_parent = nullptr;
	}

	/*
	* Prevent children from retaining a dangling pointer.
	*
	* Their local transform is preserved. because the parent
	* is disappearing, their world transform may change.
	*/
	/*
	* Normal GameObject.RequestDestroy() recursively destroys children
	*
	* This is still required as defensive cleanup because
	* immediate manager clearing may destroy objects in any
	* storage order.
	*/
	for (Transform* child : m_children) {
		if (!child) continue;

		if (child->m_parent == this) {
			child->m_parent = nullptr;
			child->MarkWorldDirty();
		}
	}

	/*
	* Transform does not own its children,
	* actual instances are still in GameObjectManager
	*
	* child list contains only non-owning pointers
	*/
	m_children.clear();
}

void Transform::SetLocalPosition(const DirectX::SimpleMath::Vector3& position) noexcept {
	m_localPosition = position;
	MarkLocalDirty();
}

void Transform::SetLocalRotation(const DirectX::SimpleMath::Quaternion& rotation) noexcept {
	m_localRotation = NormalizeRotation(rotation);
	MarkLocalDirty();
}

void Transform::SetLocalScale(const DirectX::SimpleMath::Vector3& scale) noexcept {
	m_localScale = scale;
	MarkLocalDirty();
}

void Transform::SetLocalEulerDegrees(const DirectX::SimpleMath::Vector3& eulerDegrees) noexcept {
	const auto radians = DegreesToRadians(eulerDegrees);
	SetLocalRotation(DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(radians));
}

DirectX::SimpleMath::Vector3 Transform::GetLocalEulerDegrees() const noexcept {
	const auto radians = m_localRotation.ToEuler();
	return RadiansToDegrees(radians);
}

DirectX::SimpleMath::Vector3 Transform::GetPosition() const noexcept {
	return GetWorldMatrix().Translation();
}

DirectX::SimpleMath::Quaternion Transform::GetRotation() const noexcept {
	using namespace DirectX::SimpleMath;

	Matrix world = GetWorldMatrix();

	Vector3 scale, translation;
	Quaternion rotation;

	if (!world.Decompose(scale, rotation, translation))
		return Quaternion::Identity;

	return NormalizeRotation(rotation);
}

DirectX::SimpleMath::Vector3 Transform::GetScale() const noexcept {
	using namespace DirectX::SimpleMath;

	Matrix world = GetWorldMatrix();

	Vector3 scale, translation;
	Quaternion rotation;

	if (!world.Decompose(scale, rotation, translation))
		return Vector3::One;

	return scale;
}

DirectX::SimpleMath::Vector3 Transform::GetEulerDegrees() const noexcept {
	return RadiansToDegrees(GetRotation().ToEuler());
}

bool Transform::SetPosition(const DirectX::SimpleMath::Vector3& position) noexcept {
	using namespace DirectX::SimpleMath;

	if (!m_parent) {
		SetLocalPosition(position);
		return true;
	}

	const Matrix& parentWorld = m_parent->GetWorldMatrix();

	if (std::abs(parentWorld.Determinant()) < MinimumDeterminant)
		return false;

	const Vector3 localPosition = Vector3::Transform(position, parentWorld.Invert());
	SetLocalPosition(localPosition);

	return true;
}

bool Transform::SetRotation(const DirectX::SimpleMath::Quaternion& rotation) noexcept {
	using namespace DirectX::SimpleMath;

	Matrix world = GetWorldMatrix();

	Vector3 currentScale, currentPosition;
	Quaternion currentRotation;

	if (!world.Decompose(currentScale, currentRotation, currentPosition))
		return false;

	const Matrix desiredWorld =
		Matrix::CreateScale(currentScale) *
		Matrix::CreateFromQuaternion(rotation) *
		Matrix::CreateTranslation(currentPosition);

	return TrySetWorldFromMatrix(desiredWorld);
}

bool Transform::SetScale(const DirectX::SimpleMath::Vector3& scale) noexcept {
	using namespace DirectX::SimpleMath;

	Matrix world = GetWorldMatrix();

	Vector3 currentScale, currentPosition;
	Quaternion currentRotation;

	if (!world.Decompose(currentScale, currentRotation, currentPosition))
		return false;

	const Matrix desiredWorld =
		Matrix::CreateScale(scale) *
		Matrix::CreateFromQuaternion(currentRotation) *
		Matrix::CreateTranslation(currentPosition);

	return TrySetWorldFromMatrix(desiredWorld);
}

bool Transform::SetEulerDegrees(const DirectX::SimpleMath::Vector3& eulerDegrees) noexcept {
	const auto radians = DegreesToRadians(eulerDegrees);
	return SetRotation(DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(radians));
}

// -Z
DirectX::SimpleMath::Vector3 Transform::GetForward() const noexcept {
	using namespace DirectX::SimpleMath;

	Vector3 forward = Vector3::Transform(Vector3::Forward, GetRotation());

	if (forward.LengthSquared() > 0.000001f)
		forward.Normalize();

	return forward;
}

// +X
DirectX::SimpleMath::Vector3 Transform::GetRight() const noexcept {
	using namespace DirectX::SimpleMath;

	Vector3 right = Vector3::Transform(Vector3::Right, GetRotation());

	if (right.LengthSquared() > 0.000001f)
		right.Normalize();

	return right;
}

// +Y
DirectX::SimpleMath::Vector3 Transform::GetUp() const noexcept {
	using namespace DirectX::SimpleMath;

	Vector3 up = Vector3::Transform(Vector3::Up, GetRotation());

	if (up.LengthSquared() > 0.000001f)
		up.Normalize();

	return up;
}

bool Transform::Translate(
	const DirectX::SimpleMath::Vector3& translation,
	TransformSpace space
) {
	using DirectX::SimpleMath::Vector3;

	if (space == TransformSpace::World)
		return SetPosition(GetPosition() + translation);

	const Vector3 worldTranslation =
		GetRight() * translation.x +
		GetUp() * translation.y +
		GetForward() * translation.z;

	return SetPosition(GetPosition() + worldTranslation);
}

bool Transform::RotateEulerDegrees(
	const DirectX::SimpleMath::Vector3& eulerDegrees,
	TransformSpace space
) {
	using namespace DirectX::SimpleMath;

	const Quaternion deltaRotation = Quaternion::CreateFromYawPitchRoll(DegreesToRadians(eulerDegrees));

	if (space == TransformSpace::Local) {
		// apply the new rotation around this object's current local axes
		const Quaternion result = Quaternion::Concatenate(deltaRotation, m_localRotation);
		SetLocalRotation(result);
		return true;
	}

	// apply the new rotation around world axes
	const Quaternion result = Quaternion::Concatenate(GetRotation(), deltaRotation);
	return SetRotation(result);
}

const DirectX::SimpleMath::Matrix& Transform::GetLocalMatrix() const noexcept {
	using DirectX::SimpleMath::Matrix;

	if (m_localMatrixDirty) {
		m_localMatrix =
			Matrix::CreateScale(m_localScale) *
			Matrix::CreateFromQuaternion(m_localRotation) *
			Matrix::CreateTranslation(m_localPosition);

		m_localMatrixDirty = false;
	}

	return m_localMatrix;
}

const DirectX::SimpleMath::Matrix& Transform::GetWorldMatrix() const noexcept {
	using DirectX::SimpleMath::Matrix;

	if (m_worldMatrixDirty) {
		if (m_parent)
			m_worldMatrix = GetLocalMatrix() * m_parent->GetWorldMatrix();
		else
			m_worldMatrix = GetLocalMatrix();

		m_worldMatrixDirty = false;
	}

	return m_worldMatrix;
}

bool Transform::SetParent(Transform* parent, bool keepWorldTransform) noexcept {
	using DirectX::SimpleMath::Matrix;

	if (parent == m_parent)
		return true;

	// an object cannot parent itself
	if (parent == this)
		return false;

	/*
	* Prevent:
	* A
	* └── B
	*    └── A
	*/
	if (parent && parent->IsChildOf(this))
		return false;

	Transform* previousParent = m_parent;
	const Matrix previousWorld = GetWorldMatrix();

	const auto previousLocalPosition = m_localPosition;
	const auto previousLocalRotation = m_localRotation;
	const auto previousLocalScale = m_localScale;

	// replace parent
	if (m_parent)
		m_parent->RemoveChild(this);

	m_parent = parent;

	if (m_parent)
		m_parent->m_children.push_back(this);

	MarkWorldDirty();

	if (!keepWorldTransform)
		return true;

	if (TrySetWorldFromMatrix(previousWorld))
		return true;

	// reparenting failed, usually because a parent
	// matrix could not be inverted or decomposed
	// ! so reparent it back to its original parent
	if (m_parent)
		m_parent->RemoveChild(this);

	m_parent = previousParent;

	if (m_parent)
		m_parent->m_children.push_back(this);

	m_localPosition = previousLocalPosition;
	m_localRotation = previousLocalRotation;
	m_localScale = previousLocalScale;

	MarkLocalDirty();

	return false;
}

Transform* Transform::GetChild(std::size_t index) noexcept {
	if (index >= m_children.size())
		return nullptr;

	return m_children[index];
}

const Transform* Transform::GetChild(std::size_t index) const noexcept {
	if (index >= m_children.size())
		return nullptr;

	return m_children[index];
}

bool Transform::IsChildOf(const Transform* possibleParent) const noexcept {
	if (!possibleParent)
		return false;

	const Transform* current = m_parent;

	while (current) {
		if (current == possibleParent)
			return true;

		current = current->m_parent;
	}

	return false;
}

bool Transform::TrySetLocalFromMatrix(const DirectX::SimpleMath::Matrix& matrix) noexcept {
	using namespace DirectX::SimpleMath;

	Matrix matrixCopy = matrix;

	Vector3 scale, translation;
	Quaternion rotation;

	if (!matrixCopy.Decompose(scale, rotation, translation))
		return false;

	m_localPosition = translation;
	m_localRotation = NormalizeRotation(rotation);
	m_localScale = scale;

	MarkLocalDirty();

	return true;
}

bool Transform::TrySetWorldFromMatrix(const DirectX::SimpleMath::Matrix& matrix) noexcept {
	using namespace DirectX::SimpleMath;

	if (!m_parent)
		return TrySetLocalFromMatrix(matrix);

	const Matrix& parentWorld = m_parent->GetWorldMatrix();

	// check if matrix can be inverted
	if (std::abs(parentWorld.Determinant()) < MinimumDeterminant)
		return false;

	/*
	* because world = local * parentWorld
	* so local = world * invertedParentWorld
	*/
	const Matrix localMatrix = matrix * parentWorld.Invert();

	return TrySetLocalFromMatrix(localMatrix);
}

void Transform::RemoveChild(Transform* child) noexcept {
	const auto iterator = std::remove(m_children.begin(), m_children.end(), child);
	m_children.erase(iterator, m_children.end());
}

void Transform::MarkLocalDirty() noexcept {
	m_localMatrixDirty = true;
	MarkWorldDirty();
}

void Transform::MarkWorldDirty() noexcept {
	if (m_worldMatrixDirty)
		return;

	m_worldMatrixDirty = true;

	for (Transform* child : m_children)
		if (child)
			child->MarkWorldDirty();
}

DirectX::SimpleMath::Vector3 Transform::DegreesToRadians(const DirectX::SimpleMath::Vector3& degrees) noexcept {
	return {
		DirectX::XMConvertToRadians(degrees.x),
		DirectX::XMConvertToRadians(degrees.y),
		DirectX::XMConvertToRadians(degrees.z)
	};
}

DirectX::SimpleMath::Vector3 Transform::RadiansToDegrees(const DirectX::SimpleMath::Vector3& radians) noexcept {
	return {
		DirectX::XMConvertToDegrees(radians.x),
		DirectX::XMConvertToDegrees(radians.y),
		DirectX::XMConvertToDegrees(radians.z)
	};
}

DirectX::SimpleMath::Quaternion Transform::NormalizeRotation(DirectX::SimpleMath::Quaternion rotation) noexcept {
	if (rotation.LengthSquared() <= 0.000001f)
		return DirectX::SimpleMath::Quaternion::Identity;

	rotation.Normalize();

	return rotation;
}
