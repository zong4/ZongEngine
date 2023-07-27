#pragma once

namespace zong
{
namespace platform
{

enum class ShaderDataType
{
    None = 0,
    Float,
    Float2,
    Float3,
    Float4,
    Mat3,
    Mat4,
    Int,
    Int2,
    Int3,
    Int4,
    Bool
};

static uint32_t ShaderDataTypeSize(ShaderDataType type)
{
    switch (type)
    {
        case ShaderDataType::Float:
            return 4;
        case ShaderDataType::Float2:
            return 4 * 2;
        case ShaderDataType::Float3:
            return 4 * 3;
        case ShaderDataType::Float4:
            return 4 * 4;
        case ShaderDataType::Mat3:
            return 4 * 3 * 3;
        case ShaderDataType::Mat4:
            return 4 * 4 * 4;
        case ShaderDataType::Int:
            return 4;
        case ShaderDataType::Int2:
            return 4 * 2;
        case ShaderDataType::Int3:
            return 4 * 3;
        case ShaderDataType::Int4:
            return 4 * 4;
        case ShaderDataType::Bool:
            return 1;
        default:
            ZONG_CORE_ASSERT(false, "unknown ShaderDataType!")
            return 0;
    }
}

struct BufferElement
{
    std::string    _name;
    ShaderDataType _type;
    uint32_t       _size;
    size_t         _offset;
    bool           _normalized;

    BufferElement() = default;
    BufferElement(ShaderDataType type, std::string const& name, bool normalized = false)
        : _name(name), _type(type), _size(ShaderDataTypeSize(type)), _offset(0), _normalized(normalized)
    {
    }

    uint32_t componentCount() const
    {
        switch (_type)
        {
            case ShaderDataType::Float:
                return 1;
            case ShaderDataType::Float2:
                return 2;
            case ShaderDataType::Float3:
                return 3;
            case ShaderDataType::Float4:
                return 4;
            case ShaderDataType::Mat3:
                return 3; // 3* float3
            case ShaderDataType::Mat4:
                return 4; // 4* float4
            case ShaderDataType::Int:
                return 1;
            case ShaderDataType::Int2:
                return 2;
            case ShaderDataType::Int3:
                return 3;
            case ShaderDataType::Int4:
                return 4;
            case ShaderDataType::Bool:
                return 1;
            default:
                ZONG_CORE_ASSERT(false, "unknown ShaderDataType!")
                return 0;
        }
    }
};

} // namespace platform
} // namespace zong