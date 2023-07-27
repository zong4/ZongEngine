#include "OpenGLVertexArray.h"

#include <glad/glad.h>

static GLenum ShaderDataTypeToOpenGLBaseType(zong::platform::ShaderDataType type)
{
    switch (type)
    {
        case zong::platform::ShaderDataType::Float:
            return GL_FLOAT;
        case zong::platform::ShaderDataType::Float2:
            return GL_FLOAT;
        case zong::platform::ShaderDataType::Float3:
            return GL_FLOAT;
        case zong::platform::ShaderDataType::Float4:
            return GL_FLOAT;
        case zong::platform::ShaderDataType::Mat3:
            return GL_FLOAT;
        case zong::platform::ShaderDataType::Mat4:
            return GL_FLOAT;
        case zong::platform::ShaderDataType::Int:
            return GL_INT;
        case zong::platform::ShaderDataType::Int2:
            return GL_INT;
        case zong::platform::ShaderDataType::Int3:
            return GL_INT;
        case zong::platform::ShaderDataType::Int4:
            return GL_INT;
        case zong::platform::ShaderDataType::Bool:
            return GL_BOOL;
        default:
            ZONG_CORE_ASSERT(false, "Unknown ShaderDataType!")
            return 0;
    }
}

zong::platform::OpenGLVertexArray::OpenGLVertexArray() : VertexArray()
{
    ZONG_PROFILE_FUNCTION();

    glCreateVertexArrays(1, &_rendererID);
}

zong::platform::OpenGLVertexArray::~OpenGLVertexArray()
{
    ZONG_PROFILE_FUNCTION();

    glDeleteVertexArrays(1, &_rendererID);
}

void zong::platform::OpenGLVertexArray::bind() const
{
    ZONG_PROFILE_FUNCTION();

    glBindVertexArray(_rendererID);
}

void zong::platform::OpenGLVertexArray::unbind() const
{
    ZONG_PROFILE_FUNCTION();

    glBindVertexArray(0);
}

void zong::platform::OpenGLVertexArray::addVertexBuffer(std::shared_ptr<VertexBuffer> const& vertexBuffer)
{
    ZONG_PROFILE_FUNCTION();

    ZONG_CORE_ASSERT(vertexBuffer->layout().elements().size(), "Vertex Buffer has no layout!")

    glBindVertexArray(_rendererID);
    vertexBuffer->bind();

    auto const& layout = vertexBuffer->layout();
    for (auto const& element : layout)
    {
        switch (element._type)
        {
            case ShaderDataType::Float:
            case ShaderDataType::Float2:
            case ShaderDataType::Float3:
            case ShaderDataType::Float4:
            {
                glEnableVertexAttribArray(_vertexBufferIndex);
                glVertexAttribPointer(_vertexBufferIndex, element.componentCount(), ShaderDataTypeToOpenGLBaseType(element._type),
                                      element._normalized ? GL_TRUE : GL_FALSE, layout.stride(), (const void*)element._offset);
                _vertexBufferIndex++;
                break;
            }
            case ShaderDataType::Int:
            case ShaderDataType::Int2:
            case ShaderDataType::Int3:
            case ShaderDataType::Int4:
            case ShaderDataType::Bool:
            {
                glEnableVertexAttribArray(_vertexBufferIndex);
                glVertexAttribIPointer(_vertexBufferIndex, element.componentCount(), ShaderDataTypeToOpenGLBaseType(element._type),
                                       layout.stride(), (const void*)element._offset);
                _vertexBufferIndex++;
                break;
            }
            case ShaderDataType::Mat3:
            case ShaderDataType::Mat4:
            {
                uint8_t count = element.componentCount();
                for (uint8_t i = 0; i < count; i++)
                {
                    glEnableVertexAttribArray(_vertexBufferIndex);
                    glVertexAttribPointer(_vertexBufferIndex, count, ShaderDataTypeToOpenGLBaseType(element._type),
                                          element._normalized ? GL_TRUE : GL_FALSE, layout.stride(),
                                          (const void*)(element._offset + sizeof(float) * count * i));
                    glVertexAttribDivisor(_vertexBufferIndex, 1);
                    _vertexBufferIndex++;
                }
                break;
            }
            default:
                ZONG_CORE_ASSERT(false, "Unknown ShaderDataType!");
        }
    }

    _vertexBuffers.push_back(vertexBuffer);
}

void zong::platform::OpenGLVertexArray::setIndexBuffer(std::shared_ptr<IndexBuffer> const& indexBuffer)
{
    ZONG_PROFILE_FUNCTION();

    glBindVertexArray(_rendererID);
    indexBuffer->bind();

    _indexBuffer = indexBuffer;
}
