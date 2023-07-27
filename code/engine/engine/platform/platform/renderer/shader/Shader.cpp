#include "Shader.h"

#include "../Renderer.h"
#include "../RendererAPI.h"
#include "opengl/OpenGLShader.h"

std::shared_ptr<zong::platform::Shader> zong::platform::Shader::Create(const std::string& filepath)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            ZONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return std::make_shared<OpenGLShader>(filepath);
    }

    ZONG_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

std::shared_ptr<zong::platform::Shader> zong::platform::Shader::Create(const std::string& name, const std::string& vertexSrc,
                                                                       const std::string& fragmentSrc)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            ZONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return std::make_shared<OpenGLShader>(name, vertexSrc, fragmentSrc);
    }

    ZONG_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

void zong::platform::ShaderLibrary::Add(const std::string& name, const std::shared_ptr<Shader>& shader)
{
    ZONG_CORE_ASSERT(!Exists(name), "Shader already exists!");
    m_Shaders[name] = shader;
}

void zong::platform::ShaderLibrary::Add(const std::shared_ptr<Shader>& shader)
{
    auto& name = shader->GetName();
    Add(name, shader);
}

std::shared_ptr<zong::platform::Shader> zong::platform::ShaderLibrary::Load(const std::string& filepath)
{
    auto shader = Shader::Create(filepath);
    Add(shader);
    return shader;
}

std::shared_ptr<zong::platform::Shader> zong::platform::ShaderLibrary::Load(const std::string& name, const std::string& filepath)
{
    auto shader = Shader::Create(filepath);
    Add(name, shader);
    return shader;
}

std::shared_ptr<zong::platform::Shader> zong::platform::ShaderLibrary::Get(const std::string& name)
{
    ZONG_CORE_ASSERT(Exists(name), "Shader not found!");
    return m_Shaders[name];
}

bool zong::platform::ShaderLibrary::Exists(const std::string& name) const
{
    return m_Shaders.find(name) != m_Shaders.end();
}
