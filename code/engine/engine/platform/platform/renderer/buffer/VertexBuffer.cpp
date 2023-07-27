#include "VertexBuffer.h"

#include "../Renderer.h"
#include "../RendererAPI.h"
#include "opengl/OpenGLVertexBuffer.h"

std::shared_ptr<zong::platform::VertexBuffer> zong::platform::VertexBuffer::create(uint32_t size)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            ZONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!")
            return nullptr;
        case RendererAPI::API::OpenGL:
            return std::make_shared<OpenGLVertexBuffer>(size);
        default:
            ZONG_CORE_ASSERT(false, "unknown RendererAPI!")
            return nullptr;
    }
}

std::shared_ptr<zong::platform::VertexBuffer> zong::platform::VertexBuffer::create(float* vertices, uint32_t size)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            ZONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!")
            return nullptr;
        case RendererAPI::API::OpenGL:
            return std::make_shared<OpenGLVertexBuffer>(vertices, size);
        default:
            ZONG_CORE_ASSERT(false, "Unknown RendererAPI!")
            return nullptr;
    }
}