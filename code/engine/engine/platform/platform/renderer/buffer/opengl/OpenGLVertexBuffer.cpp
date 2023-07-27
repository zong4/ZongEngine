#include "OpenGLVertexBuffer.h"

#include <glad/glad.h>

zong::platform::OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size)
{
    ZONG_PROFILE_FUNCTION();

    glCreateBuffers(1, &_rendererID);
    glBindBuffer(GL_ARRAY_BUFFER, _rendererID);
    glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
}

zong::platform::OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size)
{
    ZONG_PROFILE_FUNCTION();

    glCreateBuffers(1, &_rendererID);
    glBindBuffer(GL_ARRAY_BUFFER, _rendererID);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

zong::platform::OpenGLVertexBuffer::~OpenGLVertexBuffer()
{
    ZONG_PROFILE_FUNCTION();

    glDeleteBuffers(1, &_rendererID);
}

void zong::platform::OpenGLVertexBuffer::setData(const void* data, uint32_t size)
{
    glBindBuffer(GL_ARRAY_BUFFER, _rendererID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

void zong::platform::OpenGLVertexBuffer::bind() const
{
    ZONG_PROFILE_FUNCTION();

    glBindBuffer(GL_ARRAY_BUFFER, _rendererID);
}

void zong::platform::OpenGLVertexBuffer::unbind() const
{
    ZONG_PROFILE_FUNCTION();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
