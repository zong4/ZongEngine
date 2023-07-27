#pragma once

#include <glm/glm.hpp>

namespace zong
{
namespace core
{

bool DecomposeTransform(glm::mat4 const& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);

}
} // namespace zong
