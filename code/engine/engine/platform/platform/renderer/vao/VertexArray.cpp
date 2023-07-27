#include "VertexArray.h"

#include "../Renderer.h"
#include "../RendererAPI.h"
#include "opengl/OpenGLVertexArray.h"

std::shared_ptr<zong::platform::VertexArray> zong::platform::VertexArray::Create()
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            ZONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!")
            return nullptr;
        case RendererAPI::API::OpenGL:
            return std::make_shared<OpenGLVertexArray>();
        default:
            ZONG_CORE_ASSERT(false, "unknown RendererAPI!")
            return nullptr;
    }
}
