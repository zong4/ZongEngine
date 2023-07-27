#pragma once

#include "RendererAPI.h"

namespace zong
{
namespace platform
{

class RenderCommand
{
public:
    static void Init() { s_RendererAPI->init(); }

    static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) { s_RendererAPI->setViewport(x, y, width, height); }

    static void SetClearColor(const glm::vec4& color) { s_RendererAPI->setClearColor(color); }

    static void Clear() { s_RendererAPI->clear(); }

    static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0)
    {
        s_RendererAPI->drawIndexed(vertexArray, indexCount);
    }

    static void DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount)
    {
        s_RendererAPI->drawLines(vertexArray, vertexCount);
    }

    static void SetLineWidth(float width) { s_RendererAPI->setLineWidth(width); }

private:
    static std::unique_ptr<RendererAPI> s_RendererAPI;
};

} // namespace platform
} // namespace zong
