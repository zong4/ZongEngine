#pragma once

#include "../VertexArray.h"

namespace zong
{
namespace platform
{

class OpenGLVertexArray : public VertexArray
{
private:
    uint32_t                                   _rendererID;
    uint32_t                                   _vertexBufferIndex = 0;
    std::vector<std::shared_ptr<VertexBuffer>> _vertexBuffers;
    std::shared_ptr<IndexBuffer>               _indexBuffer;

public:
    virtual const std::vector<std::shared_ptr<VertexBuffer>>& vertexBuffers() const override { return _vertexBuffers; }
    virtual const std::shared_ptr<IndexBuffer>&               indexBuffer() const override { return _indexBuffer; }

public:
    OpenGLVertexArray();
    virtual ~OpenGLVertexArray();

    virtual void bind() const override;
    virtual void unbind() const override;

    virtual void addVertexBuffer(std::shared_ptr<VertexBuffer> const& vertexBuffer) override;
    virtual void setIndexBuffer(std::shared_ptr<IndexBuffer> const& indexBuffer) override;
};

} // namespace platform
} // namespace zong
