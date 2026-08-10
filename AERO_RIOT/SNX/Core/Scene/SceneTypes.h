#pragma once

#include <cstdint>
#include <limits>

using SceneId = std::uint32_t;

inline constexpr SceneId InvalidSceneId = (std::numeric_limits<SceneId>::max)();