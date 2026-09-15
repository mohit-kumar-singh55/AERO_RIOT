#pragma once

#include "AircraftControlInput.h"

#include <SNX/Core/Object/Component.h>

class KineticBody;

// physics component for aircraft
class AircraftKinetics : public Component {
public:
	using Component::Component;

	// apply the physics
	void Apply(const AircraftControlInput&) noexcept;

protected:
	void OnInitialize() override;

private:
	KineticBody* m_kb = nullptr;

	// configurable
	float m_maxThrust = 20.0f;
	// directional aerodynamic drag coefficient
	float m_forwardDrag = 1.0f;
	float m_sideDrag = 3.0f;
	float m_verticalDrag = 2.0f;
	float m_airBrakePower = 4.0f;	// additional drag
};