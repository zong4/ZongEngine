#include "OpenGLRendererAPI.h"

#include <glad/glad.h>

#include "../vao/VertexArray.h"

void OpenGLMessageCallback(unsigned source, unsigned type, unsigned id, unsigned severity, int length, const char* message,
                           const void* userParam)
{
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
            ZONG_CORE_CRITICAL(message);
            return;
        case GL_DEBUG_SEVERITY_MEDIUM:
            // ZONG_CORE_ERROR(message);
            return;
        case GL_DEBUG_SEVERITY_LOW:
            ZONG_CORE_WARN(message);
            return;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            ZONG_CORE_TRACE(message);
            return;
    }

    ZONG_CORE_ASSERT(false, "Unknown severity level!");
}

void zong::platform::OpenGLRendererAPI::init()
{
    ZONG_PROFILE_FUNCTION();

#ifdef ZONG_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OpenGLMessageCallback, nullptr);

    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
}

void zong::platform::OpenGLRendererAPI::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    glViewport(x, y, width, height);
}

void zong::platform::OpenGLRendererAPI::setClearColor(const glm::vec4& color)
{
    glClearColor(color.r, color.g, color.b, color.a);
}

void zong::platform::OpenGLRendererAPI::clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void zong::platform::OpenGLRendererAPI::drawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
{
    vertexArray->bind();
    uint32_t count = indexCount ? indexCount : vertexArray->indexBuffer()->count();
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
}

void zong::platform::OpenGLRendererAPI::drawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount)
{
    vertexArray->bind();
    glDrawArrays(GL_LINES, 0, vertexCount);
}

void zong::platform::OpenGLRendererAPI::setLineWidth(float width)
{
    glLineWidth(width);
}
