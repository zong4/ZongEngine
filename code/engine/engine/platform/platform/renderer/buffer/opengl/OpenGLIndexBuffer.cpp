#include "OpenGLIndexBuffer.h"

#include <glad/glad.h>

zong::platform::OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count) : m_Count(count)
{
    ZONG_PROFILE_FUNCTION();

    glCreateBuffers(1, &m_RendererID);

    // GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound VAO
    // Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of VAO state.
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
}

zong::platform::OpenGLIndexBuffer::~OpenGLIndexBuffer()
{
    ZONG_PROFILE_FUNCTION();

    glDeleteBuffers(1, &m_RendererID);
}

void zong::platform::OpenGLIndexBuffer::bind() const
{
    ZONG_PROFILE_FUNCTION();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
}

void zong::platform::OpenGLIndexBuffer::unbind() const
{
    ZONG_PROFILE_FUNCTION();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}