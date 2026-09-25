#pragma once

#include <string>

namespace Utils::Conversion {
	// std::string to std::wstring
	std::wstring ToWString(const std::string& str) {
		return { str.begin(), str.end() };
	}
}