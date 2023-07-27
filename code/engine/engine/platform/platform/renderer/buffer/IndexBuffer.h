#pragma once

namespace zong
{
namespace platform
{

// TODO: currently Engine only supports 32-bit index buffers
class IndexBuffer
{
public:
    virtual uint32_t count() const = 0;

public:
    virtual ~IndexBuffer() = default;

    virtual void bind() const   = 0;
    virtual void unbind() const = 0;

    static std::shared_ptr<IndexBuffer> create(uint32_t* indices, uint32_t count);
};

} // namespace platform
} // namespace zong
