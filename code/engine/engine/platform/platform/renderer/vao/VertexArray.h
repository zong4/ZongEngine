#pragma once

#include "../buffer/IndexBuffer.h"
#include "../buffer/VertexBuffer.h"

namespace zong
{
namespace platform
{

class VertexArray
{
public:
    virtual const std::vector<std::shared_ptr<VertexBuffer>>& vertexBuffers() const = 0;
    virtual const std::shared_ptr<IndexBuffer>&               indexBuffer() const   = 0;

public:
    virtual ~VertexArray() = default;

    virtual void bind() const   = 0;
    virtual void unbind() const = 0;

    virtual void addVertexBuffer(std::shared_ptr<VertexBuffer> const& vertexBuffer) = 0;
    virtual void setIndexBuffer(std::shared_ptr<IndexBuffer> const& indexBuffer)    = 0;

    static std::shared_ptr<VertexArray> Create();
};

} // namespace platform
} // namespace zong
