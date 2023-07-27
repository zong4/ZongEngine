#pragma once

#include "BufferLayout.h"

namespace zong
{
namespace platform
{

class VertexBuffer
{
public:
    virtual const BufferLayout& layout() const = 0;

    virtual void setLayout(BufferLayout const& layout)    = 0;
    virtual void setData(const void* data, uint32_t size) = 0;

public:
    virtual ~VertexBuffer() = default;

    virtual void bind() const   = 0;
    virtual void unbind() const = 0;

    static std::shared_ptr<VertexBuffer> create(uint32_t size);
    static std::shared_ptr<VertexBuffer> create(float* vertices, uint32_t size);
};

} // namespace platform
} // namespace zong