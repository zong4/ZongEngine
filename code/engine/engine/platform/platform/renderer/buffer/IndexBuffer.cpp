#include "IndexBuffer.h"

#include "../Renderer.h"
#include "../RendererAPI.h"
#include "opengl/OpenGLIndexBuffer.h"

std::shared_ptr<zong::platform::IndexBuffer> zong::platform::IndexBuffer::create(uint32_t* indices, uint32_t size)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            ZONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return std::make_shared<OpenGLIndexBuffer>(indices, size);
        default:
            ZONG_CORE_ASSERT(false, "unknown RendererAPI!");
            return nullptr;
    }
}