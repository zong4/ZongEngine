#include "Renderer2D.h"

#include <array>
#include <glm/gtc/matrix_transform.hpp>

#include "RenderCommand.h"
#include "buffer/VertexBuffer.h"
#include "shader/Shader.h"
#include "vao/VertexArray.h"

struct QuadVertex
{
    glm::vec3 Position;
    glm::vec4 Color;
    glm::vec2 TexCoord;
    float     TexIndex;
    float     TilingFactor;
};

struct Renderer2DData
{
    static const uint32_t MaxQuads        = 20000;
    static const uint32_t MaxVertices     = MaxQuads * 4;
    static const uint32_t MaxIndices      = MaxQuads * 6;
    static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

    std::shared_ptr<zong::platform::VertexArray>  QuadVertexArray;
    std::shared_ptr<zong::platform::VertexBuffer> QuadVertexBuffer;
    std::shared_ptr<zong::platform::Shader>       TextureShader;
    std::shared_ptr<zong::platform::Texture2D>    WhiteTexture;

    uint32_t    QuadIndexCount       = 0;
    QuadVertex* QuadVertexBufferBase = nullptr;
    QuadVertex* QuadVertexBufferPtr  = nullptr;

    std::array<std::shared_ptr<zong::platform::Texture2D>, MaxTextureSlots> TextureSlots;
    uint32_t                                                                TextureSlotIndex = 1; // 0 = white texture

    glm::vec4 QuadVertexPositions[4];

    zong::platform::Renderer2D::Statistics Stats;
};

static Renderer2DData s_Data;

void zong::platform::Renderer2D::Init()
{
    ZONG_PROFILE_FUNCTION();

    s_Data.QuadVertexArray = VertexArray::Create();

    s_Data.QuadVertexBuffer = VertexBuffer::create(s_Data.MaxVertices * sizeof(QuadVertex));
    s_Data.QuadVertexBuffer->setLayout({{ShaderDataType::Float3, "a_Position"},
                                        {ShaderDataType::Float4, "a_Color"},
                                        {ShaderDataType::Float2, "a_TexCoord"},
                                        {ShaderDataType::Float, "a_TexIndex"},
                                        {ShaderDataType::Float, "a_TilingFactor"}});
    s_Data.QuadVertexArray->addVertexBuffer(s_Data.QuadVertexBuffer);

    s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

    uint32_t* quadIndices = new uint32_t[s_Data.MaxIndices];

    uint32_t offset = 0;
    for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
    {
        quadIndices[i + 0] = offset + 0;
        quadIndices[i + 1] = offset + 1;
        quadIndices[i + 2] = offset + 2;

        quadIndices[i + 3] = offset + 2;
        quadIndices[i + 4] = offset + 3;
        quadIndices[i + 5] = offset + 0;

        offset += 4;
    }

    std::shared_ptr<IndexBuffer> quadIB = IndexBuffer::create(quadIndices, s_Data.MaxIndices);
    s_Data.QuadVertexArray->setIndexBuffer(quadIB);
    delete[] quadIndices;

    s_Data.WhiteTexture       = Texture2D::Create(TextureSpecification());
    uint32_t whiteTextureData = 0xffffffff;
    s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

    int32_t samplers[s_Data.MaxTextureSlots];
    for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++)
        samplers[i] = i;

    s_Data.TextureShader = Shader::Create("assets/shaders/Texture.glsl");
    s_Data.TextureShader->Bind();
    s_Data.TextureShader->SetIntArray("u_Textures", samplers, s_Data.MaxTextureSlots);

    // Set all texture slots to 0
    s_Data.TextureSlots[0] = s_Data.WhiteTexture;

    s_Data.QuadVertexPositions[0] = {-0.5f, -0.5f, 0.0f, 1.0f};
    s_Data.QuadVertexPositions[1] = {0.5f, -0.5f, 0.0f, 1.0f};
    s_Data.QuadVertexPositions[2] = {0.5f, 0.5f, 0.0f, 1.0f};
    s_Data.QuadVertexPositions[3] = {-0.5f, 0.5f, 0.0f, 1.0f};
}

void zong::platform::Renderer2D::Shutdown()
{
    ZONG_PROFILE_FUNCTION();

    delete[] s_Data.QuadVertexBufferBase;
}

void zong::platform::Renderer2D::BeginScene(const OrthographicCamera& camera)
{
    ZONG_PROFILE_FUNCTION();

    s_Data.TextureShader->Bind();
    s_Data.TextureShader->SetMat4("u_ViewProjection", camera.viewProjectionMatrix());

    s_Data.QuadIndexCount      = 0;
    s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

    s_Data.TextureSlotIndex = 1;
}

void zong::platform::Renderer2D::EndScene()
{
    ZONG_PROFILE_FUNCTION();

    uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);
    s_Data.QuadVertexBuffer->setData(s_Data.QuadVertexBufferBase, dataSize);

    Flush();
}

void zong::platform::Renderer2D::Flush()
{
    if (s_Data.QuadIndexCount == 0)
        return; // Nothing to draw

    // Bind textures
    for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
        s_Data.TextureSlots[i]->Bind(i);

    RenderCommand::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
    s_Data.Stats.DrawCalls++;
}

void zong::platform::Renderer2D::FlushAndReset()
{
    EndScene();

    s_Data.QuadIndexCount      = 0;
    s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

    s_Data.TextureSlotIndex = 1;
}

void zong::platform::Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
{
    DrawQuad({position.x, position.y, 0.0f}, size, color);
}

void zong::platform::Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
{
    ZONG_PROFILE_FUNCTION();

    constexpr size_t    quadVertexCount = 4;
    const float         textureIndex    = 0.0f; // White Texture
    constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    const float         tilingFactor    = 1.0f;

    if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
        FlushAndReset();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    for (size_t i = 0; i < quadVertexCount; i++)
    {
        s_Data.QuadVertexBufferPtr->Position     = transform * s_Data.QuadVertexPositions[i];
        s_Data.QuadVertexBufferPtr->Color        = color;
        s_Data.QuadVertexBufferPtr->TexCoord     = textureCoords[i];
        s_Data.QuadVertexBufferPtr->TexIndex     = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr++;
    }

    s_Data.QuadIndexCount += 6;

    s_Data.Stats.QuadCount++;
}

void zong::platform::Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const std::shared_ptr<Texture2D>& texture,
                                          float tilingFactor, const glm::vec4& tintColor)
{
    DrawQuad({position.x, position.y, 0.0f}, size, texture, tilingFactor, tintColor);
}

void zong::platform::Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const std::shared_ptr<Texture2D>& texture,
                                          float tilingFactor, const glm::vec4& tintColor)
{
    ZONG_PROFILE_FUNCTION();

    constexpr size_t    quadVertexCount = 4;
    constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
        FlushAndReset();

    float textureIndex = 0.0f;
    for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
    {
        if (*s_Data.TextureSlots[i].get() == *texture.get())
        {
            textureIndex = (float)i;
            break;
        }
    }

    if (textureIndex == 0.0f)
    {
        if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
            FlushAndReset();

        textureIndex                                 = (float)s_Data.TextureSlotIndex;
        s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
        s_Data.TextureSlotIndex++;
    }

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    for (size_t i = 0; i < quadVertexCount; i++)
    {
        s_Data.QuadVertexBufferPtr->Position     = transform * s_Data.QuadVertexPositions[i];
        s_Data.QuadVertexBufferPtr->Color        = tintColor;
        s_Data.QuadVertexBufferPtr->TexCoord     = textureCoords[i];
        s_Data.QuadVertexBufferPtr->TexIndex     = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr++;
    }

    s_Data.QuadIndexCount += 6;

    s_Data.Stats.QuadCount++;
}

void zong::platform::Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
{
    DrawRotatedQuad({position.x, position.y, 0.0f}, size, rotation, color);
}

void zong::platform::Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
{
    ZONG_PROFILE_FUNCTION();

    constexpr size_t    quadVertexCount = 4;
    const float         textureIndex    = 0.0f; // White Texture
    constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    const float         tilingFactor    = 1.0f;

    if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
        FlushAndReset();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                          glm::rotate(glm::mat4(1.0f), glm::radians(rotation), {0.0f, 0.0f, 1.0f}) *
                          glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    for (size_t i = 0; i < quadVertexCount; i++)
    {
        s_Data.QuadVertexBufferPtr->Position     = transform * s_Data.QuadVertexPositions[i];
        s_Data.QuadVertexBufferPtr->Color        = color;
        s_Data.QuadVertexBufferPtr->TexCoord     = textureCoords[i];
        s_Data.QuadVertexBufferPtr->TexIndex     = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr++;
    }

    s_Data.QuadIndexCount += 6;

    s_Data.Stats.QuadCount++;
}

void zong::platform::Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation,
                                                 const std::shared_ptr<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
{
    DrawRotatedQuad({position.x, position.y, 0.0f}, size, rotation, texture, tilingFactor, tintColor);
}

void zong::platform::Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation,
                                                 const std::shared_ptr<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
{
    ZONG_PROFILE_FUNCTION();

    constexpr size_t    quadVertexCount = 4;
    constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
        FlushAndReset();

    float textureIndex = 0.0f;
    for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
    {
        if (*s_Data.TextureSlots[i].get() == *texture.get())
        {
            textureIndex = (float)i;
            break;
        }
    }

    if (textureIndex == 0.0f)
    {
        if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
            FlushAndReset();

        textureIndex                                 = (float)s_Data.TextureSlotIndex;
        s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
        s_Data.TextureSlotIndex++;
    }

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                          glm::rotate(glm::mat4(1.0f), glm::radians(rotation), {0.0f, 0.0f, 1.0f}) *
                          glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    for (size_t i = 0; i < quadVertexCount; i++)
    {
        s_Data.QuadVertexBufferPtr->Position     = transform * s_Data.QuadVertexPositions[i];
        s_Data.QuadVertexBufferPtr->Color        = tintColor;
        s_Data.QuadVertexBufferPtr->TexCoord     = textureCoords[i];
        s_Data.QuadVertexBufferPtr->TexIndex     = textureIndex;
        s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
        s_Data.QuadVertexBufferPtr++;
    }

    s_Data.QuadIndexCount += 6;

    s_Data.Stats.QuadCount++;
}

void zong::platform::Renderer2D::ResetStats()
{
    memset(&s_Data.Stats, 0, sizeof(Statistics));
}

zong::platform::Renderer2D::Statistics zong::platform::Renderer2D::GetStats()
{
    return s_Data.Stats;
}
