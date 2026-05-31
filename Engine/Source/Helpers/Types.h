
#pragma once

// includes
#include <filesystem>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include "LinearMath/btVector3.h"

// FileSystem
using fSysPath = std::filesystem::path;

// GLM
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using quat = glm::quat;

// Bullet
using btVec3 = btVector3;

// Default
using uChar  = unsigned char;
using uInt   = unsigned int;
using wChar  = wchar_t;
using sizeT  = size_t;
using uint32 = uint32_t;

