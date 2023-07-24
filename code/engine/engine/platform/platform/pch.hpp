#pragma once

#include <core/core.hpp>

#ifdef WINDOWS
    #define GLFW_INCLUDE_VULKAN
    #include <GLFW/glfw3.h>
    #include <vulkan/vulkan.h>

    #define GLM_FORCE_RADIANS
    #define GLM_FORCE_DEPTH_ZERO_TO_ONE
    #define GLM_ENABLE_EXPERIMENTAL
    #include <glm/glm.hpp>
    #include <glm/gtc/matrix_transform.hpp>
    #include <glm/gtx/hash.hpp>

#else LINUX
    #define GLFW_INCLUDE_VULKAN
    #include <GLFW/glfw3.h>
    #include <vulkan/vulkan.h>

    #define GLM_FORCE_RADIANS
    #define GLM_FORCE_DEPTH_ZERO_TO_ONE
    #define GLM_ENABLE_EXPERIMENTAL
    #include <glm/glm.hpp>
    #include <glm/gtc/matrix_transform.hpp>
    #include <glm/gtx/hash.hpp>

#endif
