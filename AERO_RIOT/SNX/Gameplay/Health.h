#pragma once

class Health final {
public:
	explicit Health(
		int maxHealth = 100,
		float invincibilityDuration = 0.0f
	) noexcept;

	void Update(float deltaTime) noexcept;

	bool TakeDamage(int damage) noexcept;
	void Heal(int amount) noexcept;
	void Reset() noexcept;

	[[nodiscard]]
	float GetHealthRatio() const noexcept;

	[[nodiscard]]
	inline int GetCurrentHealth() const noexcept { return m_currentHealth; }

	[[nodiscard]]
	inline float GetMaxHealth() const noexcept { return m_maxHealth; }

	[[nodiscard]]
	inline bool IsDead() const noexcept { return m_currentHealth <= 0; }

	[[nodiscard]]
	inline bool IsInvincible() const noexcept { return m_invincibilityTimer > 0.0f; }

	[[nodiscard]]
	inline float GetInvincibleTimer() const noexcept { return m_invincibilityTimer; }

private:
	int m_maxHealth = 100;
	int m_currentHealth = m_maxHealth;

	float m_invincibilityDuration = 0.0f;
	float m_invincibilityTimer = 0.0f;
};