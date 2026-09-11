#pragma once

#include <string>

struct AppConfig final {
	std::wstring title = L"AERO RIOT";
	int windowWidth = 1280;
	int windowHeight = 720;
	bool useVSync = true;
	bool isFullScreen = false;
};