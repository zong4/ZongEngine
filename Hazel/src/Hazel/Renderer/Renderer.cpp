#include "hzpch.h"
#include "Renderer.h"

#include "Shader.h"

#include <map>

#include "RendererAPI.h"
#include "SceneRenderer.h"
#include "Renderer2D.h"
#include "ShaderPack.h"

#include "Hazel/Core/Timer.h"
#include "Hazel/Debug/Profiler.h"

#include "Hazel/Platform/Vulkan/VulkanComputePipeline.h"
#include "Hazel/Platform/Vulkan/VulkanRenderer.h"

#include "Hazel/Platform/Vulkan/VulkanContext.h"

#include "Hazel/Project/Project.h"

#include <filesystem>

namespace std {
	template<>
	struct hash<Hazel::WeakRef<Hazel::Shader>>
	{
		size_t operator()(const Hazel::WeakRef<Hazel::Shader>& shader) const noexcept
		{
			return shader->GetHash();
		}
	};
}

namespace Hazel {

	static std::unordered_map<size_t, Ref<Pipeline>> s_PipelineCache;

	static RendererAPI* s_RendererAPI = nullptr;

	struct ShaderDependencies
	{
		std::vector<Ref<PipelineCompute>> ComputePipelines;
		std::vector<Ref<Pipeline>> Pipelines;
		std::vector<Ref<Material>> Materials;
	};
	static std::unordered_map<size_t, ShaderDependencies> s_ShaderDependencies;

	struct GlobalShaderInfo
	{
		// Macro name, set of shaders with that macro.
		std::unordered_map<std::string, std::unordered_map<size_t, WeakRef<Shader>>> ShaderGlobalMacrosMap;
		// Shaders waiting to be reloaded.
		std::unordered_set<WeakRef<Shader>> DirtyShaders;
	};
	static GlobalShaderInfo s_GlobalShaderInfo;

	void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<PipelineCompute> computePipeline)
	{
		s_ShaderDependencies[shader->GetHash()].ComputePipelines.push_back(computePipeline);
	}

	void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Pipeline> pipeline)
	{
		s_ShaderDependencies[shader->GetHash()].Pipelines.push_back(pipeline);
	}

	void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Material> material)
	{
		s_ShaderDependencies[shader->GetHash()].Materials.push_back(material);
	}

	void Renderer::OnShaderReloaded(size_t hash)
	{
		if (s_ShaderDependencies.find(hash) != s_ShaderDependencies.end())
		{
			auto& dependencies = s_ShaderDependencies.at(hash);
			for (auto& pipeline : dependencies.Pipelines)
			{
				pipeline->Invalidate();
			}

			for (auto& computePipeline : dependencies.ComputePipelines)
			{
				computePipeline.As<VulkanComputePipeline>()->CreatePipeline();
			}

			for (auto& material : dependencies.Materials)
			{
				material->OnShaderReloaded();
			}
		}
	}

	uint32_t Renderer::RT_GetCurrentFrameIndex()
	{
		// Swapchain owns the Render Thread frame index
		return Application::Get().GetWindow().GetSwapChain().GetCurrentBufferIndex();
	}

	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return Application::Get().GetCurrentFrameIndex();
	}

	void RendererAPI::SetAPI(RendererAPIType api)
	{
		// TODO: make sure this is called at a valid time
		HZ_CORE_VERIFY(api == RendererAPIType::Vulkan, "Vulkan is currently the only supported Renderer API");
		s_CurrentRendererAPI = api;
	}

	struct RendererData
	{
		Ref<ShaderLibrary> m_ShaderLibrary;

		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BlackTexture;
		Ref<Texture2D> BRDFLutTexture;
		Ref<Texture2D> HilbertLut;
		Ref<TextureCube> BlackCubeTexture;
		Ref<Environment> EmptyEnvironment;

		std::unordered_map<std::string, std::string> GlobalShaderMacros;
	};

	static RendererConfig s_Config;
	static RendererData* s_Data = nullptr;
	constexpr static uint32_t s_RenderCommandQueueCount = 2;
	static RenderCommandQueue* s_CommandQueue[s_RenderCommandQueueCount];
	static std::atomic<uint32_t> s_RenderCommandQueueSubmissionIndex = 0;
	static RenderCommandQueue s_ResourceFreeQueue[3];

	static RendererAPI* InitRendererAPI()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::Vulkan: return hnew VulkanRenderer();
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void Renderer::Init()
	{
		s_Data = hnew RendererData();
		s_CommandQueue[0] = hnew RenderCommandQueue();
		s_CommandQueue[1] = hnew RenderCommandQueue();

		// Make sure we don't have more frames in flight than swapchain images
		s_Config.FramesInFlight = glm::min<uint32_t>(s_Config.FramesInFlight, Application::Get().GetWindow().GetSwapChain().GetImageCount());

		s_RendererAPI = InitRendererAPI();

		Renderer::SetGlobalMacroInShaders("__HZ_REFLECTION_OCCLUSION_METHOD", "0");
		Renderer::SetGlobalMacroInShaders("__HZ_AO_METHOD", fmt::format("{}", ShaderDef::GetAOMethod(true)));
		Renderer::SetGlobalMacroInShaders("__HZ_GTAO_COMPUTE_BENT_NORMALS", "0");

		s_Data->m_ShaderLibrary = Ref<ShaderLibrary>::Create();

		if (!s_Config.ShaderPackPath.empty())
			Renderer::GetShaderLibrary()->LoadShaderPack(s_Config.ShaderPackPath);


		// NOTE: some shaders (compute) need to have optimization disabled because of a shaderc internal error
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/HZB.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/HazelPBR_Static.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/HazelPBR_Transparent.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/HazelPBR_Anim.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Grid.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Wireframe.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Wireframe_Anim.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Skybox.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/DirShadowMap.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/DirShadowMap_Anim.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/SpotShadowMap.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/SpotShadowMap_Anim.glsl");

		//SSR
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Pre-Integration.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PostProcessing/Pre-Convolution.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PostProcessing/SSR.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PostProcessing/SSR-Composite.glsl");

		// Environment compute shaders
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentMipFilter.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/EquirectangularToCubeMap.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentIrradiance.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PreethamSky.glsl");

		// Post-processing
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PostProcessing/Bloom.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PostProcessing/DOF.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PostProcessing/EdgeDetection.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PostProcessing/SceneComposite.glsl");

		// Light-culling
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PreDepth.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PreDepth_Anim.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/LightCulling.glsl");

		// Renderer2D Shaders
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Line.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Circle.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Text.glsl");

		// Jump Flood Shaders
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood_Init.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood_Pass.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood_Composite.glsl");

		// GTAO
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PostProcessing/GTAO.hlsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PostProcessing/GTAO-Denoise.glsl");

		// AO
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PostProcessing/AO-Composite.glsl");

		// Misc
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/SelectedGeometry.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/SelectedGeometry_Anim.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/TexturePass.glsl");

		// Compile shaders
		Application::Get().GetRenderThread().Pump();

		uint32_t whiteTextureData = 0xffffffff;
		TextureSpecification spec;
		spec.Format = ImageFormat::RGBA;
		spec.Width = 1;
		spec.Height = 1;
		s_Data->WhiteTexture = Texture2D::Create(spec, Buffer(&whiteTextureData, sizeof(uint32_t)));

		constexpr uint32_t blackTextureData = 0xff000000;
		s_Data->BlackTexture = Texture2D::Create(spec, Buffer(&blackTextureData, sizeof(uint32_t)));

		{
			TextureSpecification spec;
			spec.SamplerWrap = TextureWrap::Clamp;
			s_Data->BRDFLutTexture = Texture2D::Create(spec, Buffer("Resources/Renderer/BRDF_LUT.tga"));
		}

		constexpr uint32_t blackCubeTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
		s_Data->BlackCubeTexture = TextureCube::Create(spec, Buffer(&blackTextureData, sizeof(blackCubeTextureData)));

		s_Data->EmptyEnvironment = Ref<Environment>::Create(s_Data->BlackCubeTexture, s_Data->BlackCubeTexture);

		// Hilbert look-up texture! It's a 64 x 64 uint16 texture
		{
			TextureSpecification spec;
			spec.Format = ImageFormat::RED16UI;
			spec.Width = 64;
			spec.Height = 64;
			spec.SamplerWrap = TextureWrap::Clamp;
			spec.SamplerFilter = TextureFilter::Nearest;

			constexpr auto HilbertIndex = [](uint32_t posX, uint32_t posY)
			{
				uint16_t index = 0u;
				for (uint16_t curLevel = 64 / 2u; curLevel > 0u; curLevel /= 2u)
				{
					const uint16_t regionX = (posX & curLevel) > 0u;
					const uint16_t regionY = (posY & curLevel) > 0u;
					index += curLevel * curLevel * ((3u * regionX) ^ regionY);
					if (regionY == 0u)
					{
						if (regionX == 1u)
						{
							posX = uint16_t((64 - 1u)) - posX;
							posY = uint16_t((64 - 1u)) - posY;
						}

						std::swap(posX, posY);
					}
				}
				return index;
			};

			uint16_t* data = new uint16_t[(size_t)(64 * 64)];
			for (int x = 0; x < 64; x++)
			{
				for (int y = 0; y < 64; y++)
				{
					const uint16_t r2index = HilbertIndex(x, y);
					HZ_CORE_ASSERT(r2index < 65536);
					data[x + 64 * y] = r2index;
				}
			}
			s_Data->HilbertLut = Texture2D::Create(spec, Buffer(data, 1));
			delete[] data;

		}

		s_RendererAPI->Init();
	}

	void Renderer::Shutdown()
	{
		s_ShaderDependencies.clear();
		s_RendererAPI->Shutdown();

		delete s_Data;

		// Resource release queue
		for (uint32_t i = 0; i < s_Config.FramesInFlight; i++)
		{
			auto& queue = Renderer::GetRenderResourceReleaseQueue(i);
			queue.Execute();
		}

		delete s_CommandQueue[0];
		delete s_CommandQueue[1];
	}

	RendererCapabilities& Renderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	Ref<ShaderLibrary> Renderer::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}

	void Renderer::RenderThreadFunc(RenderThread* renderThread)
	{
		HZ_PROFILE_THREAD("Render Thread");

		while (renderThread->IsRunning())
		{
			WaitAndRender(renderThread);
		}
	}

	void Renderer::WaitAndRender(RenderThread* renderThread)
	{
		HZ_PROFILE_FUNC();
		auto& performanceTimers = Application::Get().m_PerformanceTimers;

		// Wait for kick, then set render thread to busy
		{
			HZ_PROFILE_SCOPE("Wait");
			Timer waitTimer;
			renderThread->WaitAndSet(RenderThread::State::Kick, RenderThread::State::Busy);
			performanceTimers.RenderThreadWaitTime = waitTimer.ElapsedMillis();
		}

		Timer workTimer;
		s_CommandQueue[GetRenderQueueIndex()]->Execute();
		// ExecuteRenderCommandQueue();

		// Rendering has completed, set state to idle
		renderThread->Set(RenderThread::State::Idle);

		performanceTimers.RenderThreadWorkTime = workTimer.ElapsedMillis();
	}

	void Renderer::SwapQueues()
	{
		s_RenderCommandQueueSubmissionIndex = (s_RenderCommandQueueSubmissionIndex + 1) % s_RenderCommandQueueCount;
	}

	uint32_t Renderer::GetRenderQueueIndex()
	{
		return (s_RenderCommandQueueSubmissionIndex + 1) % s_RenderCommandQueueCount;
	}

	uint32_t Renderer::GetRenderQueueSubmissionIndex()
	{
		return s_RenderCommandQueueSubmissionIndex;
	}

	void Renderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear)
	{
		HZ_CORE_ASSERT(renderPass, "RenderPass cannot be null!");

		s_RendererAPI->BeginRenderPass(renderCommandBuffer, renderPass, explicitClear);
	}

	void Renderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		s_RendererAPI->EndRenderPass(renderCommandBuffer);
	}

	void Renderer::BeginComputePass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass)
	{
		HZ_CORE_ASSERT(computePass, "ComputePass cannot be null!");

		s_RendererAPI->BeginComputePass(renderCommandBuffer, computePass);
	}

	void Renderer::EndComputePass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass)
	{
		s_RendererAPI->EndComputePass(renderCommandBuffer, computePass);
	}

	void Renderer::DispatchCompute(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass, Ref<Material> material, const glm::uvec3& workGroups, Buffer constants)
	{
		s_RendererAPI->DispatchCompute(renderCommandBuffer, computePass, material, workGroups, constants);
	}

	void Renderer::InsertGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& color)
	{
		s_RendererAPI->InsertGPUPerfMarker(renderCommandBuffer, label, color);
	}

	void Renderer::BeginGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& markerColor)
	{
		s_RendererAPI->BeginGPUPerfMarker(renderCommandBuffer, label, markerColor);
	}

	void Renderer::EndGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		s_RendererAPI->EndGPUPerfMarker(renderCommandBuffer);
	}

	void Renderer::RT_InsertGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& color)
	{
		s_RendererAPI->RT_InsertGPUPerfMarker(renderCommandBuffer, label, color);
	}

	void Renderer::RT_BeginGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& markerColor)
	{
		s_RendererAPI->RT_BeginGPUPerfMarker(renderCommandBuffer, label, markerColor);
	}

	void Renderer::RT_EndGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		s_RendererAPI->RT_EndGPUPerfMarker(renderCommandBuffer);
	}

	void Renderer::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}

	void Renderer::EndFrame()
	{
		s_RendererAPI->EndFrame();
	}

	void Renderer::SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<Image2D> shadow, Ref<Image2D> spotShadow)
	{
		s_RendererAPI->SetSceneEnvironment(sceneRenderer, environment, shadow, spotShadow);
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::CreateEnvironmentMap(const std::string& filepath)
	{
		return s_RendererAPI->CreateEnvironmentMap(filepath);
	}

	void Renderer::LightCulling(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass, Ref<Material> material, const glm::uvec3& workGroups)
	{
		s_RendererAPI->LightCulling(renderCommandBuffer, computePass, material, workGroups);
	}

	Ref<TextureCube> Renderer::CreatePreethamSky(float turbidity, float azimuth, float inclination)
	{
		return s_RendererAPI->CreatePreethamSky(turbidity, azimuth, inclination);
	}

	void Renderer::RenderStaticMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<StaticMesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount)
	{
		s_RendererAPI->RenderStaticMesh(renderCommandBuffer, pipeline, mesh, submeshIndex, materialTable, transformBuffer, transformOffset, instanceCount);
	}

#if 0
	void Renderer::RenderSubmesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable, const glm::mat4& transform)
	{
		s_RendererAPI->RenderSubmesh(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, submeshIndex, materialTable, transform);
	}
#endif

	void Renderer::RenderSubmeshInstanced(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t boneTransformsOffset, uint32_t instanceCount)
	{
		s_RendererAPI->RenderSubmeshInstanced(renderCommandBuffer, pipeline, mesh, submeshIndex, materialTable, transformBuffer, transformOffset, boneTransformsOffset, instanceCount);
	}

	void Renderer::RenderMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t boneTransformsOffset, uint32_t instanceCount, Ref<Material> material, Buffer additionalUniforms)
	{
		s_RendererAPI->RenderMeshWithMaterial(renderCommandBuffer, pipeline, mesh, submeshIndex, material, transformBuffer, transformOffset, boneTransformsOffset, instanceCount, additionalUniforms);
	}

	void Renderer::RenderStaticMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<StaticMesh> mesh, uint32_t submeshIndex, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount, Ref<Material> material, Buffer additionalUniforms)
	{
		s_RendererAPI->RenderStaticMeshWithMaterial(renderCommandBuffer, pipeline, mesh, submeshIndex, material, transformBuffer, transformOffset, instanceCount, additionalUniforms);
	}

	void Renderer::RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, const glm::mat4& transform)
	{
		s_RendererAPI->RenderQuad(renderCommandBuffer, pipeline, material, transform);
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount /*= 0*/)
	{
		s_RendererAPI->RenderGeometry(renderCommandBuffer, pipeline, material, vertexBuffer, indexBuffer, transform, indexCount);
	}

	void Renderer::SubmitQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, const glm::mat4& transform)
	{
		HZ_CORE_ASSERT(false, "Not Implemented");
		/*bool depthTest = true;
		if (material)
		{
				material->Bind();
				depthTest = material->GetFlag(MaterialFlag::DepthTest);
				cullFace = !material->GetFlag(MaterialFlag::TwoSided);

				auto shader = material->GetShader();
				shader->SetUniformBuffer("Transform", &transform, sizeof(glm::mat4));
		}

		s_Data->m_FullscreenQuadVertexBuffer->Bind();
		s_Data->m_FullscreenQuadPipeline->Bind();
		s_Data->m_FullscreenQuadIndexBuffer->Bind();
		Renderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);*/
	}

	void Renderer::ClearImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> image, const ImageClearValue& clearValue, ImageSubresourceRange subresourceRange)
	{
		s_RendererAPI->ClearImage(renderCommandBuffer, image, clearValue, subresourceRange);
	}

	void Renderer::CopyImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage)
	{
		s_RendererAPI->CopyImage(renderCommandBuffer, sourceImage, destinationImage);
	}

	void Renderer::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material)
	{
		s_RendererAPI->SubmitFullscreenQuad(renderCommandBuffer, pipeline, material);
	}

	void Renderer::SubmitFullscreenQuadWithOverrides(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides)
	{
		s_RendererAPI->SubmitFullscreenQuadWithOverrides(renderCommandBuffer, pipeline, material, vertexShaderOverrides, fragmentShaderOverrides);
	}

#if 0
	void Renderer::SubmitFullscreenQuad(Ref<Material> material)
	{
		// Retrieve pipeline from cache
		auto& shader = material->GetShader();
		auto hash = shader->GetHash();
		if (s_PipelineCache.find(hash) == s_PipelineCache.end())
		{
			// Create pipeline
			PipelineSpecification spec = s_Data->m_FullscreenQuadPipelineSpec;
			spec.Shader = shader;
			spec.DebugName = "Renderer-FullscreenQuad-" + shader->GetName();
			s_PipelineCache[hash] = Pipeline::Create(spec);
		}

		auto& pipeline = s_PipelineCache[hash];

		bool depthTest = true;
		bool cullFace = true;
		if (material)
		{
			// material->Bind();
			depthTest = material->GetFlag(MaterialFlag::DepthTest);
			cullFace = !material->GetFlag(MaterialFlag::TwoSided);
		}

		s_Data->FullscreenQuadVertexBuffer->Bind();
		pipeline->Bind();
		s_Data->FullscreenQuadIndexBuffer->Bind();
		Renderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);
}
#endif

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_Data->WhiteTexture;
	}

	Ref<Texture2D> Renderer::GetBlackTexture()
	{
		return s_Data->BlackTexture;
	}

	Ref<Texture2D> Renderer::GetHilbertLut()
	{
		return s_Data->HilbertLut;
	}

	Ref<Texture2D> Renderer::GetBRDFLutTexture()
	{
		return s_Data->BRDFLutTexture;
	}

	Ref<TextureCube> Renderer::GetBlackCubeTexture()
	{
		return s_Data->BlackCubeTexture;
	}


	Ref<Environment> Renderer::GetEmptyEnvironment()
	{
		return s_Data->EmptyEnvironment;
	}

	RenderCommandQueue& Renderer::GetRenderCommandQueue()
	{
		return *s_CommandQueue[s_RenderCommandQueueSubmissionIndex];
	}

	RenderCommandQueue& Renderer::GetRenderResourceReleaseQueue(uint32_t index)
	{
		return s_ResourceFreeQueue[index];
	}


	const std::unordered_map<std::string, std::string>& Renderer::GetGlobalShaderMacros()
	{
		return s_Data->GlobalShaderMacros;
	}

	RendererConfig& Renderer::GetConfig()
	{
		return s_Config;
	}

	void Renderer::SetConfig(const RendererConfig& config)
	{
		s_Config = config;
	}

	void Renderer::AcknowledgeParsedGlobalMacros(const std::unordered_set<std::string>& macros, Ref<Shader> shader)
	{
		for (const std::string& macro : macros)
		{
			s_GlobalShaderInfo.ShaderGlobalMacrosMap[macro][shader->GetHash()] = shader;
		}
	}

	void Renderer::SetMacroInShader(Ref<Shader> shader, const std::string& name, const std::string& value)
	{
		shader->SetMacro(name, value);
		s_GlobalShaderInfo.DirtyShaders.emplace(shader.Raw());
	}

	void Renderer::SetGlobalMacroInShaders(const std::string& name, const std::string& value)
	{
		if (s_Data->GlobalShaderMacros.find(name) != s_Data->GlobalShaderMacros.end())
		{
			if (s_Data->GlobalShaderMacros.at(name) == value)
				return;
		}

		s_Data->GlobalShaderMacros[name] = value;

		if (s_GlobalShaderInfo.ShaderGlobalMacrosMap.find(name) == s_GlobalShaderInfo.ShaderGlobalMacrosMap.end())
		{
			HZ_CORE_WARN("No shaders with {} macro found", name);
			return;
		}

		HZ_CORE_ASSERT(s_GlobalShaderInfo.ShaderGlobalMacrosMap.find(name) != s_GlobalShaderInfo.ShaderGlobalMacrosMap.end(), "Macro has not been passed from any shader!");
		for (auto& [hash, shader] : s_GlobalShaderInfo.ShaderGlobalMacrosMap.at(name))
		{
			HZ_CORE_ASSERT(shader.IsValid(), "Shader is deleted!");
			s_GlobalShaderInfo.DirtyShaders.emplace(shader);
		}
	}

	bool Renderer::UpdateDirtyShaders()
	{
		// TODO(Yan): how is this going to work for dist?
		const bool updatedAnyShaders = s_GlobalShaderInfo.DirtyShaders.size();
		for (WeakRef<Shader> shader : s_GlobalShaderInfo.DirtyShaders)
		{
			HZ_CORE_ASSERT(shader.IsValid(), "Shader is deleted!");
			shader->RT_Reload(true);
		}
		s_GlobalShaderInfo.DirtyShaders.clear();

		return updatedAnyShaders;
	}

	GPUMemoryStats Renderer::GetGPUMemoryStats()
	{
		return VulkanAllocator::GetStats();
	}

}
