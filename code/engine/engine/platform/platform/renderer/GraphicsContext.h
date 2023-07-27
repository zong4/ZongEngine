#pragma once

namespace zong
{
namespace platform
{

class GraphicsContext
{
public:
    virtual ~GraphicsContext() = default;

    virtual void init()        = 0;
    virtual void swapBuffers() = 0;

    static std::unique_ptr<GraphicsContext> create(void* window);
};

} // namespace platform
} // namespace zong