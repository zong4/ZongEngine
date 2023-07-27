#include "RendererAPI.h"

#include "opengl/OpenGLRendererAPI.h"

zong::platform::RendererAPI::API zong::platform::RendererAPI::_api = RendererAPI::API::OpenGL;

std::unique_ptr<zong::platform::RendererAPI> zong::platform::RendererAPI::create()
{
    switch (_api)
    {
        case RendererAPI::API::None:
            ZONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return std::make_unique<OpenGLRendererAPI>();
    }

    ZONG_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}