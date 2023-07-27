#include "Renderer.h"

#include "RenderCommand.h"
#include "Renderer2D.h"

std::unique_ptr<zong::platform::Renderer::SceneData> zong::platform::Renderer::s_SceneData =
    std::make_unique<zong::platform::Renderer::SceneData>();

void zong::platform::Renderer::init()
{
    ZONG_PROFILE_FUNCTION();

    RenderCommand::Init();
    Renderer2D::Init();
}

void zong::platform::Renderer::shutdown()
{
    Renderer2D::Shutdown();
}

void zong::platform::Renderer::onWindowResize(uint32_t width, uint32_t height)
{
    RenderCommand::SetViewport(0, 0, width, height);
}

void zong::platform::Renderer::beginScene(OrthographicCamera& camera)
{
    s_SceneData->ViewProjectionMatrix = camera.viewProjectionMatrix();
}

void zong::platform::Renderer::endScene()
{
}

void zong::platform::Renderer::submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray,
                                      const glm::mat4& transform)
{
    shader->Bind();
    shader->SetMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
    shader->SetMat4("u_Transform", transform);

    vertexArray->bind();
    RenderCommand::DrawIndexed(vertexArray);
}
