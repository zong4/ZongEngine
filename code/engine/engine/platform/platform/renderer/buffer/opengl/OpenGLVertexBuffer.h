#pragma once

#include "../VertexBuffer.h"

namespace zong
{
namespace platform
{

class OpenGLVertexBuffer : public VertexBuffer
{
private:
    uint32_t     _rendererID;
    BufferLayout _layout;

public:
    virtual const BufferLayout& layout() const override { return _layout; }

    virtual void setLayout(const BufferLayout& layout) override { _layout = layout; }
    virtual void setData(const void* data, uint32_t size) override;

public:
    OpenGLVertexBuffer(uint32_t size);
    OpenGLVertexBuffer(float* vertices, uint32_t size);
    virtual ~OpenGLVertexBuffer();

    virtual void bind() const override;
    virtual void unbind() const override;
};

} // namespace platform
} // namespace zong