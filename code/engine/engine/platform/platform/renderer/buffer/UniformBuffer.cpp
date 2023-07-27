#include "UniformBuffer.h"

#include "../Renderer.h"
#include "../RendererAPI.h"
#include "opengl/OpenGLUniformBuffer.h"

std::shared_ptr<zong::platform::UniformBuffer> zong::platform::UniformBuffer::Create(uint32_t size, uint32_t binding)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            ZONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return std::make_shared<OpenGLUniformBuffer>(size, binding);
    }

    ZONG_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}
