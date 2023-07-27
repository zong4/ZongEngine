#include "Texture.h"

#include "../Renderer.h"
#include "opengl/OpenGLTexture.h"

std::shared_ptr<zong::platform::Texture2D> zong::platform::Texture2D::Create(const TextureSpecification& specification)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            ZONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return std::make_shared<OpenGLTexture2D>(specification);
    }

    ZONG_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

std::shared_ptr<zong::platform::Texture2D> zong::platform::Texture2D::Create(const std::string& path)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            ZONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return std::make_shared<OpenGLTexture2D>(path);
    }

    ZONG_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}
