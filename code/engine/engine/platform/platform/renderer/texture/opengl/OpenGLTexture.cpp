#include "OpenGLTexture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace Utils
{

static GLenum HazelImageFormatToGLDataFormat(zong::platform::ImageFormat format)
{
    switch (format)
    {
        case zong::platform::ImageFormat::RGB8:
            return GL_RGB;
        case zong::platform::ImageFormat::RGBA8:
            return GL_RGBA;
    }

    ZONG_CORE_ASSERT(false, "!23")
    return 0;
}

static GLenum HazelImageFormatToGLInternalFormat(zong::platform::ImageFormat format)
{
    switch (format)
    {
        case zong::platform::ImageFormat::RGB8:
            return GL_RGB8;
        case zong::platform::ImageFormat::RGBA8:
            return GL_RGBA8;
    }

    ZONG_CORE_ASSERT(false, "123")
    return 0;
}

} // namespace Utils

zong::platform::OpenGLTexture2D::OpenGLTexture2D(const TextureSpecification& specification)
    : m_Specification(specification), m_Width(m_Specification.Width), m_Height(m_Specification.Height)
{
    ZONG_PROFILE_FUNCTION();

    m_InternalFormat = Utils::HazelImageFormatToGLInternalFormat(m_Specification.Format);
    m_DataFormat     = Utils::HazelImageFormatToGLDataFormat(m_Specification.Format);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
    glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

    glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

zong::platform::OpenGLTexture2D::OpenGLTexture2D(const std::string& path) : m_Path(path)
{
    ZONG_PROFILE_FUNCTION();

    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    stbi_uc* data = nullptr;
    {
        ZONG_PROFILE_SCOPE("stbi_load - OpenGLTexture2D::OpenGLTexture2D(const std::string&)");
        data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    }

    if (data)
    {
        m_IsLoaded = true;

        m_Width  = width;
        m_Height = height;

        GLenum internalFormat = 0, dataFormat = 0;
        if (channels == 4)
        {
            internalFormat = GL_RGBA8;
            dataFormat     = GL_RGBA;
        }
        else if (channels == 3)
        {
            internalFormat = GL_RGB8;
            dataFormat     = GL_RGB;
        }

        m_InternalFormat = internalFormat;
        m_DataFormat     = dataFormat;

        ZONG_CORE_ASSERT(internalFormat & dataFormat, "Format not supported!");

        glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
        glTextureStorage2D(m_RendererID, 1, internalFormat, m_Width, m_Height);

        glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    }
}

zong::platform::OpenGLTexture2D::~OpenGLTexture2D()
{
    ZONG_PROFILE_FUNCTION();

    glDeleteTextures(1, &m_RendererID);
}

void zong::platform::OpenGLTexture2D::SetData(void* data, uint32_t size)
{
    ZONG_PROFILE_FUNCTION();

    uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
    ZONG_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
    glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
}

void zong::platform::OpenGLTexture2D::Bind(uint32_t slot) const
{
    ZONG_PROFILE_FUNCTION();

    glBindTextureUnit(slot, m_RendererID);
}
