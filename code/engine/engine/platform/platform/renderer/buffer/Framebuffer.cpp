#include "Framebuffer.h"

#include "../Renderer.h"
#include "../RendererAPI.h"
#include "opengl/OpenGLFramebuffer.h"

std::shared_ptr<zong::platform::Framebuffer> zong::platform::Framebuffer::Create(FramebufferSpecification const& spec)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            ZONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return std::make_shared<OpenGLFramebuffer>(spec);
    }

    ZONG_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}
