#pragma once

#include "../RendererAPI.h"

namespace zong
{
namespace platform
{

class OpenGLRendererAPI : public RendererAPI
{
public:
    virtual void init() override;
    virtual void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
    virtual void setClearColor(const glm::vec4& color) override;
    virtual void clear() override;

    virtual void drawIndexed(std::shared_ptr<VertexArray> const& vertexArray, uint32_t indexCount = 0) override;
    virtual void drawLines(std::shared_ptr<VertexArray> const& vertexArray, uint32_t vertexCount) override;

    virtual void setLineWidth(float width) override;
};

} // namespace platform
} // namespace zong
