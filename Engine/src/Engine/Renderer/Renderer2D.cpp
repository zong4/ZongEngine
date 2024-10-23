#include "hzpch.h"
#include "Renderer2D.h"

#include "Engine/Renderer/Pipeline.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/RenderCommandBuffer.h"

#include <glm/gtc/matrix_transform.hpp>

// TEMP
#include "Engine/Platform/Vulkan/VulkanRenderCommandBuffer.h"

#include "Engine/Renderer/UI/MSDFData.h"

#include <codecvt>

namespace Hazel {

	Renderer2D::Renderer2D(const Renderer2DSpecification& specification)
		: m_Specification(specification),
		  c_MaxVertices(specification.MaxQuads * 4),
		  c_MaxIndices(specification.MaxQuads * 6),
		  c_MaxLineVertices(specification.MaxLines * 2),
		  c_MaxLineIndices(specification.MaxLines * 6)
	{
		Init();
	}

	Renderer2D::~Renderer2D()
	{
		Shutdown();
	}

	void Renderer2D::Init()
	{
		if (m_Specification.SwapChainTarget)
			m_RenderCommandBuffer = RenderCommandBuffer::CreateFromSwapChain("Renderer2D");
		else
			m_RenderCommandBuffer = RenderCommandBuffer::Create(0, "Renderer2D");

		m_UBSCamera = UniformBufferSet::Create(sizeof(UBCamera));

		m_MemoryStats.TotalAllocated = 0;

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		FramebufferSpecification framebufferSpec;
		framebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };
		framebufferSpec.Samples = 1;
		framebufferSpec.ClearColorOnLoad = false;
		framebufferSpec.ClearColor = { 0.1f, 0.5f, 0.5f, 1.0f };
		framebufferSpec.DebugName = "Renderer2D Framebuffer";

		Ref<Framebuffer> framebuffer = Framebuffer::Create(framebufferSpec);

		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "Renderer2D-Quad";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Renderer2D");
			pipelineSpecification.TargetFramebuffer = framebuffer;
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float4, "a_Color" },
				{ ShaderDataType::Float2, "a_TexCoord" },
				{ ShaderDataType::Float, "a_TexIndex" },
				{ ShaderDataType::Float, "a_TilingFactor" }
			};

			RenderPassSpecification quadSpec;
			quadSpec.DebugName = "Renderer2D-Quad";
			quadSpec.Pipeline = Pipeline::Create(pipelineSpecification);
			m_QuadPass = RenderPass::Create(quadSpec);
			m_QuadPass->SetInput("Camera", m_UBSCamera);
			HZ_CORE_VERIFY(m_QuadPass->Validate());
			m_QuadPass->Bake();

			m_QuadVertexBuffers.resize(1);
			m_QuadVertexBufferBases.resize(1);
			m_QuadVertexBufferPtr.resize(1);

			m_QuadVertexBuffers[0].resize(framesInFlight);
			m_QuadVertexBufferBases[0].resize(framesInFlight);
			for (uint32_t i = 0; i < framesInFlight; i++)
			{
				uint64_t allocationSize = c_MaxVertices * sizeof(QuadVertex);
				m_QuadVertexBuffers[0][i] = VertexBuffer::Create(allocationSize);
				m_MemoryStats.TotalAllocated += allocationSize;
				m_QuadVertexBufferBases[0][i] = hnew QuadVertex[c_MaxVertices];
			}

			uint32_t* quadIndices = hnew uint32_t[c_MaxIndices];

			uint32_t offset = 0;
			for (uint32_t i = 0; i < c_MaxIndices; i += 6)
			{
				quadIndices[i + 0] = offset + 0;
				quadIndices[i + 1] = offset + 1;
				quadIndices[i + 2] = offset + 2;

				quadIndices[i + 3] = offset + 2;
				quadIndices[i + 4] = offset + 3;
				quadIndices[i + 5] = offset + 0;

				offset += 4;
			}

			{
				uint64_t allocationSize = c_MaxIndices * sizeof(uint32_t);
				m_QuadIndexBuffer = IndexBuffer::Create(quadIndices, allocationSize);
				m_MemoryStats.TotalAllocated += allocationSize;
			}
			hdelete[] quadIndices;
		}

		m_WhiteTexture = Renderer::GetWhiteTexture();

		// Set all texture slots to 0
		m_TextureSlots[0] = m_WhiteTexture;

		m_QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		m_QuadVertexPositions[1] = { -0.5f,  0.5f, 0.0f, 1.0f };
		m_QuadVertexPositions[2] = {  0.5f,  0.5f, 0.0f, 1.0f };
		m_QuadVertexPositions[3] = {  0.5f, -0.5f, 0.0f, 1.0f };

		// Lines
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "Renderer2D-Line";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Renderer2D_Line");
			pipelineSpecification.TargetFramebuffer = framebuffer;
			pipelineSpecification.Topology = PrimitiveTopology::Lines;
			pipelineSpecification.LineWidth = 2.0f;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float4, "a_Color" }
			};

			{
				RenderPassSpecification lineSpec;
				lineSpec.DebugName = "Renderer2D-Line";
				lineSpec.Pipeline = Pipeline::Create(pipelineSpecification);
				m_LinePass = RenderPass::Create(lineSpec);
				m_LinePass->SetInput("Camera", m_UBSCamera);
				HZ_CORE_VERIFY(m_LinePass->Validate());
				m_LinePass->Bake();
			}

			{
				RenderPassSpecification lineOnTopSpec;
				lineOnTopSpec.DebugName = "Renderer2D-Line(OnTop)";
				pipelineSpecification.DepthTest = false;
				lineOnTopSpec.Pipeline = Pipeline::Create(pipelineSpecification);
				m_LineOnTopPass = RenderPass::Create(lineOnTopSpec);
				m_LineOnTopPass->SetInput("Camera", m_UBSCamera);
				HZ_CORE_VERIFY(m_LineOnTopPass->Validate());
				m_LineOnTopPass->Bake();
			}

			m_LineVertexBuffers.resize(1);
			m_LineVertexBufferBases.resize(1);
			m_LineVertexBufferPtr.resize(1);

			m_LineVertexBuffers[0].resize(framesInFlight);
			m_LineVertexBufferBases[0].resize(framesInFlight);
			for (uint32_t i = 0; i < framesInFlight; i++)
			{
				uint64_t allocationSize = c_MaxLineVertices * sizeof(LineVertex);
				m_LineVertexBuffers[0][i] = VertexBuffer::Create(allocationSize);
				m_MemoryStats.TotalAllocated += allocationSize;
				m_LineVertexBufferBases[0][i] = hnew LineVertex[c_MaxLineVertices];
			}

			uint32_t* lineIndices = hnew uint32_t[c_MaxLineIndices];
			for (uint32_t i = 0; i < c_MaxLineIndices; i++)
				lineIndices[i] = i;

			{
				uint64_t allocationSize = c_MaxLineIndices * sizeof(uint32_t);
				m_LineIndexBuffer = IndexBuffer::Create(lineIndices, allocationSize);
				m_MemoryStats.TotalAllocated += allocationSize;
			}
			hdelete[] lineIndices;
		}

		// Text
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "Renderer2D-Text";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Renderer2D_Text");
			pipelineSpecification.TargetFramebuffer = framebuffer;
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float4, "a_Color" },
				{ ShaderDataType::Float2, "a_TexCoord" },
				{ ShaderDataType::Float, "a_TexIndex" }
			};

			RenderPassSpecification textSpec;
			textSpec.DebugName = "Renderer2D-Text";
			textSpec.Pipeline = Pipeline::Create(pipelineSpecification);
			m_TextPass = RenderPass::Create(textSpec);
			m_TextPass->SetInput("Camera", m_UBSCamera);
			HZ_CORE_VERIFY(m_TextPass->Validate());
			m_TextPass->Bake();

			m_TextMaterial = Material::Create(pipelineSpecification.Shader);

			m_TextVertexBuffers.resize(1);
			m_TextVertexBufferBases.resize(1);
			m_TextVertexBufferPtr.resize(1);

			m_TextVertexBuffers[0].resize(framesInFlight);
			m_TextVertexBufferBases[0].resize(framesInFlight);
			for (uint32_t i = 0; i < framesInFlight; i++)
			{
				uint64_t allocationSize = c_MaxVertices * sizeof(TextVertex);
				m_TextVertexBuffers[0][i] = VertexBuffer::Create(allocationSize);
				m_MemoryStats.TotalAllocated += allocationSize;
				m_TextVertexBufferBases[0][i] = hnew TextVertex[c_MaxVertices];
			}

			uint32_t* textQuadIndices = hnew uint32_t[c_MaxIndices];

			uint32_t offset = 0;
			for (uint32_t i = 0; i < c_MaxIndices; i += 6)
			{
				textQuadIndices[i + 0] = offset + 0;
				textQuadIndices[i + 1] = offset + 1;
				textQuadIndices[i + 2] = offset + 2;

				textQuadIndices[i + 3] = offset + 2;
				textQuadIndices[i + 4] = offset + 3;
				textQuadIndices[i + 5] = offset + 0;

				offset += 4;
			}

			{
				uint64_t allocationSize = c_MaxIndices * sizeof(uint32_t);
				m_TextIndexBuffer = IndexBuffer::Create(textQuadIndices, allocationSize);
				m_MemoryStats.TotalAllocated += allocationSize;
			}
			hdelete[] textQuadIndices;
		}

		// Circles
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "Renderer2D-Circle";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Renderer2D_Circle");
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.TargetFramebuffer = framebuffer;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_WorldPosition" },
				{ ShaderDataType::Float,  "a_Thickness" },
				{ ShaderDataType::Float2, "a_LocalPosition" },
				{ ShaderDataType::Float4, "a_Color" }
			};
			m_CirclePipeline = Pipeline::Create(pipelineSpecification);
			m_CircleMaterial = Material::Create(pipelineSpecification.Shader);

			m_CircleVertexBuffers.resize(1);
			m_CircleVertexBufferBases.resize(1);
			m_CircleVertexBufferPtr.resize(1);

			m_CircleVertexBuffers[0].resize(framesInFlight);
			m_CircleVertexBufferBases[0].resize(framesInFlight);
			for (uint32_t i = 0; i < framesInFlight; i++)
			{
				uint64_t allocationSize = c_MaxVertices * sizeof(QuadVertex);
				m_CircleVertexBuffers[0][i] = VertexBuffer::Create(allocationSize);
				m_MemoryStats.TotalAllocated += allocationSize;
				m_CircleVertexBufferBases[0][i] = hnew CircleVertex[c_MaxVertices];
			}
		}

		m_QuadMaterial = Material::Create(m_QuadPass->GetPipeline()->GetShader(), "QuadMaterial");
		m_LineMaterial = Material::Create(m_LinePass->GetPipeline()->GetShader(), "LineMaterial");
	}

	void Renderer2D::Shutdown()
	{
		for (auto buffers : m_QuadVertexBufferBases)
		{
			for (auto buffer : buffers)
				hdelete[] buffer;
		}

		for (auto buffers : m_TextVertexBufferBases)
		{
			for (auto buffer : buffers)
				hdelete[] buffer;
		}

		for (auto buffers : m_LineVertexBufferBases)
		{
			for (auto buffer : buffers)
				hdelete[] buffer;
		}

		for (auto buffers : m_CircleVertexBufferBases)
		{
			for (auto buffer : buffers)
				hdelete[] buffer;
		}
	}

	void Renderer2D::BeginScene(const glm::mat4& viewProj, const glm::mat4& view, bool depthTest)
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		const bool updatedAnyShaders = Renderer::UpdateDirtyShaders();
		if(updatedAnyShaders)
		{
			// Update materials that aren't set on use.
		}
		m_CameraViewProj = viewProj;
		m_CameraView = view;
		m_DepthTest = depthTest;

		Renderer::Submit([ubsCamera = m_UBSCamera, viewProj]() mutable
		{
			uint32_t bufferIndex = Renderer::RT_GetCurrentFrameIndex();
			ubsCamera->RT_Get()->RT_SetData(&viewProj, sizeof(UBCamera));
		});

		HZ_CORE_TRACE_TAG("Renderer", "Renderer2D::BeginScene frame {}", frameIndex);

		m_QuadIndexCount = 0;
		for (uint32_t i = 0; i < m_QuadVertexBufferPtr.size(); i++)
			m_QuadVertexBufferPtr[i] = m_QuadVertexBufferBases[i][frameIndex];

		m_TextIndexCount = 0;
		for (uint32_t i = 0; i < m_TextVertexBufferPtr.size(); i++)
			m_TextVertexBufferPtr[i] = m_TextVertexBufferBases[i][frameIndex];

		m_LineIndexCount = 0;
		for (uint32_t i = 0; i < m_LineVertexBufferPtr.size(); i++)
			m_LineVertexBufferPtr[i] = m_LineVertexBufferBases[i][frameIndex];

		m_CircleIndexCount = 0;
		for (uint32_t i = 0; i < m_CircleVertexBufferPtr.size(); i++)
			m_CircleVertexBufferPtr[i] = m_CircleVertexBufferBases[i][frameIndex];

		m_TextureSlotIndex = 1;
		m_FontTextureSlotIndex = 0;

		m_TextBufferWriteIndex = 0;

		for (uint32_t i = 1; i < m_TextureSlots.size(); i++)
			m_TextureSlots[i] = nullptr;

		for (uint32_t i = 0; i < m_FontTextureSlots.size(); i++)
			m_FontTextureSlots[i] = nullptr;
	}

	void Renderer2D::EndScene()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		m_RenderCommandBuffer->Begin();

		HZ_CORE_TRACE_TAG("Renderer", "Renderer2D::EndScene frame {}", frameIndex);
		uint32_t dataSize = 0;
		
		// Quads
		for (uint32_t i = 0; i <= m_QuadBufferWriteIndex; i++)
		{
			dataSize = (uint32_t)((uint8_t*)m_QuadVertexBufferPtr[i] - (uint8_t*)m_QuadVertexBufferBases[i][frameIndex]);
			if (dataSize)
			{
				uint32_t indexCount = i == m_QuadBufferWriteIndex ? m_QuadIndexCount - (c_MaxIndices * i) : c_MaxIndices;
				m_QuadVertexBuffers[i][frameIndex]->SetData(m_QuadVertexBufferBases[i][frameIndex], dataSize);

				for (uint32_t i = 0; i < m_TextureSlots.size(); i++)
				{
					if (m_TextureSlots[i])
						m_QuadMaterial->Set("u_Textures", m_TextureSlots[i], i);
					else
						m_QuadMaterial->Set("u_Textures", m_WhiteTexture, i);
				}

				Renderer::BeginRenderPass(m_RenderCommandBuffer, m_QuadPass);
				Renderer::RenderGeometry(m_RenderCommandBuffer, m_QuadPass->GetPipeline(), m_QuadMaterial, m_QuadVertexBuffers[i][frameIndex], m_QuadIndexBuffer, glm::mat4(1.0f), indexCount);
				Renderer::EndRenderPass(m_RenderCommandBuffer);

				m_DrawStats.DrawCalls++;
				m_MemoryStats.Used += dataSize;
			}

		}

		// Render text
		for (uint32_t i = 0; i <= m_TextBufferWriteIndex; i++)
		{
			dataSize = (uint32_t)((uint8_t*)m_TextVertexBufferPtr[i] - (uint8_t*)m_TextVertexBufferBases[i][frameIndex]);
			if (dataSize)
			{
				uint32_t indexCount = i == m_TextBufferWriteIndex ? m_TextIndexCount - (c_MaxIndices * i) : c_MaxIndices;
				m_TextVertexBuffers[i][frameIndex]->SetData(m_TextVertexBufferBases[i][frameIndex], dataSize);

				for (uint32_t i = 0; i < m_FontTextureSlots.size(); i++)
				{
					if (m_FontTextureSlots[i])
						m_TextMaterial->Set("u_FontAtlases", m_FontTextureSlots[i], i);
					else
						m_TextMaterial->Set("u_FontAtlases", m_WhiteTexture, i);
				}

				Renderer::BeginRenderPass(m_RenderCommandBuffer, m_TextPass);
				Renderer::RenderGeometry(m_RenderCommandBuffer, m_TextPass->GetSpecification().Pipeline, m_TextMaterial, m_TextVertexBuffers[i][frameIndex], m_TextIndexBuffer, glm::mat4(1.0f), indexCount);
				Renderer::EndRenderPass(m_RenderCommandBuffer);

				m_DrawStats.DrawCalls++;
				m_MemoryStats.Used += dataSize;
			}

		}

		// Lines
		Renderer::Submit([lineWidth = m_LineWidth, renderCommandBuffer = m_RenderCommandBuffer]()
		{
			uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);
			vkCmdSetLineWidth(commandBuffer, lineWidth);
		});

		for (uint32_t i = 0; i <= m_LineBufferWriteIndex; i++)
		{
			dataSize = (uint32_t)((uint8_t*)m_LineVertexBufferPtr[i] - (uint8_t*)m_LineVertexBufferBases[i][frameIndex]);
			if (dataSize)
			{
				uint32_t indexCount = i == m_LineBufferWriteIndex ? m_LineIndexCount - (c_MaxLineIndices * i) : c_MaxLineIndices;
				m_LineVertexBuffers[i][frameIndex]->SetData(m_LineVertexBufferBases[i][frameIndex], dataSize);

				Renderer::BeginRenderPass(m_RenderCommandBuffer, m_LinePass);
				Renderer::RenderGeometry(m_RenderCommandBuffer, m_LinePass->GetSpecification().Pipeline, m_LineMaterial, m_LineVertexBuffers[i][frameIndex], m_LineIndexBuffer, glm::mat4(1.0f), indexCount);
				Renderer::EndRenderPass(m_RenderCommandBuffer);

				m_DrawStats.DrawCalls++;
				m_MemoryStats.Used += dataSize;
			}

		}

#if TODO
		// Circles
		for (uint32_t i = 0; i <= m_CircleBufferWriteIndex; i++)
		{
			dataSize = (uint32_t)((uint8_t*)m_CircleVertexBufferPtr[i] - (uint8_t*)m_CircleVertexBufferBases[i][frameIndex]);
			if (dataSize)
			{
				uint32_t indexCount = i == m_LineBufferWriteIndex ? m_LineIndexCount - (c_MaxIndices * i) : c_MaxIndices;
				m_LineVertexBuffers[i][frameIndex]->SetData(m_CircleVertexBufferBases[i][frameIndex], dataSize);

				Renderer::BeginRenderPass(m_RenderCommandBuffer, m_LinePass);
				Renderer::RenderGeometry(m_RenderCommandBuffer, m_LinePass->GetSpecification().Pipeline, m_LineMaterial, m_LineVertexBuffers[i][frameIndex], m_LineIndexBuffer, glm::mat4(1.0f), indexCount);
				Renderer::EndRenderPass(m_RenderCommandBuffer);

				m_DrawStats.DrawCalls++;
				m_MemoryStats.Used += dataSize;
			}

		}

		// OLD
		dataSize = (uint32_t)((uint8_t*)m_CircleVertexBufferPtr - (uint8_t*)m_CircleVertexBufferBase[frameIndex]);
		if (dataSize)
		{
			m_CircleVertexBuffer[frameIndex]->SetData(m_CircleVertexBufferBase[frameIndex], dataSize);
			//TODO: Renderer::RenderGeometry(m_RenderCommandBuffer, m_CirclePipeline, m_CircleMaterial, m_CircleVertexBuffer[frameIndex], m_QuadIndexBuffer, glm::mat4(1.0f), m_CircleIndexCount);

			m_DrawStats.DrawCalls++;
			m_MemoryStats.Used += dataSize;
		}
#endif

		m_RenderCommandBuffer->End();
		m_RenderCommandBuffer->Submit();
	}

	void Renderer2D::Flush()
	{
		// TODO(Yan)
	}

	Ref<RenderPass> Renderer2D::GetTargetRenderPass()
	{
		// TODO return m_QuadPipeline->GetSpecification().RenderPass;
		return nullptr;
	}

	void Renderer2D::SetTargetFramebuffer(Ref<Framebuffer> framebuffer)
	{
		if (framebuffer != m_TextPass->GetTargetFramebuffer())
		{
			{
				PipelineSpecification pipelineSpec = m_QuadPass->GetSpecification().Pipeline->GetSpecification();
				pipelineSpec.TargetFramebuffer = framebuffer;
				RenderPassSpecification& renderpassSpec = m_QuadPass->GetSpecification();
				renderpassSpec.Pipeline = Pipeline::Create(pipelineSpec);
			}

			{
				PipelineSpecification pipelineSpec = m_LinePass->GetSpecification().Pipeline->GetSpecification();
				pipelineSpec.TargetFramebuffer = framebuffer;
				RenderPassSpecification& renderpassSpec = m_LinePass->GetSpecification();
				renderpassSpec.Pipeline = Pipeline::Create(pipelineSpec);
			}
			{
				PipelineSpecification pipelineSpec = m_TextPass->GetSpecification().Pipeline->GetSpecification();
				pipelineSpec.TargetFramebuffer = framebuffer;
				RenderPassSpecification& renderpassSpec = m_TextPass->GetSpecification();
				renderpassSpec.Pipeline = Pipeline::Create(pipelineSpec);
			}
		}
	}

	void Renderer2D::OnRecreateSwapchain()
	{
		if (m_Specification.SwapChainTarget)
			m_RenderCommandBuffer = RenderCommandBuffer::CreateFromSwapChain("Renderer2D");
	}

	void Renderer2D::AddQuadBuffer()
	{
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		VertexBufferPerFrame& newVertexBuffer = m_QuadVertexBuffers.emplace_back();
		QuadVertexBasePerFrame& newVertexBufferBase = m_QuadVertexBufferBases.emplace_back();

		newVertexBuffer.resize(framesInFlight);
		newVertexBufferBase.resize(framesInFlight);
		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			uint64_t allocationSize = c_MaxVertices * sizeof(QuadVertex);
			newVertexBuffer[i] = VertexBuffer::Create(allocationSize);
			m_MemoryStats.TotalAllocated += allocationSize;
			newVertexBufferBase[i] = hnew QuadVertex[c_MaxVertices];
		}
	}
	
	void Renderer2D::AddLineBuffer()
	{
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		VertexBufferPerFrame& newVertexBuffer = m_LineVertexBuffers.emplace_back();
		LineVertexBasePerFrame& newVertexBufferBase = m_LineVertexBufferBases.emplace_back();

		newVertexBuffer.resize(framesInFlight);
		newVertexBufferBase.resize(framesInFlight);
		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			uint64_t allocationSize = c_MaxLineVertices * sizeof(LineVertex);
			newVertexBuffer[i] = VertexBuffer::Create(allocationSize);
			m_MemoryStats.TotalAllocated += allocationSize;
			newVertexBufferBase[i] = hnew LineVertex[c_MaxVertices];
		}
	}

	void Renderer2D::AddTextBuffer()
	{
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		VertexBufferPerFrame& newVertexBuffer = m_TextVertexBuffers.emplace_back();
		TextVertexBasePerFrame& newVertexBufferBase = m_TextVertexBufferBases.emplace_back();

		newVertexBuffer.resize(framesInFlight);
		newVertexBufferBase.resize(framesInFlight);
		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			uint64_t allocationSize = c_MaxVertices * sizeof(TextVertex);
			newVertexBuffer[i] = VertexBuffer::Create(allocationSize);
			m_MemoryStats.TotalAllocated += allocationSize;
			newVertexBufferBase[i] = hnew TextVertex[c_MaxVertices];
		}
	}


	void Renderer2D::AddCircleBuffer()
	{
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		VertexBufferPerFrame& newVertexBuffer = m_CircleVertexBuffers.emplace_back();
		CircleVertexBasePerFrame& newVertexBufferBase = m_CircleVertexBufferBases.emplace_back();

		newVertexBuffer.resize(framesInFlight);
		newVertexBufferBase.resize(framesInFlight);
		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			uint64_t allocationSize = c_MaxVertices * sizeof(CircleVertex);
			newVertexBuffer[i] = VertexBuffer::Create(allocationSize);
			m_MemoryStats.TotalAllocated += allocationSize;
			newVertexBufferBase[i] = hnew CircleVertex[c_MaxVertices];
		}
	}

	Renderer2D::QuadVertex*& Renderer2D::GetWriteableQuadBuffer()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		m_QuadBufferWriteIndex = m_QuadIndexCount / c_MaxIndices;
		if (m_QuadBufferWriteIndex >= m_QuadVertexBufferBases.size())
		{
			AddQuadBuffer();
			m_QuadVertexBufferPtr.emplace_back(); // TODO(Yan): check
			m_QuadVertexBufferPtr[m_QuadBufferWriteIndex] = m_QuadVertexBufferBases[m_QuadBufferWriteIndex][frameIndex];
		}

		return m_QuadVertexBufferPtr[m_QuadBufferWriteIndex];
	}

	Renderer2D::LineVertex*& Renderer2D::GetWriteableLineBuffer()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		m_LineBufferWriteIndex = m_LineIndexCount / c_MaxIndices;
		if (m_LineBufferWriteIndex >= m_LineVertexBufferBases.size())
		{
			AddLineBuffer();
			m_LineVertexBufferPtr.emplace_back(); // TODO(Yan): check
			m_LineVertexBufferPtr[m_LineBufferWriteIndex] = m_LineVertexBufferBases[m_LineBufferWriteIndex][frameIndex];
		}

		return m_LineVertexBufferPtr[m_LineBufferWriteIndex];
	}

	Renderer2D::TextVertex*& Renderer2D::GetWriteableTextBuffer()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		m_TextBufferWriteIndex = m_TextIndexCount / c_MaxIndices;
		if (m_TextBufferWriteIndex >= m_TextVertexBufferBases.size())
		{
			AddTextBuffer();
			m_TextVertexBufferPtr.emplace_back(); // TODO(Yan): check
			m_TextVertexBufferPtr[m_TextBufferWriteIndex] = m_TextVertexBufferBases[m_TextBufferWriteIndex][frameIndex];
		}

		return m_TextVertexBufferPtr[m_TextBufferWriteIndex];
	}

	Renderer2D::CircleVertex*& Renderer2D::GetWriteableCircleBuffer()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		m_CircleBufferWriteIndex = m_CircleIndexCount / c_MaxIndices;
		if (m_CircleBufferWriteIndex >= m_CircleVertexBufferBases.size())
		{
			AddCircleBuffer();
			m_CircleVertexBufferPtr.emplace_back(); // TODO(Yan): check
			m_CircleVertexBufferPtr[m_CircleBufferWriteIndex] = m_CircleVertexBufferBases[m_CircleBufferWriteIndex][frameIndex];
		}

		return m_CircleVertexBufferPtr[m_CircleBufferWriteIndex];
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color)
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		constexpr size_t quadVertexCount = 4;
		const float textureIndex = 0.0f; // White Texture
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float tilingFactor = 1.0f;

		m_QuadBufferWriteIndex = m_QuadIndexCount / c_MaxIndices;
		if (m_QuadBufferWriteIndex >= m_QuadVertexBufferBases.size())
		{
			AddQuadBuffer();
			m_QuadVertexBufferPtr.emplace_back(); // TODO(Yan): check
			m_QuadVertexBufferPtr[m_QuadBufferWriteIndex] = m_QuadVertexBufferBases[m_QuadBufferWriteIndex][frameIndex];
		}

		auto& bufferPtr = m_QuadVertexBufferPtr[m_QuadBufferWriteIndex];
		for (size_t i = 0; i < quadVertexCount; i++)
		{
			bufferPtr->Position = transform * m_QuadVertexPositions[i];
			bufferPtr->Color = color;
			bufferPtr->TexCoord = textureCoords[i];
			bufferPtr->TexIndex = textureIndex;
			bufferPtr->TilingFactor = tilingFactor;
			bufferPtr++;
		}

		m_QuadIndexCount += 6;

		m_DrawStats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, glm::vec2 uv0, glm::vec2 uv1)
	{
		constexpr size_t quadVertexCount = 4;
		glm::vec2 textureCoords[] = { uv0, { uv1.x, uv0.y }, uv1, { uv0.x, uv1.y } };

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < m_TextureSlotIndex; i++)
		{
			if (m_TextureSlots[i]->GetHash() == texture->GetHash())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			//if (m_TextureSlotIndex >= MaxTextureSlots)
			//	FlushAndReset();

			textureIndex = (float)m_TextureSlotIndex;
			m_TextureSlots[m_TextureSlotIndex] = texture;
			m_TextureSlotIndex++;
		}

		auto& bufferPtr = m_QuadVertexBufferPtr[m_QuadBufferWriteIndex];
		for (size_t i = 0; i < quadVertexCount; i++)
		{
			bufferPtr->Position = transform * m_QuadVertexPositions[i];
			bufferPtr->Color = tintColor;
			bufferPtr->TexCoord = textureCoords[i];
			bufferPtr->TexIndex = textureIndex;
			bufferPtr->TilingFactor = tilingFactor;
			bufferPtr++;
		}

		m_QuadIndexCount += 6;

		m_DrawStats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		const float textureIndex = 0.0f; // White Texture
		const float tilingFactor = 1.0f;

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		auto& bufferPtr = GetWriteableQuadBuffer();
		bufferPtr->Position = transform * m_QuadVertexPositions[0];
		bufferPtr->Color = color;
		bufferPtr->TexCoord = { 0.0f, 0.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = transform * m_QuadVertexPositions[1];
		bufferPtr->Color = color;
		bufferPtr->TexCoord = { 1.0f, 0.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = transform * m_QuadVertexPositions[2];
		bufferPtr->Color = color;
		bufferPtr->TexCoord = { 1.0f, 1.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = transform * m_QuadVertexPositions[3];
		bufferPtr->Color = color;
		bufferPtr->TexCoord = { 0.0f, 1.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		m_QuadIndexCount += 6;

		m_DrawStats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, glm::vec2 uv0, glm::vec2 uv1)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor, uv0, uv1);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, glm::vec2 uv0, glm::vec2 uv1)
	{
		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < m_TextureSlotIndex; i++)
		{
			if (*m_TextureSlots[i].Raw() == *texture.Raw())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)m_TextureSlotIndex;
			m_TextureSlots[m_TextureSlotIndex] = texture;
			m_TextureSlotIndex++;
		}
		
		glm::vec2 textureCoords[] = { uv0, { uv1.x, uv0.y }, uv1, { uv0.x, uv1.y } };

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		auto& bufferPtr = GetWriteableQuadBuffer();
		bufferPtr->Position = transform * m_QuadVertexPositions[0];
		bufferPtr->Color = tintColor;
		bufferPtr->TexCoord = textureCoords[0];
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = transform * m_QuadVertexPositions[1];
		bufferPtr->Color = tintColor;
		bufferPtr->TexCoord = textureCoords[1];
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = transform * m_QuadVertexPositions[2];
		bufferPtr->Color = tintColor;
		bufferPtr->TexCoord = textureCoords[2];
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = transform * m_QuadVertexPositions[3];
		bufferPtr->Color = tintColor;
		bufferPtr->TexCoord = textureCoords[3];
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		m_QuadIndexCount += 6;

		m_DrawStats.QuadCount++;
	}

	void Renderer2D::DrawQuadBillboard(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		const float textureIndex = 0.0f; // White Texture
		const float tilingFactor = 1.0f;

		glm::vec3 camRightWS = { m_CameraView[0][0], m_CameraView[1][0], m_CameraView[2][0] };
		glm::vec3 camUpWS = { m_CameraView[0][1], m_CameraView[1][1], m_CameraView[2][1] };

		auto& bufferPtr = GetWriteableQuadBuffer();
		bufferPtr->Position = position + camRightWS * (m_QuadVertexPositions[0].x) * size.x + camUpWS * m_QuadVertexPositions[0].y * size.y;
		bufferPtr->Color = color;
		bufferPtr->TexCoord = { 0.0f, 0.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = position + camRightWS * m_QuadVertexPositions[1].x * size.x + camUpWS * m_QuadVertexPositions[1].y * size.y;
		bufferPtr->Color = color;
		bufferPtr->TexCoord = { 1.0f, 0.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = position + camRightWS * m_QuadVertexPositions[2].x * size.x + camUpWS * m_QuadVertexPositions[2].y * size.y;
		bufferPtr->Color = color;
		bufferPtr->TexCoord = { 1.0f, 1.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = position + camRightWS * m_QuadVertexPositions[3].x * size.x + camUpWS * m_QuadVertexPositions[3].y * size.y;
		bufferPtr->Color = color;
		bufferPtr->TexCoord = { 0.0f, 1.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		m_QuadIndexCount += 6;

		m_DrawStats.QuadCount++;
	}

	void Renderer2D::DrawQuadBillboard(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < m_TextureSlotIndex; i++)
		{
			if (m_TextureSlots[i]->GetHash() == texture->GetHash())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)m_TextureSlotIndex;
			m_TextureSlots[m_TextureSlotIndex] = texture;
			m_TextureSlotIndex++;
		}

		glm::vec3 camRightWS = { m_CameraView[0][0], m_CameraView[1][0], m_CameraView[2][0] };
		glm::vec3 camUpWS = { m_CameraView[0][1], m_CameraView[1][1], m_CameraView[2][1] };

		auto& bufferPtr = GetWriteableQuadBuffer();
		bufferPtr->Position = position + camRightWS * (m_QuadVertexPositions[0].x) * size.x + camUpWS * m_QuadVertexPositions[0].y * size.y;
		bufferPtr->Color = tintColor;
		bufferPtr->TexCoord = { 0.0f, 1.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = position + camRightWS * m_QuadVertexPositions[1].x * size.x + camUpWS * m_QuadVertexPositions[1].y * size.y;
		bufferPtr->Color = tintColor;
		bufferPtr->TexCoord = { 0.0f, 0.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = position + camRightWS * m_QuadVertexPositions[2].x * size.x + camUpWS * m_QuadVertexPositions[2].y * size.y;
		bufferPtr->Color = tintColor;
		bufferPtr->TexCoord = { 1.0f, 0.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = position + camRightWS * m_QuadVertexPositions[3].x * size.x + camUpWS * m_QuadVertexPositions[3].y * size.y;
		bufferPtr->Color = tintColor;
		bufferPtr->TexCoord = { 1.0f, 1.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		m_QuadIndexCount += 6;

		m_DrawStats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		const float textureIndex = 0.0f; // White Texture
		const float tilingFactor = 1.0f;

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		auto& bufferPtr = GetWriteableQuadBuffer();
		bufferPtr->Position = transform * m_QuadVertexPositions[0];
		bufferPtr->Color = color;
		bufferPtr->TexCoord = { 0.0f, 0.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = transform * m_QuadVertexPositions[1];
		bufferPtr->Color = color;
		bufferPtr->TexCoord = { 1.0f, 0.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = transform * m_QuadVertexPositions[2];
		bufferPtr->Color = color;
		bufferPtr->TexCoord = { 1.0f, 1.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = transform * m_QuadVertexPositions[3];
		bufferPtr->Color = color;
		bufferPtr->TexCoord = { 0.0f, 1.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		m_QuadIndexCount += 6;

		m_DrawStats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < m_TextureSlotIndex; i++)
		{
			if (m_TextureSlots[i]->GetHash() == texture->GetHash())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)m_TextureSlotIndex;
			m_TextureSlots[m_TextureSlotIndex] = texture;
			m_TextureSlotIndex++;
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		auto& bufferPtr = GetWriteableQuadBuffer();
		bufferPtr->Position = transform * m_QuadVertexPositions[0];
		bufferPtr->Color = tintColor;
		bufferPtr->TexCoord = { 0.0f, 0.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = transform * m_QuadVertexPositions[1];
		bufferPtr->Color = tintColor;
		bufferPtr->TexCoord = { 1.0f, 0.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = transform * m_QuadVertexPositions[2];
		bufferPtr->Color = tintColor;
		bufferPtr->TexCoord = { 1.0f, 1.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		bufferPtr->Position = transform * m_QuadVertexPositions[3];
		bufferPtr->Color = tintColor;
		bufferPtr->TexCoord = { 0.0f, 1.0f };
		bufferPtr->TexIndex = textureIndex;
		bufferPtr->TilingFactor = tilingFactor;
		bufferPtr++;

		m_QuadIndexCount += 6;

		m_DrawStats.QuadCount++;
	}

	void Renderer2D::DrawRotatedRect(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedRect({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedRect(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		glm::vec3 positions[4] =
		{
			transform * m_QuadVertexPositions[0],
			transform * m_QuadVertexPositions[1],
			transform * m_QuadVertexPositions[2],
			transform * m_QuadVertexPositions[3]
		};

		for (int i = 0; i < 4; i++)
		{
			auto& v0 = positions[i];
			auto& v1 = positions[(i + 1) % 4];

			auto& bufferPtr = GetWriteableLineBuffer();
			bufferPtr->Position = v0;
			bufferPtr->Color = color;
			bufferPtr++;

			bufferPtr->Position = v1;
			bufferPtr->Color = color;
			bufferPtr++;

			m_LineIndexCount += 2;
			m_DrawStats.LineCount++;
		}
	}

	void Renderer2D::FillCircle(const glm::vec2& position, float radius, const glm::vec4& color, float thickness)
	{
		FillCircle({ position.x, position.y, 0.0f }, radius, color, thickness);
	}

	void Renderer2D::FillCircle(const glm::vec3& position, float radius, const glm::vec4& color, float thickness)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { radius * 2.0f, radius * 2.0f, 1.0f });

		auto& bufferPtr = GetWriteableCircleBuffer();
		for (int i = 0; i < 4; i++)
		{
			bufferPtr->WorldPosition = transform * m_QuadVertexPositions[i];
			bufferPtr->Thickness = thickness;
			bufferPtr->LocalPosition = m_QuadVertexPositions[i] * 2.0f;
			bufferPtr->Color = color;
			bufferPtr++;

			m_CircleIndexCount += 6;
			m_DrawStats.QuadCount++;
		}
	}

	void Renderer2D::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color)
	{
		auto& bufferPtr = GetWriteableLineBuffer();
		bufferPtr->Position = p0;
		bufferPtr->Color = color;
		bufferPtr++;

		bufferPtr->Position = p1;
		bufferPtr->Color = color;
		bufferPtr++;

		m_LineIndexCount += 2;

		m_DrawStats.LineCount++;
	}

	void Renderer2D::DrawCircle(const glm::vec3& position, const glm::vec3& rotation, float radius, const glm::vec4& color)
	{
		const glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation.x, { 1.0f, 0.0f, 0.0f })
			* glm::rotate(glm::mat4(1.0f), rotation.y, { 0.0f, 1.0f, 0.0f })
			* glm::rotate(glm::mat4(1.0f), rotation.z, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), glm::vec3(radius));

		DrawCircle(transform, color);
	}

	void Renderer2D::DrawCircle(const glm::mat4& transform, const glm::vec4& color)
	{
		int segments = 32;
		for (int i = 0; i < segments; i++)
		{
			float angle = 2.0f * glm::pi<float>() * (float)i / segments;
			glm::vec4 startPosition = { glm::cos(angle), glm::sin(angle), 0.0f, 1.0f };
			angle = 2.0f * glm::pi<float>() * (float)((i + 1) % segments) / segments;
			glm::vec4 endPosition = { glm::cos(angle), glm::sin(angle), 0.0f, 1.0f };

			glm::vec3 p0 = transform * startPosition;
			glm::vec3 p1 = transform * endPosition;
			DrawLine(p0, p1, color);
		}
	}

	void Renderer2D::DrawAABB(Ref<Mesh> mesh, const glm::mat4& transform, const glm::vec4& color)
	{
		const auto& meshAssetSubmeshes = mesh->GetMeshSource()->GetSubmeshes();
		auto& submeshes = mesh->GetSubmeshes();
		for (uint32_t submeshIndex : submeshes)
		{
			const Submesh& submesh = meshAssetSubmeshes[submeshIndex];
			auto& aabb = submesh.BoundingBox;
			auto aabbTransform = transform * submesh.Transform;
			DrawAABB(aabb, aabbTransform);
		}
	}

	void Renderer2D::DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color /*= glm::vec4(1.0f)*/)
	{
		glm::vec4 min = { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f };
		glm::vec4 max = { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f };

		glm::vec4 corners[8] =
		{
			transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Max.z, 1.0f },

			transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Min.z, 1.0f }
		};

		for (uint32_t i = 0; i < 4; i++)
			DrawLine(corners[i], corners[(i + 1) % 4], color);

		for (uint32_t i = 0; i < 4; i++)
			DrawLine(corners[i + 4], corners[((i + 1) % 4) + 4], color);

		for (uint32_t i = 0; i < 4; i++)
			DrawLine(corners[i], corners[i + 4], color);
	}

	static bool NextLine(int index, const std::vector<int>& lines)
	{
		for (int line : lines)
		{
			if (line == index)
				return true;
		}
		return false;
	}

	void Renderer2D::DrawString(const std::string& string, const glm::vec3& position, float maxWidth, const glm::vec4& color)
	{
		// Use default font
		DrawString(string, Font::GetDefaultFont(), position, maxWidth, color);
	}

	void Renderer2D::DrawString(const std::string& string, const Ref<Font>& font, const glm::vec3& position, float maxWidth, const glm::vec4& color)
	{
		DrawString(string, font, glm::translate(glm::mat4(1.0f), position), maxWidth, color);
	}

// warning C4996: 'std::codecvt_utf8<char32_t,1114111,(std::codecvt_mode)0>': warning STL4017: std::wbuffer_convert, std::wstring_convert, and the <codecvt> header
// (containing std::codecvt_mode, std::codecvt_utf8, std::codecvt_utf16, and std::codecvt_utf8_utf16) are deprecated in C++17. (The std::codecvt class template is NOT deprecated.)
// The C++ Standard doesn't provide equivalent non-deprecated functionality; consider using MultiByteToWideChar() and WideCharToMultiByte() from <Windows.h> instead.
// You can define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.
#pragma warning(disable : 4996)

	// From https://stackoverflow.com/questions/31302506/stdu32string-conversion-to-from-stdstring-and-stdu16string
	static std::u32string To_UTF32(const std::string& s)
	{
		std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
		return conv.from_bytes(s);
	}

#pragma warning(default : 4996)

	void Renderer2D::DrawString(const std::string& string, const Ref<Font>& font, const glm::mat4& transform, float maxWidth, const glm::vec4& color, float lineHeightOffset, float kerningOffset)
	{
		if (string.empty())
			return;

		float textureIndex = -1.0f;

		// TODO(Yan): this isn't really ideal, but we need to iterate through UTF-8 code points
		std::u32string utf32string = To_UTF32(string);

		Ref<Texture2D> fontAtlas = font->GetFontAtlas();
		HZ_CORE_ASSERT(fontAtlas);

		for (uint32_t i = 0; i < m_FontTextureSlotIndex; i++)
		{
			if (*m_FontTextureSlots[i].Raw() == *fontAtlas.Raw())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == -1.0f)
		{
			textureIndex = (float)m_FontTextureSlotIndex;
			m_FontTextureSlots[m_FontTextureSlotIndex] = fontAtlas;
			m_FontTextureSlotIndex++;
		}

		auto& fontGeometry = font->GetMSDFData()->FontGeometry;
		const auto& metrics = fontGeometry.getMetrics();

		// TODO(Yan): these font metrics really should be cleaned up/refactored...
		//            (this is a first pass WIP)
		std::vector<int> nextLines;
		{
			double x = 0.0;
			double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
			double y = -fsScale * metrics.ascenderY;
			int lastSpace = -1;
			for (int i = 0; i < utf32string.size(); i++)
			{
				char32_t character = utf32string[i];
				if (character == '\n')
				{
					x = 0;
					y -= fsScale * metrics.lineHeight + lineHeightOffset;
					continue;
				}
				if (character == '\r')
					continue;

				auto glyph = fontGeometry.getGlyph(character);
				//if (!glyph)
				//	glyph = fontGeometry.getGlyph('?');
				if (!glyph)
					continue;

				if (character != ' ')
				{
					// Calc geo
					double pl, pb, pr, pt;
					glyph->getQuadPlaneBounds(pl, pb, pr, pt);
					glm::vec2 quadMin((float)pl, (float)pb);
					glm::vec2 quadMax((float)pr, (float)pt);

					quadMin *= fsScale;
					quadMax *= fsScale;
					quadMin += glm::vec2(x, y);
					quadMax += glm::vec2(x, y);

					if (quadMax.x > maxWidth && lastSpace != -1)
					{
						i = lastSpace;
						nextLines.emplace_back(lastSpace);
						lastSpace = -1;
						x = 0;
						y -= fsScale * metrics.lineHeight + lineHeightOffset;
					}
				}
				else
				{
					lastSpace = i;
				}

				double advance = glyph->getAdvance();
				fontGeometry.getAdvance(advance, character, utf32string[i + 1]);
				x += fsScale * advance + kerningOffset;
			}
		}

		{
			double x = 0.0;
			double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
			double y = 0.0;// -fsScale * metrics.ascenderY;
			for (int i = 0; i < utf32string.size(); i++)
			{
				char32_t character = utf32string[i];
				if (character == '\n' || NextLine(i, nextLines))
				{
					x = 0;
					y -= fsScale * metrics.lineHeight + lineHeightOffset;
					continue;
				}

				auto glyph = fontGeometry.getGlyph(character);
				//if (!glyph)
				//	glyph = fontGeometry.getGlyph('?');
				if (!glyph)
					continue;

				double l, b, r, t;
				glyph->getQuadAtlasBounds(l, b, r, t);

				double pl, pb, pr, pt;
				glyph->getQuadPlaneBounds(pl, pb, pr, pt);

				pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
				pl += x, pb += y, pr += x, pt += y;

				double texelWidth = 1. / fontAtlas->GetWidth();
				double texelHeight = 1. / fontAtlas->GetHeight();
				l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;

				// ImGui::Begin("Font");
				// ImGui::Text("Size: %d, %d", m_ExampleFontSheet->GetWidth(), m_ExampleFontSheet->GetHeight());
				// UI::Image(m_ExampleFontSheet, ImVec2(m_ExampleFontSheet->GetWidth(), m_ExampleFontSheet->GetHeight()), ImVec2(0, 1), ImVec2(1, 0));
				// ImGui::End();

				auto& bufferPtr = GetWriteableTextBuffer();
				bufferPtr->Position = transform * glm::vec4(pl, pb, 0.0f, 1.0f);
				bufferPtr->Color = color;
				bufferPtr->TexCoord = { l, b };
				bufferPtr->TexIndex = textureIndex;
				bufferPtr++;

				bufferPtr->Position = transform * glm::vec4(pl, pt, 0.0f, 1.0f);
				bufferPtr->Color = color;
				bufferPtr->TexCoord = { l, t };
				bufferPtr->TexIndex = textureIndex;
				bufferPtr++;

				bufferPtr->Position = transform * glm::vec4(pr, pt, 0.0f, 1.0f);
				bufferPtr->Color = color;
				bufferPtr->TexCoord = { r, t };
				bufferPtr->TexIndex = textureIndex;
				bufferPtr++;

				bufferPtr->Position = transform * glm::vec4(pr, pb, 0.0f, 1.0f);
				bufferPtr->Color = color;
				bufferPtr->TexCoord = { r, b };
				bufferPtr->TexIndex = textureIndex;
				bufferPtr++;

				m_TextIndexCount += 6;

				double advance = glyph->getAdvance();
				fontGeometry.getAdvance(advance, character, utf32string[i + 1]);
				x += fsScale * advance + kerningOffset;

				m_DrawStats.QuadCount++;
			}
		}

	}

	float Renderer2D::GetLineWidth()
	{
		return m_LineWidth;
	}

	void Renderer2D::SetLineWidth(float lineWidth)
	{
		m_LineWidth = lineWidth;
	}

	void Renderer2D::ResetStats()
	{
		memset(&m_DrawStats, 0, sizeof(DrawStatistics));
		m_MemoryStats.Used = 0;
	}

	Renderer2D::DrawStatistics Renderer2D::GetDrawStats()
	{
		return m_DrawStats;
	}

	Renderer2D::MemoryStatistics Renderer2D::GetMemoryStats()
	{
		return m_MemoryStats;
	}

	uint64_t Renderer2D::MemoryStatistics::GetAllocatedPerFrame() const
	{
		return TotalAllocated / Renderer::GetConfig().FramesInFlight;
	}

}
