#include "GraphicsContext.h"

#include "../window/linux/GLFWWindow.hpp"
#include "Renderer.h"
#include "opengl/OpenGLContext.h"

std::unique_ptr<zong::platform::GraphicsContext> zong::platform::GraphicsContext::create(void* window)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            ZONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return std::make_unique<OpenGLContext>(static_cast<GLFWwindow*>(window));
    }

    ZONG_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}
