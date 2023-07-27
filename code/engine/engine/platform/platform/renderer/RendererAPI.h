#pragma once

#include <glm/glm.hpp>

#include "vao/VertexArray.h"

namespace zong
{
namespace platform
{

class RendererAPI
{
public:
    enum class API
    {
        None   = 0,
        OpenGL = 1
    };

private:
    static API _api;

public:
    static API api() { return _api; }

public:
    virtual ~RendererAPI() = default;

    virtual void init()                                                               = 0;
    virtual void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
    virtual void setClearColor(const glm::vec4& color)                                = 0;
    virtual void clear()                                                              = 0;

    virtual void drawIndexed(std::shared_ptr<VertexArray> const& vertexArray, uint32_t indexCount = 0) = 0;
    virtual void drawLines(std::shared_ptr<VertexArray> const& vertexArray, uint32_t vertexCount)      = 0;

    virtual void setLineWidth(float width) = 0;

    static std::unique_ptr<RendererAPI> create();
};

} // namespace platform
} // namespace zong
