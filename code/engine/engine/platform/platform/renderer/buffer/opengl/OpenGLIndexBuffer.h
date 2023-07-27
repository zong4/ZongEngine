#pragma once

#include "../IndexBuffer.h"

namespace zong
{
namespace platform
{

class OpenGLIndexBuffer : public IndexBuffer
{
private:
    uint32_t m_RendererID;
    uint32_t m_Count;

public:
    virtual uint32_t count() const override { return m_Count; }

public:
    OpenGLIndexBuffer(uint32_t* indices, uint32_t count);
    virtual ~OpenGLIndexBuffer();

    virtual void bind() const override;
    virtual void unbind() const override;
};

} // namespace platform
} // namespace zong