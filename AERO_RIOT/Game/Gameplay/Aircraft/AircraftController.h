#pragma once

#include <SNX/Core/Object/Component.h>

class Aircraft;

class AircraftController final : public Component {
public:
	using Component::Component;

protected:
	void OnInitialize() override;

	void OnUpdate() override;

	void OnDestroy() override;

private:
	Aircraft* m_aircraft;
};