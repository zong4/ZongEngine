#pragma once

#include "BufferElement.h"

namespace zong
{
namespace platform
{

class BufferLayout
{
private:
    std::vector<BufferElement> _elements;
    uint32_t                   _stride;

public:
    inline uint32_t stride() const { return _stride; }

    const std::vector<BufferElement>& elements() const { return _elements; }

    std::vector<BufferElement>::iterator       begin() { return _elements.begin(); }
    std::vector<BufferElement>::iterator       end() { return _elements.end(); }
    std::vector<BufferElement>::const_iterator begin() const { return _elements.begin(); }
    std::vector<BufferElement>::const_iterator end() const { return _elements.end(); }

private:
    inline void setStride(uint32_t value) { _stride = value; }

public:
    BufferLayout() : _stride(0) {}
    BufferLayout(std::initializer_list<BufferElement> elements) : _elements(elements) { calculateOffsetsAndStride(); }

private:
    void calculateOffsetsAndStride()
    {
        size_t offset = 0;
        setStride(0);

        for (auto& element : _elements)
        {
            element._offset = offset;
            offset += element._size;
            setStride(stride() + element._size);
        }
    }
};

} // namespace platform
} // namespace zong