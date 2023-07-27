#pragma once

#include "RendererAPI.h"
#include "camera/OrthographicCamera.h"
#include "platform/pch.hpp"
#include "shader/Shader.h"

namespace zong
{
namespace platform
{

class Renderer
{
public:
    static void init();
    static void shutdown();

    static void onWindowResize(uint32_t width, uint32_t height);

    static void beginScene(OrthographicCamera& camera);
    static void endScene();

    static void submit(std::shared_ptr<Shader> const& shader, const std::shared_ptr<VertexArray>& vertexArray,
                       const glm::mat4& transform = glm::mat4(1.0f));

    static RendererAPI::API GetAPI() { return RendererAPI::api(); }

private:
    struct SceneData
    {
        glm::mat4 ViewProjectionMatrix;
    };

    static std::unique_ptr<SceneData> s_SceneData;
};

} // namespace platform
} // namespace zong
