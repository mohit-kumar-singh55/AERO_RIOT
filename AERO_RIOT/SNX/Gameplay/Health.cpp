#include "Health.h"

#include <algorithm>

Health::Health(
	int maxHealth,
	float invincibilityDuration
) noexcept :
	m_maxHealth(maxHealth),
	m_invincibilityDuration(invincibilityDuration) {
	m_currentHealth = m_maxHealth;
}

void Health::Update(float deltaTime) noexcept {
	if (m_invincibilityTimer <= 0.0f) return;
	m_invincibilityTimer -= deltaTime;
}

bool Health::TakeDamage(int damage) noexcept {
	if (damage <= 0 || IsDead() || IsInvincible())
		return false;

	// take damage & clamp to 0
	m_currentHealth = std::max(m_currentHealth - damage, 0);

	// start invincibility
	m_invincibilityTimer = m_invincibilityDuration;

	return true;
}

void Health::Heal(int amount) noexcept {
	if (amount <= 0 || IsDead())
		return;

	// heal & clamp to max health
	m_currentHealth = std::min(m_currentHealth + amount, m_maxHealth);
}

void Health::Reset() noexcept {
	m_currentHealth = m_maxHealth;
	m_invincibilityTimer = 0.0f;
}

float Health::GetHealthRatio() const noexcept {
	if (m_maxHealth <= 0)
		return 0.0f;

	const float ratio =
		static_cast<float>(m_currentHealth) /
		static_cast<float>(m_maxHealth);

	return std::clamp(ratio, 0.0f, 1.0f);
}
