#pragma once

#include "../GraphicsContext.h"

struct GLFWwindow;

namespace zong
{
namespace platform
{

class OpenGLContext : public GraphicsContext
{
public:
    OpenGLContext(GLFWwindow* windowHandle);

    virtual void init() override;
    virtual void swapBuffers() override;

private:
    GLFWwindow* m_WindowHandle;
};

} // namespace platform
} // namespace zong