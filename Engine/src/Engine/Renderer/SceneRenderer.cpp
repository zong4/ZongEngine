#include "pch.h"
#include "SceneRenderer.h"

#include "Renderer.h"
#include "SceneEnvironment.h"

#include <imgui.h>
#include <imgui/imgui_internal.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/compatibility.hpp>

#include "Renderer2D.h"
#include "UniformBuffer.h"
#include "Engine/Core/Math/Noise.h"

#include "Engine/Utilities/FileSystem.h"

#include "Engine/ImGui/ImGui.h"
#include "Engine/Debug/Profiler.h"
#include "Engine/Math/Math.h"

namespace Engine {

	static std::vector<std::thread> s_ThreadPool;

	SceneRenderer::SceneRenderer(Ref<Scene> scene, SceneRendererSpecification specification)
		: m_Scene(scene), m_Specification(specification)
	{
		Init();
	}

	SceneRenderer::~SceneRenderer()
	{
		Shutdown();
	}

	void SceneRenderer::Init()
	{
		ZONG_SCOPE_TIMER("SceneRenderer::Init");

		m_ShadowCascadeSplits[0] = 0.1f;
		m_ShadowCascadeSplits[1] = 0.2f;
		m_ShadowCascadeSplits[2] = 0.3f;
		m_ShadowCascadeSplits[3] = 1.0f;

		// Tiering
		{
			using namespace Tiering::Renderer;

			const auto& tiering = m_Specification.Tiering;

			RendererDataUB.SoftShadows = tiering.ShadowQuality == ShadowQualitySetting::High;

			m_Options.EnableGTAO = false;

			switch (tiering.AOQuality)
			{
				case Tiering::Renderer::AmbientOcclusionQualitySetting::High:
					m_Options.EnableGTAO = true;
					GTAODataCB.HalfRes = true;
					break;
				case Tiering::Renderer::AmbientOcclusionQualitySetting::Ultra:
					m_Options.EnableGTAO = true;
					GTAODataCB.HalfRes = false;
					break;
			}

			if (tiering.EnableAO)
			{
				switch (tiering.AOType)
				{
					case Tiering::Renderer::AmbientOcclusionTypeSetting::GTAO:
						m_Options.EnableGTAO = true;
						break;
				}
			}

			switch (tiering.SSRQuality)
			{
				case Tiering::Renderer::SSRQualitySetting::Medium:
					m_Options.EnableSSR = true;
					m_SSROptions.HalfRes = true;
					break;
				case Tiering::Renderer::SSRQualitySetting::High:
					m_Options.EnableSSR = true;
					m_SSROptions.HalfRes = false;
					break;
			}

		}

		m_CommandBuffer = RenderCommandBuffer::Create(0, "SceneRenderer");

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		m_UBSCamera = UniformBufferSet::Create(sizeof(UBCamera));
		m_UBSShadow = UniformBufferSet::Create(sizeof(UBShadow));
		m_UBSScene = UniformBufferSet::Create(sizeof(UBScene));
		m_UBSRendererData = UniformBufferSet::Create(sizeof(UBRendererData));
		m_UBSPointLights = UniformBufferSet::Create(sizeof(UBPointLights));
		m_UBSScreenData = UniformBufferSet::Create(sizeof(UBScreenData));
		m_UBSSpotLights = UniformBufferSet::Create(sizeof(UBSpotLights));
		m_UBSSpotShadowData = UniformBufferSet::Create(sizeof(UBSpotShadowData));

		{
			StorageBufferSpecification spec;
			spec.DebugName = "BoneTransforms";
			spec.GPUOnly = false;
			const size_t BoneTransformBufferCount = 1 * 1024; // basically means limited to 1024 animated meshes   TODO(0x): resizeable/flushable
			m_SBSBoneTransforms = StorageBufferSet::Create(spec, sizeof(BoneTransforms) * BoneTransformBufferCount);
			m_BoneTransformsData = hnew BoneTransforms[BoneTransformBufferCount];
		}
		// Create passes and specify "flow" -> this can (and should) include outputs and chain together,
		// for eg. shadow pass output goes into geo pass input

		m_Renderer2D = Ref<Renderer2D>::Create();
		m_Renderer2DScreenSpace = Ref<Renderer2D>::Create();
		m_DebugRenderer = Ref<DebugRenderer>::Create();

		m_CompositeShader = Renderer::GetShaderLibrary()->Get("SceneComposite");
		m_CompositeMaterial = Material::Create(m_CompositeShader);

		// Descriptor Set Layout
		// 

		// Light culling compute pipeline
		{
			StorageBufferSpecification spec;
			spec.DebugName = "VisiblePointLightIndices";
			m_SBSVisiblePointLightIndicesBuffer = StorageBufferSet::Create(spec, 1); // Resized later

			spec.DebugName = "VisibleSpotLightIndices";
			m_SBSVisibleSpotLightIndicesBuffer = StorageBufferSet::Create(spec, 1); // Resized later

			m_LightCullingMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("LightCulling"), "LightCulling");
			Ref<Shader> lightCullingShader = Renderer::GetShaderLibrary()->Get("LightCulling");
			m_LightCullingPipeline = PipelineCompute::Create(lightCullingShader);
		}

		VertexBufferLayout vertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float3, "a_Tangent" },
			{ ShaderDataType::Float3, "a_Binormal" },
			{ ShaderDataType::Float2, "a_TexCoord" }
		};

		VertexBufferLayout instanceLayout = {
			{ ShaderDataType::Float4, "a_MRow0" },
			{ ShaderDataType::Float4, "a_MRow1" },
			{ ShaderDataType::Float4, "a_MRow2" },
		};

		VertexBufferLayout boneInfluenceLayout = {
			{ ShaderDataType::Int4,   "a_BoneIDs" },
			{ ShaderDataType::Float4, "a_BoneWeights" },
		};

		uint32_t shadowMapResolution = 4096;
		switch (m_Specification.Tiering.ShadowResolution)
		{
			case Tiering::Renderer::ShadowResolutionSetting::Low:
				shadowMapResolution = 1024;
				break;
			case Tiering::Renderer::ShadowResolutionSetting::Medium:
				shadowMapResolution = 2048;
				break;
			case Tiering::Renderer::ShadowResolutionSetting::High:
				shadowMapResolution = 4096;
				break;
		}

		// Bloom Compute
		{
			auto shader = Renderer::GetShaderLibrary()->Get("Bloom");
			m_BloomComputePipeline = PipelineCompute::Create(shader);
			{
				TextureSpecification spec;
				spec.Format = ImageFormat::RGBA32F;
				spec.Width = 1;
				spec.Height = 1;
				spec.SamplerWrap = TextureWrap::Clamp;
				spec.Storage = true;
				spec.GenerateMips = true;
				spec.DebugName = "BloomCompute-0";
				m_BloomComputeTextures[0].Texture = Texture2D::Create(spec);
				spec.DebugName = "BloomCompute-1";
				m_BloomComputeTextures[1].Texture = Texture2D::Create(spec);
				spec.DebugName = "BloomCompute-2";
				m_BloomComputeTextures[2].Texture = Texture2D::Create(spec);
			}

			// Image Views (per-mip)
			ImageViewSpecification imageViewSpec;
			for (int i = 0; i < 3; i++)
			{
				imageViewSpec.DebugName = fmt::format("BloomCompute-{}", i);
				uint32_t mipCount = m_BloomComputeTextures[i].Texture->GetMipLevelCount();
				m_BloomComputeTextures[i].ImageViews.resize(mipCount);
				for (uint32_t mip = 0; mip < mipCount; mip++)
				{
					imageViewSpec.Image = m_BloomComputeTextures[i].Texture->GetImage();
					imageViewSpec.Mip = mip;
					m_BloomComputeTextures[i].ImageViews[mip] = ImageView::Create(imageViewSpec);
				}
			}

			{
				ComputePassSpecification spec;
				spec.DebugName = "Bloom-Compute";
				auto shader = Renderer::GetShaderLibrary()->Get("Bloom");
				spec.Pipeline = PipelineCompute::Create(shader);
				m_BloomComputePass = ComputePass::Create(spec);
				ZONG_CORE_VERIFY(m_BloomComputePass->Validate());
				m_BloomComputePass->Bake();
			}

			m_BloomDirtTexture = Renderer::GetBlackTexture();

			// TODO
			// m_BloomComputePrefilterMaterial = Material::Create(shader);
		}

		// Directional Shadow pass
		{
			ImageSpecification spec;
			spec.Format = ImageFormat::DEPTH32F;
			spec.Usage = ImageUsage::Attachment;
			spec.Width = shadowMapResolution;
			spec.Height = shadowMapResolution;
			spec.Layers = m_Specification.NumShadowCascades;
			spec.DebugName = "ShadowCascades";
			Ref<Image2D> cascadedDepthImage = Image2D::Create(spec);
			cascadedDepthImage->Invalidate();
			if (m_Specification.NumShadowCascades > 1)
				cascadedDepthImage->CreatePerLayerImageViews();

			FramebufferSpecification shadowMapFramebufferSpec;
			shadowMapFramebufferSpec.DebugName = "Dir Shadow Map";
			shadowMapFramebufferSpec.Width = shadowMapResolution;
			shadowMapFramebufferSpec.Height = shadowMapResolution;
			shadowMapFramebufferSpec.Attachments = { ImageFormat::DEPTH32F };
			shadowMapFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			shadowMapFramebufferSpec.DepthClearValue = 1.0f;
			shadowMapFramebufferSpec.NoResize = true;
			shadowMapFramebufferSpec.ExistingImage = cascadedDepthImage;

			auto shadowPassShader = Renderer::GetShaderLibrary()->Get("DirShadowMap");
			auto shadowPassShaderAnim = Renderer::GetShaderLibrary()->Get("DirShadowMap_Anim");

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "DirShadowPass";
			pipelineSpec.Shader = shadowPassShader;
			pipelineSpec.DepthOperator = DepthCompareOperator::LessOrEqual;
			pipelineSpec.Layout = vertexLayout;
			pipelineSpec.InstanceLayout = instanceLayout;

			PipelineSpecification pipelineSpecAnim = pipelineSpec;
			pipelineSpecAnim.DebugName = "DirShadowPass-Anim";
			pipelineSpecAnim.Shader = shadowPassShaderAnim;
			pipelineSpecAnim.BoneInfluenceLayout = boneInfluenceLayout;

			RenderPassSpecification shadowMapRenderPassSpec;
			shadowMapRenderPassSpec.DebugName = shadowMapFramebufferSpec.DebugName;

			// 4 cascades by default
			m_DirectionalShadowMapPass.resize(m_Specification.NumShadowCascades);
			m_DirectionalShadowMapAnimPass.resize(m_Specification.NumShadowCascades);
			for (uint32_t i = 0; i < m_Specification.NumShadowCascades; i++)
			{
				shadowMapFramebufferSpec.ExistingImageLayers.clear();
				shadowMapFramebufferSpec.ExistingImageLayers.emplace_back(i);

				shadowMapFramebufferSpec.ClearDepthOnLoad = true;
				pipelineSpec.TargetFramebuffer = Framebuffer::Create(shadowMapFramebufferSpec);

				m_ShadowPassPipelines[i] = Pipeline::Create(pipelineSpec);
				shadowMapRenderPassSpec.Pipeline = m_ShadowPassPipelines[i];

				shadowMapFramebufferSpec.ClearDepthOnLoad = false;
				pipelineSpecAnim.TargetFramebuffer = Framebuffer::Create(shadowMapFramebufferSpec);
				m_ShadowPassPipelinesAnim[i] = Pipeline::Create(pipelineSpecAnim);

				m_DirectionalShadowMapPass[i] = RenderPass::Create(shadowMapRenderPassSpec);
				m_DirectionalShadowMapPass[i]->SetInput("ShadowData", m_UBSShadow);
				ZONG_CORE_VERIFY(m_DirectionalShadowMapPass[i]->Validate());
				m_DirectionalShadowMapPass[i]->Bake();

				shadowMapRenderPassSpec.Pipeline = m_ShadowPassPipelinesAnim[i];
				m_DirectionalShadowMapAnimPass[i] = RenderPass::Create(shadowMapRenderPassSpec);
				m_DirectionalShadowMapAnimPass[i]->SetInput("ShadowData", m_UBSShadow);
				m_DirectionalShadowMapAnimPass[i]->SetInput("BoneTransforms", m_SBSBoneTransforms);
				ZONG_CORE_VERIFY(m_DirectionalShadowMapAnimPass[i]->Validate());
				m_DirectionalShadowMapAnimPass[i]->Bake();
			}
			m_ShadowPassMaterial = Material::Create(shadowPassShader, "DirShadowPass");
		}

		// Non-directional shadow mapping pass
		{
			FramebufferSpecification framebufferSpec;
			framebufferSpec.Width = shadowMapResolution; // TODO(Yan): could probably halve these
			framebufferSpec.Height = shadowMapResolution;
			framebufferSpec.Attachments = { ImageFormat::DEPTH32F };
			framebufferSpec.DepthClearValue = 1.0f;
			framebufferSpec.NoResize = true;
			framebufferSpec.DebugName = "Spot Shadow Map";

			auto shadowPassShader = Renderer::GetShaderLibrary()->Get("SpotShadowMap");
			auto shadowPassShaderAnim = Renderer::GetShaderLibrary()->Get("SpotShadowMap_Anim");

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "SpotShadowPass";
			pipelineSpec.Shader = shadowPassShader;
			pipelineSpec.TargetFramebuffer = Framebuffer::Create(framebufferSpec);
			pipelineSpec.DepthOperator = DepthCompareOperator::LessOrEqual;

			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpec.InstanceLayout = {
				{ ShaderDataType::Float4, "a_MRow0" },
				{ ShaderDataType::Float4, "a_MRow1" },
				{ ShaderDataType::Float4, "a_MRow2" },
			};

			PipelineSpecification pipelineSpecAnim = pipelineSpec;
			pipelineSpecAnim.DebugName = "SpotShadowPass-Anim";
			pipelineSpecAnim.Shader = shadowPassShaderAnim;
			pipelineSpecAnim.BoneInfluenceLayout = boneInfluenceLayout;

			m_SpotShadowPassPipeline = Pipeline::Create(pipelineSpec);
			m_SpotShadowPassAnimPipeline = Pipeline::Create(pipelineSpecAnim);

			m_SpotShadowPassMaterial = Material::Create(shadowPassShader, "SpotShadowPass");

			RenderPassSpecification spotShadowPassSpec;
			spotShadowPassSpec.DebugName = "SpotShadowMap";
			spotShadowPassSpec.Pipeline = m_SpotShadowPassPipeline;
			m_SpotShadowPass = RenderPass::Create(spotShadowPassSpec);

			m_SpotShadowPass->SetInput("SpotShadowData", m_UBSSpotShadowData);

			ZONG_CORE_VERIFY(m_SpotShadowPass->Validate());
			m_SpotShadowPass->Bake();
		}

		// Define/setup renderer resources that are provided by CPU (meaning not GPU pass outputs)
		// eg. camera buffer, environment textures, BRDF LUT, etc.

		// CHANGES:
		// - RenderPass class becomes implemented instead of just placeholder for data
		// - Pipeline no longer contains render pass in spec (NOTE: double check this is okay)
		// - RenderPass contains Pipeline in spec

		{
			//
			// A render pass should provide context and initiate (prepare) certain layout transitions for all
			// required resources. This should be easy to check and ensure everything is ready.
			// 
			// Passes should be somewhat pre-baked to contain ready-to-go descriptors that aren't draw-call
			// specific. Hazel defines Set 0 as per-draw - so usually materials. Sets 1-3 are scene/renderer owned.
			//
			// This means that when you define a render pass, you need to set-up required inputs from Sets 1-3
			// and based on what is used here, these need to get baked into ready-to-go allocated descriptor sets,
			// for that specific render pass draw call (so we can use vkCmdBindDescriptorSets).
			//
			// API could look something like:

			// Ref<RenderPass> shadowMapRenderPass[4]; // Four shadow map passes, into single layered framebuffer
			// 
			// {
			// 	for (int i = 0; i < 4; i++)
			// 	{
			// 		RenderPassSpecification spec;
			// 		spec.DebugName = "ShadowMapPass";
			// 		spec.Pipeline = m_ShadowPassPipelines[i];
			// 		//spec.TargetFramebuffer = m_ShadowPassPipelines[i]->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer; // <- set framebuffer here
			// 
			// 		shadowMapRenderPass[i] = RenderPass::Create(spec);
			// 		// shadowMapRenderPass[i]->GetRequiredInputs(); // Returns list of sets+bindings of required resources from descriptor layout
			// 
			// 		// NOTE:
			// 		// AddInput needs to potentially take in the set + binding of the resource
			// 		// We (currently) don't store the actual variable name, just the struct type (eg. ShadowData and not u_ShadowData),
			// 		// so if there are multiple instances of the ShadowData struct then it's ambiguous.
			// 		// I suspect clashes will be rare - usually we only have one input per type/buffer
			// 		shadowMapRenderPass[i]->SetInput("ShadowData", m_UBSShadow);
			// 		// Note: outputs are automatically set by framebuffer
			// 
			// 		// Bake will create descriptor sets and ensure everything is ready for rendering
			// 		// If resources (eg. storage buffers/images) resize, passes need to be invalidated
			// 		// so we can re-create proper descriptors to the newly created replacement resources
			// 		ZONG_CORE_VERIFY(shadowMapRenderPass[i]->Validate());
			// 		shadowMapRenderPass[i]->Bake();
			// 	}
			// }

			// PreDepth
			{
				FramebufferSpecification preDepthFramebufferSpec;
				preDepthFramebufferSpec.DebugName = "PreDepth-Opaque";
				//Linear depth, reversed device depth
				preDepthFramebufferSpec.Attachments = { /*ImageFormat::RED32F, */ ImageFormat::DEPTH32FSTENCIL8UINT };
				preDepthFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
				preDepthFramebufferSpec.DepthClearValue = 0.0f;
			
				Ref<Framebuffer> clearFramebuffer = Framebuffer::Create(preDepthFramebufferSpec);
				preDepthFramebufferSpec.ClearDepthOnLoad = false;
				preDepthFramebufferSpec.ExistingImages[0] = clearFramebuffer->GetDepthImage();
				Ref<Framebuffer> loadFramebuffer = Framebuffer::Create(preDepthFramebufferSpec);

				PipelineSpecification pipelineSpec;
				pipelineSpec.DebugName = preDepthFramebufferSpec.DebugName;
				pipelineSpec.TargetFramebuffer = clearFramebuffer;
				pipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("PreDepth");
				pipelineSpec.Layout = vertexLayout;
				pipelineSpec.InstanceLayout = instanceLayout;
				m_PreDepthPipeline = Pipeline::Create(pipelineSpec);
				m_PreDepthMaterial = Material::Create(pipelineSpec.Shader, pipelineSpec.DebugName);

				// Change to loading framebuffer so we don't clear
				pipelineSpec.TargetFramebuffer = loadFramebuffer;

				pipelineSpec.DebugName = "PreDepth-Anim";
				pipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("PreDepth_Anim");
				pipelineSpec.BoneInfluenceLayout = boneInfluenceLayout;
				m_PreDepthPipelineAnim = Pipeline::Create(pipelineSpec);  // same renderpass as Predepth-Opaque

				pipelineSpec.DebugName = "PreDepth-Transparent";
				pipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("PreDepth");
				preDepthFramebufferSpec.DebugName = pipelineSpec.DebugName;
				m_PreDepthTransparentPipeline = Pipeline::Create(pipelineSpec);

				// TODO(0x): Need PreDepth-Transparent-Animated pipeline

				// Static
				RenderPassSpecification preDepthRenderPassSpec;
				preDepthRenderPassSpec.DebugName = preDepthFramebufferSpec.DebugName;
				preDepthRenderPassSpec.Pipeline = m_PreDepthPipeline;

				m_PreDepthPass = RenderPass::Create(preDepthRenderPassSpec);
				m_PreDepthPass->SetInput("Camera", m_UBSCamera);
				ZONG_CORE_VERIFY(m_PreDepthPass->Validate());
				m_PreDepthPass->Bake();

				// Animated
				preDepthRenderPassSpec.DebugName = "PreDepth-Anim";
				preDepthRenderPassSpec.Pipeline = m_PreDepthPipelineAnim;
				m_PreDepthAnimPass = RenderPass::Create(preDepthRenderPassSpec);
				m_PreDepthAnimPass->SetInput("Camera", m_UBSCamera);
				m_PreDepthAnimPass->SetInput("BoneTransforms", m_SBSBoneTransforms);
				ZONG_CORE_VERIFY(m_PreDepthAnimPass->Validate());
				m_PreDepthAnimPass->Bake();

				// Transparent
				preDepthRenderPassSpec.DebugName = "PreDepth-Transparent";
				preDepthRenderPassSpec.Pipeline = m_PreDepthTransparentPipeline;
				m_PreDepthTransparentPass = RenderPass::Create(preDepthRenderPassSpec);
				m_PreDepthTransparentPass->SetInput("Camera", m_UBSCamera);

				ZONG_CORE_VERIFY(m_PreDepthTransparentPass->Validate());
				m_PreDepthTransparentPass->Bake();
			}

			// Geometry
			{
				ImageSpecification imageSpec;
				imageSpec.Width = Application::Get().GetWindow().GetWidth();
				imageSpec.Height = Application::Get().GetWindow().GetHeight();
				imageSpec.Format = ImageFormat::RGBA32F;
				imageSpec.Usage = ImageUsage::Attachment;
				imageSpec.DebugName = "GeometryPass-ColorAttachment0";
				m_GeometryPassColorAttachmentImage = Image2D::Create(imageSpec);
				m_GeometryPassColorAttachmentImage->Invalidate();

				FramebufferSpecification geoFramebufferSpec;
				geoFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::RGBA16F, ImageFormat::RGBA, ImageFormat::DEPTH32FSTENCIL8UINT };
				geoFramebufferSpec.ExistingImages[3] = m_PreDepthPass->GetDepthOutput();

				// Don't clear primary color attachment (skybox pass writes into it)
				geoFramebufferSpec.Attachments.Attachments[0].LoadOp = AttachmentLoadOp::Load;
				// Don't blend with luminance in the alpha channel.
				geoFramebufferSpec.Attachments.Attachments[1].Blend = false;
				geoFramebufferSpec.Samples = 1;
				geoFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
				geoFramebufferSpec.DebugName = "Geometry";
				geoFramebufferSpec.ClearDepthOnLoad = false;
				Ref<Framebuffer> clearFramebuffer = Framebuffer::Create(geoFramebufferSpec);
				geoFramebufferSpec.ClearColorOnLoad = false;
				geoFramebufferSpec.ExistingImages[0] = clearFramebuffer->GetImage(0);
				geoFramebufferSpec.ExistingImages[1] = clearFramebuffer->GetImage(1);
				geoFramebufferSpec.ExistingImages[2] = clearFramebuffer->GetImage(2);
				geoFramebufferSpec.ExistingImages[3] = m_PreDepthPass->GetDepthOutput();
				Ref<Framebuffer> loadFramebuffer = Framebuffer::Create(geoFramebufferSpec);

				PipelineSpecification pipelineSpecification;
				pipelineSpecification.DebugName = "PBR-Static";
				pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("HazelPBR_Static");
				pipelineSpecification.TargetFramebuffer = clearFramebuffer;
				pipelineSpecification.DepthOperator = DepthCompareOperator::Equal;
				pipelineSpecification.DepthWrite = false;
				pipelineSpecification.Layout = vertexLayout;
				pipelineSpecification.InstanceLayout = instanceLayout;
				pipelineSpecification.LineWidth = m_LineWidth;
				m_GeometryPipeline = Pipeline::Create(pipelineSpecification);

				// Switch to load framebuffer to not clear subsequent passes
				pipelineSpecification.TargetFramebuffer = loadFramebuffer;

				//
				// Transparent Geometry
				//
				pipelineSpecification.DebugName = "PBR-Transparent";
				pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("HazelPBR_Transparent");
				pipelineSpecification.DepthOperator = DepthCompareOperator::GreaterOrEqual;
				m_TransparentGeometryPipeline = Pipeline::Create(pipelineSpecification);

				//
				// Animated Geometry
				//
				pipelineSpecification.DebugName = "PBR-Anim";
				pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("HazelPBR_Anim");
				pipelineSpecification.DepthOperator = DepthCompareOperator::Equal;
				pipelineSpecification.BoneInfluenceLayout = boneInfluenceLayout;
				m_GeometryPipelineAnim = Pipeline::Create(pipelineSpecification);

				// TODO(0x): Need Transparent-Animated geometry pipeline
			}

			// Selected Geometry isolation (for outline pass)
			{
				FramebufferSpecification framebufferSpec;
				framebufferSpec.DebugName = "SelectedGeometry";
				framebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };
				framebufferSpec.Samples = 1;
				framebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
				framebufferSpec.DepthClearValue = 1.0f;

				PipelineSpecification pipelineSpecification;
				pipelineSpecification.DebugName = framebufferSpec.DebugName;
				pipelineSpecification.TargetFramebuffer = Framebuffer::Create(framebufferSpec);
				pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("SelectedGeometry");
				pipelineSpecification.Layout = vertexLayout;
				pipelineSpecification.InstanceLayout = instanceLayout;
				pipelineSpecification.DepthOperator = DepthCompareOperator::LessOrEqual;

				RenderPassSpecification rpSpec;
				rpSpec.DebugName = "SelectedGeometry";
				rpSpec.Pipeline = Pipeline::Create(pipelineSpecification);
				m_SelectedGeometryPass = RenderPass::Create(rpSpec);
				m_SelectedGeometryPass->SetInput("Camera", m_UBSCamera);
				ZONG_CORE_VERIFY(m_SelectedGeometryPass->Validate());
				m_SelectedGeometryPass->Bake();

				m_SelectedGeometryMaterial = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);

				pipelineSpecification.DebugName = "SelectedGeometry-Anim";
				pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("SelectedGeometry_Anim");
				pipelineSpecification.BoneInfluenceLayout = boneInfluenceLayout;
				framebufferSpec.ExistingFramebuffer = m_SelectedGeometryPass->GetTargetFramebuffer();
				framebufferSpec.ClearColorOnLoad = false;
				framebufferSpec.ClearDepthOnLoad = false;
				pipelineSpecification.TargetFramebuffer = Framebuffer::Create(framebufferSpec);
				rpSpec.Pipeline = Pipeline::Create(pipelineSpecification);
				m_SelectedGeometryAnimPass = RenderPass::Create(rpSpec); // Note: same framebuffer and renderpass as m_SelectedGeometryPipeline
				m_SelectedGeometryAnimPass->SetInput("Camera", m_UBSCamera);
				m_SelectedGeometryAnimPass->SetInput("BoneTransforms", m_SBSBoneTransforms);
				ZONG_CORE_VERIFY(m_SelectedGeometryAnimPass->Validate());
				m_SelectedGeometryAnimPass->Bake();
			}

			{
				RenderPassSpecification spec;
				spec.DebugName = "GeometryPass";
				spec.Pipeline = m_GeometryPipeline;

				m_GeometryPass = RenderPass::Create(spec);
				// geometryRenderPass->GetRequiredInputs(); // Returns list of sets+bindings of required resources from descriptor layout

				m_GeometryPass->SetInput("Camera", m_UBSCamera);

				m_GeometryPass->SetInput("SpotShadowData", m_UBSSpotShadowData);
				m_GeometryPass->SetInput("ShadowData", m_UBSShadow);
				m_GeometryPass->SetInput("PointLightData", m_UBSPointLights);
				m_GeometryPass->SetInput("SpotLightData", m_UBSSpotLights);
				m_GeometryPass->SetInput("SceneData", m_UBSScene);
				m_GeometryPass->SetInput("VisiblePointLightIndicesBuffer", m_SBSVisiblePointLightIndicesBuffer);
				m_GeometryPass->SetInput("VisibleSpotLightIndicesBuffer", m_SBSVisibleSpotLightIndicesBuffer);
				
				m_GeometryPass->SetInput("RendererData", m_UBSRendererData);

				// Some resources that are scene specific cannot be set before
				// SceneRenderer::BeginScene. As such these should be placeholders that
				// when invalidated are replaced with real resources
				m_GeometryPass->SetInput("u_EnvRadianceTex", Renderer::GetBlackCubeTexture());
				m_GeometryPass->SetInput("u_EnvIrradianceTex", Renderer::GetBlackCubeTexture());

				// Set 3
				m_GeometryPass->SetInput("u_BRDFLUTTexture", Renderer::GetBRDFLutTexture());

				// Some resources are the results of previous passes. This will enforce
				// a layout transition when RenderPass::Begin() is called. GetOutput(0)
				// refers to the first output of the pass - since the pass is depth-only,
				// this will be the depth image
				m_GeometryPass->SetInput("u_ShadowMapTexture", m_DirectionalShadowMapPass[0]->GetOutput(0));
				m_GeometryPass->SetInput("u_SpotShadowTexture", m_SpotShadowPass->GetOutput(0));

				// Note: outputs are automatically set by framebuffer

				// Bake will create descriptor sets and ensure everything is ready for rendering
				// If resources (eg. storage buffers/images) resize, passes need to be invalidated
				// so we can re-create proper descriptors to the newly created replacement resources
				ZONG_CORE_VERIFY(m_GeometryPass->Validate());
				m_GeometryPass->Bake();
			}

			{
				RenderPassSpecification spec;
				spec.DebugName = "GeometryPass-Animated";
				spec.Pipeline = m_GeometryPipelineAnim;
				m_GeometryAnimPass = RenderPass::Create(spec);

				m_GeometryAnimPass->SetInput("Camera", m_UBSCamera);

				m_GeometryAnimPass->SetInput("SpotShadowData", m_UBSSpotShadowData);
				m_GeometryAnimPass->SetInput("ShadowData", m_UBSShadow);
				m_GeometryAnimPass->SetInput("PointLightData", m_UBSPointLights);
				m_GeometryAnimPass->SetInput("SpotLightData", m_UBSSpotLights);
				m_GeometryAnimPass->SetInput("SceneData", m_UBSScene);
				m_GeometryAnimPass->SetInput("VisiblePointLightIndicesBuffer", m_SBSVisiblePointLightIndicesBuffer);
				m_GeometryAnimPass->SetInput("VisibleSpotLightIndicesBuffer", m_SBSVisibleSpotLightIndicesBuffer);
				
				m_GeometryAnimPass->SetInput("BoneTransforms", m_SBSBoneTransforms);

				m_GeometryAnimPass->SetInput("RendererData", m_UBSRendererData);

				m_GeometryAnimPass->SetInput("u_EnvRadianceTex", Renderer::GetBlackCubeTexture());
				m_GeometryAnimPass->SetInput("u_EnvIrradianceTex", Renderer::GetBlackCubeTexture());

				// Set 3
				m_GeometryAnimPass->SetInput("u_BRDFLUTTexture", Renderer::GetBRDFLutTexture());

				m_GeometryAnimPass->SetInput("u_ShadowMapTexture", m_DirectionalShadowMapPass[0]->GetOutput(0));
				m_GeometryAnimPass->SetInput("u_SpotShadowTexture", m_SpotShadowPass->GetOutput(0));

				ZONG_CORE_VERIFY(m_GeometryAnimPass->Validate());
				m_GeometryAnimPass->Bake();
			}

			{
				ComputePassSpecification spec;
				spec.DebugName = "LightCulling";
				spec.Pipeline = m_LightCullingPipeline;
				m_LightCullingPass = ComputePass::Create(spec);

				m_LightCullingPass->SetInput("Camera", m_UBSCamera);
				m_LightCullingPass->SetInput("ScreenData", m_UBSScreenData);
				m_LightCullingPass->SetInput("PointLightData", m_UBSPointLights);
				m_LightCullingPass->SetInput("SpotLightData", m_UBSSpotLights);
				m_LightCullingPass->SetInput("VisiblePointLightIndicesBuffer", m_SBSVisiblePointLightIndicesBuffer);
				m_LightCullingPass->SetInput("VisibleSpotLightIndicesBuffer", m_SBSVisibleSpotLightIndicesBuffer);
				m_LightCullingPass->SetInput("u_DepthMap", m_PreDepthPass->GetDepthOutput());
				ZONG_CORE_VERIFY(m_LightCullingPass->Validate());
				m_LightCullingPass->Bake();
			}

#if 0
			// Render example

			// BeginRenderPass needs to do the following:
			// - insert layout transitions for input resources that are required
			// - call vkCmdBeginRenderPass
			// - bind pipeline - one render pass is only allowed one pipeline, and
			//   the actual render pass should be able to contain it
			// - bind descriptor sets that are not per-draw-call, meaning Sets 1-3
			//     - this makes sense because they are "global" for entire render pass,
			//       so these can easily be bound up-front

			Renderer::BeginRenderPass(m_CommandBuffer, shadowMapRenderPass);

			// Render functions (which issue draw calls) should no longer be concerned with
			// binding the pipeline or descriptor sets 1-3, they only:
			// - bind vertex/index buffers
			// - bind descriptor set 0, which contains material resources
			// - push constants
			// - actual draw call

			Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, ...);
			Renderer::EndRenderPass(m_CommandBuffer);

			Renderer::BeginRenderPass(m_CommandBuffer, geometryRenderPass);
			Renderer::RenderStaticMesh(m_CommandBuffer, ...);
			Renderer::EndRenderPass(m_CommandBuffer);
#endif
		}
	
#if 0
		// Deinterleaving
		{
			ImageSpecification imageSpec;
			imageSpec.Format = ImageFormat::RED32F;
			imageSpec.Layers = 16;
			imageSpec.Usage = ImageUsage::Attachment;
			imageSpec.DebugName = "Deinterleaved";
			Ref<Image2D> image = Image2D::Create(imageSpec);
			image->Invalidate();
			image->CreatePerLayerImageViews();

			FramebufferSpecification deinterleavingFramebufferSpec;
			deinterleavingFramebufferSpec.Attachments.Attachments = { 8, FramebufferTextureSpecification{ ImageFormat::RED32F } };
			deinterleavingFramebufferSpec.ClearColor = { 1.0f, 0.0f, 0.0f, 1.0f };
			deinterleavingFramebufferSpec.DebugName = "Deinterleaving";
			deinterleavingFramebufferSpec.ExistingImage = image;

			Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("Deinterleaving");
			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "Deinterleaving";
			pipelineSpec.DepthWrite = false;
			pipelineSpec.DepthTest = false;

			pipelineSpec.Shader = shader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};

			// 2 frame buffers, 2 render passes .. 8 attachments each
			for (int rp = 0; rp < 2; rp++)
			{
				deinterleavingFramebufferSpec.ExistingImageLayers.clear();
				for (int layer = 0; layer < 8; layer++)
					deinterleavingFramebufferSpec.ExistingImageLayers.emplace_back(rp * 8 + layer);

				pipelineSpec.TargetFramebuffer = Framebuffer::Create(deinterleavingFramebufferSpec);
				m_DeinterleavingPipelines[rp] = Pipeline::Create(pipelineSpec);

				RenderPassSpecification spec;
				spec.DebugName = "Deinterleaving";
				spec.Pipeline = m_DeinterleavingPipelines[rp];
				m_DeinterleavingPass[rp] = RenderPass::Create(spec);

				m_DeinterleavingPass[rp]->SetInput("u_Depth", m_PreDepthPass->GetDepthOutput());
				m_DeinterleavingPass[rp]->SetInput("Camera", m_UBSCamera);
				m_DeinterleavingPass[rp]->SetInput("ScreenData", m_UBSScreenData);
				ZONG_CORE_VERIFY(m_DeinterleavingPass[rp]->Validate());
				m_DeinterleavingPass[rp]->Bake();
			}
			m_DeinterleavingMaterial = Material::Create(pipelineSpec.Shader, pipelineSpec.DebugName);
		}
#endif

		// Hierarchical Z buffer
		{
			TextureSpecification spec;
			spec.Format = ImageFormat::RED32F;
			spec.Width = 1;
			spec.Height = 1;
			spec.SamplerWrap = TextureWrap::Clamp;
			spec.SamplerFilter = TextureFilter::Nearest;
			spec.DebugName = "HierarchicalZ";

			m_HierarchicalDepthTexture.Texture = Texture2D::Create(spec);

			Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("HZB");

			ComputePassSpecification hdPassSpec;
			hdPassSpec.DebugName = "HierarchicalDepth";
			hdPassSpec.Pipeline = PipelineCompute::Create(shader);
			m_HierarchicalDepthPass = ComputePass::Create(hdPassSpec);

			ZONG_CORE_VERIFY(m_HierarchicalDepthPass->Validate());
			m_HierarchicalDepthPass->Bake();
		}

		// SSR Composite
		{
			ImageSpecification imageSpec;
			imageSpec.Format = ImageFormat::RGBA16F;
			imageSpec.Usage = ImageUsage::Storage;
			imageSpec.DebugName = "SSR";
			m_SSRImage = Image2D::Create(imageSpec);
			m_SSRImage->Invalidate();

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.DepthTest = false;
			pipelineSpecification.DepthWrite = false;
			pipelineSpecification.DebugName = "SSR-Composite";
			auto shader = Renderer::GetShaderLibrary()->Get("SSR-Composite");
			pipelineSpecification.Shader = shader;

			FramebufferSpecification framebufferSpec;
			framebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			framebufferSpec.Attachments.Attachments.emplace_back(ImageFormat::RGBA32F);
			framebufferSpec.ExistingImages[0] = m_GeometryPass->GetOutput(0);
			framebufferSpec.DebugName = "SSR-Composite";
			framebufferSpec.BlendMode = FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha;
			framebufferSpec.ClearColorOnLoad = false;
			pipelineSpecification.TargetFramebuffer = Framebuffer::Create(framebufferSpec);

			RenderPassSpecification renderPassSpec;
			renderPassSpec.DebugName = "SSR-Composite";
			renderPassSpec.Pipeline = Pipeline::Create(pipelineSpecification);
			m_SSRCompositePass = RenderPass::Create(renderPassSpec);
			m_SSRCompositePass->SetInput("u_SSR", m_SSRImage);
			ZONG_CORE_VERIFY(m_SSRCompositePass->Validate());
			m_SSRCompositePass->Bake();
		}

		// Pre-Integration
		{
			TextureSpecification spec;
			spec.Format = ImageFormat::RED8UN;
			spec.Width = 1;
			spec.Height = 1;
			spec.DebugName = "Pre-Integration";

			m_PreIntegrationVisibilityTexture.Texture = Texture2D::Create(spec);

			Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("Pre-Integration");
			ComputePassSpecification passSpec;
			passSpec.DebugName = "Pre-Integration";
			passSpec.Pipeline = PipelineCompute::Create(shader);
			m_PreIntegrationPass = ComputePass::Create(passSpec);
		}

		// Pre-convolution Compute
		{
			TextureSpecification spec;
			spec.Format = ImageFormat::RGBA32F;
			spec.Width = 1;
			spec.Height = 1;
			spec.SamplerWrap = TextureWrap::Clamp;
			spec.DebugName = "Pre-Convoluted";
			spec.Storage = true;
			m_PreConvolutedTexture.Texture = Texture2D::Create(spec);

			Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("Pre-Convolution");
			ComputePassSpecification passSpec;
			passSpec.DebugName = "Pre-Integration";
			passSpec.Pipeline = PipelineCompute::Create(shader);
			m_PreConvolutionComputePass = ComputePass::Create(passSpec);
			ZONG_CORE_VERIFY(m_PreConvolutionComputePass->Validate());
			m_PreConvolutionComputePass->Bake();
		}

		// Edge Detection
		if (m_Specification.EnableEdgeOutlineEffect)
		{
			FramebufferSpecification compFramebufferSpec;
			compFramebufferSpec.DebugName = "POST-EdgeDetection";
			compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			compFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };
			compFramebufferSpec.Transfer = true;

			Ref<Framebuffer> framebuffer = Framebuffer::Create(compFramebufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("EdgeDetection");
			pipelineSpecification.TargetFramebuffer = framebuffer;
			pipelineSpecification.DebugName = compFramebufferSpec.DebugName;
			pipelineSpecification.DepthWrite = false;

			RenderPassSpecification renderPassSpec;
			renderPassSpec.DebugName = compFramebufferSpec.DebugName;
			renderPassSpec.Pipeline = Pipeline::Create(pipelineSpecification);
			m_EdgeDetectionPass = RenderPass::Create(renderPassSpec);
			m_EdgeDetectionPass->SetInput("u_ViewNormalsTexture", m_GeometryPass->GetOutput(1));
			m_EdgeDetectionPass->SetInput("u_DepthTexture", m_PreDepthPass->GetDepthOutput());
			m_EdgeDetectionPass->SetInput("Camera", m_UBSCamera);
			ZONG_CORE_VERIFY(m_EdgeDetectionPass->Validate());
			m_EdgeDetectionPass->Bake();
		}

		// Composite
		{
			FramebufferSpecification compFramebufferSpec;
			compFramebufferSpec.DebugName = "SceneComposite";
			compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			compFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };
			compFramebufferSpec.Transfer = true;

			Ref<Framebuffer> framebuffer = Framebuffer::Create(compFramebufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("SceneComposite");
			pipelineSpecification.TargetFramebuffer = framebuffer;
			pipelineSpecification.DebugName = "SceneComposite";
			pipelineSpecification.DepthWrite = false;
			pipelineSpecification.DepthTest = false;

			RenderPassSpecification renderPassSpec;
			renderPassSpec.DebugName = "SceneComposite";
			renderPassSpec.Pipeline = Pipeline::Create(pipelineSpecification);
			m_CompositePass = RenderPass::Create(renderPassSpec);
			m_CompositePass->SetInput("u_Texture", m_GeometryPass->GetOutput(0));
			m_CompositePass->SetInput("u_BloomTexture", m_BloomComputeTextures[2].Texture);
			m_CompositePass->SetInput("u_BloomDirtTexture", m_BloomDirtTexture);
			m_CompositePass->SetInput("u_DepthTexture", m_PreDepthPass->GetDepthOutput());
			m_CompositePass->SetInput("u_TransparentDepthTexture", m_PreDepthTransparentPass->GetDepthOutput());

			if (m_Specification.EnableEdgeOutlineEffect)
				m_CompositePass->SetInput("u_EdgeTexture", m_EdgeDetectionPass->GetOutput(0));
			
			m_CompositePass->SetInput("Camera", m_UBSCamera);

			ZONG_CORE_VERIFY(m_CompositePass->Validate());
			m_CompositePass->Bake();
		}

		// DOF
		{
			FramebufferSpecification compFramebufferSpec;
			compFramebufferSpec.DebugName = "POST-DepthOfField";
			compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			compFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };
			compFramebufferSpec.Transfer = true;

			Ref<Framebuffer> framebuffer = Framebuffer::Create(compFramebufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("DOF");
			pipelineSpecification.DebugName = compFramebufferSpec.DebugName;
			pipelineSpecification.DepthWrite = false;
			pipelineSpecification.TargetFramebuffer = framebuffer;
			m_DOFMaterial = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);

			RenderPassSpecification renderPassSpec;
			renderPassSpec.DebugName = "POST-DOF";
			renderPassSpec.Pipeline = Pipeline::Create(pipelineSpecification);
			m_DOFPass = RenderPass::Create(renderPassSpec);
			m_DOFPass->SetInput("u_Texture", m_CompositePass->GetOutput(0));
			m_DOFPass->SetInput("u_DepthTexture", m_PreDepthPass->GetDepthOutput());
			m_DOFPass->SetInput("Camera", m_UBSCamera);
			ZONG_CORE_VERIFY(m_DOFPass->Validate());
			m_DOFPass->Bake();
		}

		FramebufferSpecification fbSpec;
		fbSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::DEPTH32FSTENCIL8UINT };
		fbSpec.ClearColorOnLoad = false;
		fbSpec.ClearDepthOnLoad = false;
		fbSpec.ExistingImages[0] = m_CompositePass->GetOutput(0);
		fbSpec.ExistingImages[1] = m_PreDepthPass->GetDepthOutput();
		m_CompositingFramebuffer = Framebuffer::Create(fbSpec);

		// Wireframe
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "Wireframe";
			pipelineSpecification.TargetFramebuffer = m_CompositingFramebuffer;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Wireframe");
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.Wireframe = true;
			pipelineSpecification.DepthTest = true;
			pipelineSpecification.LineWidth = 2.0f;
			pipelineSpecification.Layout = vertexLayout;
			pipelineSpecification.InstanceLayout = instanceLayout;

			RenderPassSpecification renderPassSpec;
			renderPassSpec.DebugName = "Geometry-Wireframe";
			renderPassSpec.Pipeline = Pipeline::Create(pipelineSpecification);;
			m_GeometryWireframePass = RenderPass::Create(renderPassSpec);
			m_GeometryWireframePass->SetInput("Camera", m_UBSCamera);
			ZONG_CORE_VERIFY(m_GeometryWireframePass->Validate());
			m_GeometryWireframePass->Bake();

			pipelineSpecification.DepthTest = false;
			pipelineSpecification.DebugName = "Wireframe-OnTop";
			renderPassSpec.Pipeline = Pipeline::Create(pipelineSpecification);
			renderPassSpec.DebugName = pipelineSpecification.DebugName;
			m_GeometryWireframeOnTopPass = RenderPass::Create(renderPassSpec);
			m_GeometryWireframeOnTopPass->SetInput("Camera", m_UBSCamera);
			ZONG_CORE_VERIFY(m_GeometryWireframeOnTopPass->Validate());
			m_GeometryWireframeOnTopPass->Bake();

			pipelineSpecification.DepthTest = true;
			pipelineSpecification.DebugName = "Wireframe-Anim";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Wireframe_Anim");
			pipelineSpecification.BoneInfluenceLayout = boneInfluenceLayout;
			renderPassSpec.Pipeline = Pipeline::Create(pipelineSpecification); // Note: same framebuffer and renderpass as m_GeometryWireframePipeline
			renderPassSpec.DebugName = pipelineSpecification.DebugName;
			m_GeometryWireframeAnimPass = RenderPass::Create(renderPassSpec);
			m_GeometryWireframeAnimPass->SetInput("Camera", m_UBSCamera);
			m_GeometryWireframeAnimPass->SetInput("BoneTransforms", m_SBSBoneTransforms);
			ZONG_CORE_VERIFY(m_GeometryWireframeAnimPass->Validate());
			m_GeometryWireframeAnimPass->Bake();

			pipelineSpecification.DepthTest = false;
			pipelineSpecification.DebugName = "Wireframe-Anim-OnTop";
			renderPassSpec.Pipeline = Pipeline::Create(pipelineSpecification);
			m_GeometryWireframeOnTopAnimPass = RenderPass::Create(renderPassSpec);
			m_GeometryWireframeOnTopAnimPass->SetInput("Camera", m_UBSCamera);
			m_GeometryWireframeOnTopAnimPass->SetInput("BoneTransforms", m_SBSBoneTransforms);
			ZONG_CORE_VERIFY(m_GeometryWireframeOnTopAnimPass->Validate());
			m_GeometryWireframeOnTopAnimPass->Bake();
		}

		// Read-back Image
		if (false) // WIP
		{
			ImageSpecification spec;
			spec.Format = ImageFormat::RGBA32F;
			spec.Usage = ImageUsage::HostRead;
			spec.Transfer = true;
			spec.DebugName = "ReadBack";
			m_ReadBackImage = Image2D::Create(spec);
		}

		// Temporary framebuffers for re-use
		{
			FramebufferSpecification framebufferSpec;
			framebufferSpec.Attachments = { ImageFormat::RGBA32F };
			framebufferSpec.Samples = 1;
			framebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			framebufferSpec.BlendMode = FramebufferBlendMode::OneZero;
			framebufferSpec.DebugName = "Temporaries";

			for (uint32_t i = 0; i < 2; i++)
				m_TempFramebuffers.emplace_back(Framebuffer::Create(framebufferSpec));
		}

		// Jump Flood (outline)
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "JumpFlood-Init";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFlood_Init");
			pipelineSpecification.TargetFramebuffer = m_TempFramebuffers[0];
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			m_JumpFloodInitMaterial = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);

			RenderPassSpecification renderPassSpec;
			renderPassSpec.DebugName = "JumpFlood-Init";
			renderPassSpec.Pipeline = Pipeline::Create(pipelineSpecification);
			m_JumpFloodInitPass = RenderPass::Create(renderPassSpec);
			m_JumpFloodInitPass->SetInput("u_Texture", m_SelectedGeometryPass->GetOutput(0));
			ZONG_CORE_VERIFY(m_JumpFloodInitPass->Validate());
			m_JumpFloodInitPass->Bake();

			const char* passName[2] = { "EvenPass", "OddPass" };
			for (uint32_t i = 0; i < 2; i++)
			{
				pipelineSpecification.DebugName = renderPassSpec.DebugName;
				pipelineSpecification.TargetFramebuffer = m_TempFramebuffers[(i + 1) % 2];
				pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFlood_Pass");

				renderPassSpec.DebugName = fmt::format("JumpFlood-{0}", passName[i]);
				renderPassSpec.Pipeline = Pipeline::Create(pipelineSpecification);
				m_JumpFloodPass[i] = RenderPass::Create(renderPassSpec);
				m_JumpFloodPass[i]->SetInput("u_Texture", m_TempFramebuffers[i]->GetImage());
				ZONG_CORE_VERIFY(m_JumpFloodPass[i]->Validate());
				m_JumpFloodPass[i]->Bake();

				m_JumpFloodPassMaterial[i] = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);
			}

			// Outline compositing
			if (m_Specification.JumpFloodPass)
			{
				FramebufferSpecification fbSpec;
				fbSpec.Attachments = { ImageFormat::RGBA32F };
				fbSpec.ExistingImages[0] = m_CompositePass->GetOutput(0);
				fbSpec.ClearColorOnLoad = false;
				pipelineSpecification.TargetFramebuffer = Framebuffer::Create(fbSpec); // TODO(Yan): move this and skybox FB to be central, can use same

				pipelineSpecification.DebugName = "JumpFlood-Composite";
				pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFlood_Composite");
				pipelineSpecification.DepthTest = false;
				renderPassSpec.Pipeline = Pipeline::Create(pipelineSpecification);
				m_JumpFloodCompositePass = RenderPass::Create(renderPassSpec);
				m_JumpFloodCompositePass->SetInput("u_Texture", m_TempFramebuffers[1]->GetImage());
				ZONG_CORE_VERIFY(m_JumpFloodCompositePass->Validate());
				m_JumpFloodCompositePass->Bake();

				m_JumpFloodCompositeMaterial = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);
			}
		}

		// Grid
		{
			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "Grid";
			pipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("Grid");
			pipelineSpec.BackfaceCulling = false;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpec.TargetFramebuffer = m_CompositingFramebuffer;

			RenderPassSpecification renderPassSpec;
			renderPassSpec.DebugName = "Grid";
			renderPassSpec.Pipeline = Pipeline::Create(pipelineSpec);
			m_GridRenderPass = RenderPass::Create(renderPassSpec);
			m_GridRenderPass->SetInput("Camera", m_UBSCamera);
			ZONG_CORE_VERIFY(m_GridRenderPass->Validate());
			m_GridRenderPass->Bake();

			const float gridScale = 16.025f;
			const float gridSize = 0.025f;
			m_GridMaterial = Material::Create(pipelineSpec.Shader, pipelineSpec.DebugName);
			m_GridMaterial->Set("u_Settings.Scale", gridScale);
			m_GridMaterial->Set("u_Settings.Size", gridSize);
		}

		// Collider
		m_SimpleColliderMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("Wireframe"), "SimpleCollider");
		m_SimpleColliderMaterial->Set("u_MaterialUniforms.Color", m_Options.SimplePhysicsCollidersColor);
		m_ComplexColliderMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("Wireframe"), "ComplexCollider");
		m_ComplexColliderMaterial->Set("u_MaterialUniforms.Color", m_Options.ComplexPhysicsCollidersColor);

		m_WireframeMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("Wireframe"), "Wireframe");
		m_WireframeMaterial->Set("u_MaterialUniforms.Color", glm::vec4{ 1.0f, 0.5f, 0.0f, 1.0f });

		// Skybox
		{
			auto skyboxShader = Renderer::GetShaderLibrary()->Get("Skybox");

			FramebufferSpecification fbSpec;
			fbSpec.Attachments = { ImageFormat::RGBA32F };
			fbSpec.ExistingImages[0] = m_GeometryPass->GetOutput(0);
			Ref<Framebuffer> skyboxFB = Framebuffer::Create(fbSpec);

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "Skybox";
			pipelineSpec.Shader = skyboxShader;
			pipelineSpec.DepthWrite = false;
			pipelineSpec.DepthTest = false;
			pipelineSpec.DepthWrite = false;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpec.TargetFramebuffer = skyboxFB;
			m_SkyboxPipeline = Pipeline::Create(pipelineSpec);
			m_SkyboxMaterial = Material::Create(pipelineSpec.Shader, pipelineSpec.DebugName);
			m_SkyboxMaterial->SetFlag(MaterialFlag::DepthTest, false);

			RenderPassSpecification renderPassSpec;
			renderPassSpec.DebugName = "Skybox";
			renderPassSpec.Pipeline = m_SkyboxPipeline;
			m_SkyboxPass = RenderPass::Create(renderPassSpec);
			m_SkyboxPass->SetInput("Camera", m_UBSCamera);
			ZONG_CORE_VERIFY(m_SkyboxPass->Validate());
			m_SkyboxPass->Bake();
		}

		// TODO(Yan): resizeable/flushable
		const size_t TransformBufferCount = 10 * 1024; // 10240 transforms
		m_SubmeshTransformBuffers.resize(framesInFlight);
		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			m_SubmeshTransformBuffers[i].Buffer = VertexBuffer::Create(sizeof(TransformVertexData) * TransformBufferCount);
			m_SubmeshTransformBuffers[i].Data = hnew TransformVertexData[TransformBufferCount];
		}

		Renderer::Submit([instance = Ref(this)]() mutable { instance->m_ResourcesCreatedGPU = true; });

		InitOptions();

		////////////////////////////////////////
		// COMPUTE
		////////////////////////////////////////

		// GTAO
		{
			{
				ImageSpecification imageSpec;
				if (m_Options.GTAOBentNormals)
					imageSpec.Format = ImageFormat::RED32UI;
				else
					imageSpec.Format = ImageFormat::RED8UI;
				imageSpec.Usage = ImageUsage::Storage;
				imageSpec.Layers = 1;
				imageSpec.DebugName = "GTAO";
				m_GTAOOutputImage = Image2D::Create(imageSpec);
				m_GTAOOutputImage->Invalidate();
			}

			// GTAO-Edges
			{ 
				ImageSpecification imageSpec;
				imageSpec.Format = ImageFormat::RED8UN;
				imageSpec.Usage = ImageUsage::Storage;
				imageSpec.DebugName = "GTAO-Edges";
				m_GTAOEdgesOutputImage = Image2D::Create(imageSpec);
				m_GTAOEdgesOutputImage->Invalidate();
			}

			Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("GTAO");

			ComputePassSpecification spec;
			spec.DebugName = "GTAO-ComputePass";
			spec.Pipeline = PipelineCompute::Create(shader);
			m_GTAOComputePass = ComputePass::Create(spec);
			m_GTAOComputePass->SetInput("u_HiZDepth", m_HierarchicalDepthTexture.Texture);
			m_GTAOComputePass->SetInput("u_HilbertLut", Renderer::GetHilbertLut());
			m_GTAOComputePass->SetInput("u_ViewNormal", m_GeometryPass->GetOutput(1));
			m_GTAOComputePass->SetInput("o_AOwBentNormals", m_GTAOOutputImage);
			m_GTAOComputePass->SetInput("o_Edges", m_GTAOEdgesOutputImage);

			m_GTAOComputePass->SetInput("Camera", m_UBSCamera);
			m_GTAOComputePass->SetInput("ScreenData", m_UBSScreenData);
			ZONG_CORE_VERIFY(m_GTAOComputePass->Validate());
			m_GTAOComputePass->Bake();
		}

		// GTAO Denoise
		{
			{
				ImageSpecification imageSpec;
				if (m_Options.GTAOBentNormals)
					imageSpec.Format = ImageFormat::RED32UI;
				else
					imageSpec.Format = ImageFormat::RED8UI;
				imageSpec.Usage = ImageUsage::Storage;
				imageSpec.Layers = 1;
				imageSpec.DebugName = "GTAO-Denoise";
				m_GTAODenoiseImage = Image2D::Create(imageSpec);
				m_GTAODenoiseImage->Invalidate();

				Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("GTAO-Denoise");
				m_GTAODenoiseMaterial[0] = Material::Create(shader, "GTAO-Denoise-Ping");
				m_GTAODenoiseMaterial[1] = Material::Create(shader, "GTAO-Denoise-Pong");

				ComputePassSpecification spec;
				spec.DebugName = "GTAO-Denoise";
				spec.Pipeline = PipelineCompute::Create(shader);

				m_GTAODenoisePass[0] = ComputePass::Create(spec);
				m_GTAODenoisePass[0]->SetInput("u_Edges", m_GTAOEdgesOutputImage);
				m_GTAODenoisePass[0]->SetInput("u_AOTerm", m_GTAOOutputImage);
				m_GTAODenoisePass[0]->SetInput("o_AOTerm", m_GTAODenoiseImage);
				m_GTAODenoisePass[0]->SetInput("ScreenData", m_UBSScreenData);
				ZONG_CORE_VERIFY(m_GTAODenoisePass[0]->Validate());
				m_GTAODenoisePass[0]->Bake();

				m_GTAODenoisePass[1] = ComputePass::Create(spec);
				m_GTAODenoisePass[1]->SetInput("u_Edges", m_GTAOEdgesOutputImage);
				m_GTAODenoisePass[1]->SetInput("u_AOTerm", m_GTAODenoiseImage);
				m_GTAODenoisePass[1]->SetInput("o_AOTerm", m_GTAOOutputImage);
				m_GTAODenoisePass[1]->SetInput("ScreenData", m_UBSScreenData);
				ZONG_CORE_VERIFY(m_GTAODenoisePass[1]->Validate());
				m_GTAODenoisePass[1]->Bake();
			}

			// GTAO Composite
			{
				PipelineSpecification aoCompositePipelineSpec;
				aoCompositePipelineSpec.DebugName = "AO-Composite";
				FramebufferSpecification framebufferSpec;
				framebufferSpec.DebugName = "AO-Composite";
				framebufferSpec.Attachments = { ImageFormat::RGBA32F };
				framebufferSpec.ExistingImages[0] = m_GeometryPass->GetOutput(0);
				framebufferSpec.Blend = true;
				framebufferSpec.ClearColorOnLoad = false;
				framebufferSpec.BlendMode = FramebufferBlendMode::Zero_SrcColor;

				aoCompositePipelineSpec.TargetFramebuffer = Framebuffer::Create(framebufferSpec);
				aoCompositePipelineSpec.DepthTest = false;
				aoCompositePipelineSpec.Layout = {
					{ ShaderDataType::Float3, "a_Position" },
					{ ShaderDataType::Float2, "a_TexCoord" },
				};
				aoCompositePipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("AO-Composite");
				
				// Create RenderPass
				RenderPassSpecification renderPassSpec;
				renderPassSpec.DebugName = "AO-Composite";
				renderPassSpec.Pipeline = Pipeline::Create(aoCompositePipelineSpec);
				m_AOCompositePass = RenderPass::Create(renderPassSpec);
				m_AOCompositePass->SetInput("u_GTAOTex", m_GTAOOutputImage);
				ZONG_CORE_VERIFY(m_AOCompositePass->Validate());
				m_AOCompositePass->Bake();

				m_AOCompositeMaterial = Material::Create(aoCompositePipelineSpec.Shader, "GTAO-Composite");
			}
		}

		m_GTAOFinalImage = m_Options.GTAODenoisePasses && m_Options.GTAODenoisePasses % 2 != 0 ? m_GTAODenoiseImage : m_GTAOOutputImage;

		// SSR
		{
			Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("SSR");

			ComputePassSpecification spec;
			spec.DebugName = "SSR-Compute";
			spec.Pipeline = PipelineCompute::Create(shader);
			m_SSRPass = ComputePass::Create(spec);
			m_SSRPass->SetInput("outColor", m_SSRImage);
			m_SSRPass->SetInput("u_InputColor", m_PreConvolutedTexture.Texture);
			m_SSRPass->SetInput("u_VisibilityBuffer", m_PreIntegrationVisibilityTexture.Texture);
			m_SSRPass->SetInput("u_HiZBuffer", m_HierarchicalDepthTexture.Texture);
			m_SSRPass->SetInput("u_Normal", m_GeometryPass->GetOutput(1));
			m_SSRPass->SetInput("u_MetalnessRoughness", m_GeometryPass->GetOutput(2));
			m_SSRPass->SetInput("u_GTAOTex", m_GTAOFinalImage);
			m_SSRPass->SetInput("Camera", m_UBSCamera);
			m_SSRPass->SetInput("ScreenData", m_UBSScreenData);
			// TODO(Yan): HBAO texture as well maybe
			ZONG_CORE_VERIFY(m_SSRPass->Validate());
			m_SSRPass->Bake();
		}


	}

	void SceneRenderer::Shutdown()
	{
		hdelete[] m_BoneTransformsData;
		for (auto& transformBuffer : m_SubmeshTransformBuffers)
			hdelete[] transformBuffer.Data;
	}

	void SceneRenderer::InitOptions()
	{
		//TODO(Karim): Deserialization?
		if (m_Options.EnableGTAO)
			*(int*)&m_Options.ReflectionOcclusionMethod |= (int)ShaderDef::AOMethod::GTAO;

		// OVERRIDE
		m_Options.ReflectionOcclusionMethod = ShaderDef::AOMethod::None;

		// Special macros are strictly starting with "__ZONG_"
		Renderer::SetGlobalMacroInShaders("__ZONG_REFLECTION_OCCLUSION_METHOD", fmt::format("{}", (int)m_Options.ReflectionOcclusionMethod));
		//Renderer::SetGlobalMacroInShaders("__ZONG_GTAO_COMPUTE_BENT_NORMALS", fmt::format("{}", (int)m_Options.GTAOBentNormals));
	}

	void SceneRenderer::InsertGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& markerColor)
	{
		Renderer::Submit([=]
		{
			Renderer::RT_InsertGPUPerfMarker(renderCommandBuffer, label, markerColor);
		});
	}

	void SceneRenderer::BeginGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer, const std::string& label, const glm::vec4& markerColor)
	{
		Renderer::Submit([=]
		{
			Renderer::RT_BeginGPUPerfMarker(renderCommandBuffer, label, markerColor);
		});
	}

	void SceneRenderer::EndGPUPerfMarker(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		Renderer::Submit([=]
		{
			Renderer::RT_EndGPUPerfMarker(renderCommandBuffer);
		});
	}

	void SceneRenderer::SetScene(Ref<Scene> scene)
	{
		ZONG_CORE_ASSERT(!m_Active, "Can't change scenes while rendering");
		m_Scene = scene;
	}

	void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
	{
		width = (uint32_t)(width * m_Specification.Tiering.RendererScale);
		height = (uint32_t)(height * m_Specification.Tiering.RendererScale);

		if (m_ViewportWidth != width || m_ViewportHeight != height)
		{
			m_ViewportWidth = width;
			m_ViewportHeight = height;
			m_InvViewportWidth = 1.f / (float)width;
			m_InvViewportHeight = 1.f / (float)height;
			m_NeedsResize = true;
		}
	}

	// Some other settings are directly set in gui
	void SceneRenderer::UpdateGTAOData()
	{
		CBGTAOData& gtaoData = GTAODataCB;
		gtaoData.NDCToViewMul_x_PixelSize = { CameraDataUB.NDCToViewMul * (gtaoData.HalfRes ? m_ScreenDataUB.InvHalfResolution : m_ScreenDataUB.InvFullResolution) };
		gtaoData.HZBUVFactor = m_SSROptions.HZBUvFactor;
		gtaoData.ShadowTolerance = m_Options.AOShadowTolerance;
	}

	void SceneRenderer::BeginScene(const SceneRendererCamera& camera)
	{
		ZONG_PROFILE_FUNC();

		ZONG_CORE_ASSERT(m_Scene);
		ZONG_CORE_ASSERT(!m_Active);
		m_Active = true;

		if (m_ResourcesCreatedGPU)
			m_ResourcesCreated = true;

		if (!m_ResourcesCreated)
			return;

		m_GTAOFinalImage = m_Options.GTAODenoisePasses && m_Options.GTAODenoisePasses % 2 != 0 ? m_GTAODenoiseImage : m_GTAOOutputImage;
		// TODO(Yan): enable if shader uses this: m_SSRPass->SetInput("u_GTAOTex", m_GTAOFinalImage);

		m_SceneData.SceneCamera = camera;
		m_SceneData.SceneEnvironment = m_Scene->m_Environment;
		m_SceneData.SceneEnvironmentIntensity = m_Scene->m_EnvironmentIntensity;
		m_SceneData.ActiveLight = m_Scene->m_Light;
		m_SceneData.SceneLightEnvironment = m_Scene->m_LightEnvironment;
		m_SceneData.SkyboxLod = m_Scene->m_SkyboxLod;

		m_GeometryPass->SetInput("u_EnvRadianceTex", m_SceneData.SceneEnvironment->RadianceMap);
		m_GeometryPass->SetInput("u_EnvIrradianceTex", m_SceneData.SceneEnvironment->IrradianceMap);	
		
		m_GeometryAnimPass->SetInput("u_EnvRadianceTex", m_SceneData.SceneEnvironment->RadianceMap);
		m_GeometryAnimPass->SetInput("u_EnvIrradianceTex", m_SceneData.SceneEnvironment->IrradianceMap);

		if (m_NeedsResize)
		{
			m_NeedsResize = false;

			const glm::uvec2 viewportSize = { m_ViewportWidth, m_ViewportHeight };

			m_ScreenSpaceProjectionMatrix = glm::ortho(0.0f, (float)m_ViewportWidth, 0.0f, (float)m_ViewportHeight);

			m_ScreenDataUB.FullResolution = { m_ViewportWidth, m_ViewportHeight };
			m_ScreenDataUB.InvFullResolution = { m_InvViewportWidth,  m_InvViewportHeight };
			m_ScreenDataUB.HalfResolution = glm::ivec2{ m_ViewportWidth,  m_ViewportHeight } / 2;
			m_ScreenDataUB.InvHalfResolution = { m_InvViewportWidth * 2.0f,  m_InvViewportHeight * 2.0f };

			// Both Pre-depth and geometry framebuffers need to be resized first.
			// Note the _Anim variants of these pipelines share the same framebuffer
			m_PreDepthPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_PreDepthAnimPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_PreDepthTransparentPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_GeometryPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_GeometryAnimPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_SkyboxPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_SelectedGeometryPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_SelectedGeometryAnimPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_GeometryPassColorAttachmentImage->Resize({ m_ViewportWidth, m_ViewportHeight });

			// Dependent on Geometry 
			m_SSRCompositePass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);

			m_PreIntegrationVisibilityTexture.Texture->Resize(m_ViewportWidth, m_ViewportHeight);
			// TODO: m_AOCompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->Resize(m_ViewportWidth, m_ViewportHeight); //Only resize after geometry framebuffer
			m_AOCompositePass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_CompositePass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);

			m_GridRenderPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);

			if (m_JumpFloodCompositePass)
				m_JumpFloodCompositePass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);

			if (m_DOFPass)
				m_DOFPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);

			if (m_ReadBackImage)
				m_ReadBackImage->Resize({ m_ViewportWidth, m_ViewportHeight });

			// HZB
			{
				//HZB size must be power of 2's
				const glm::uvec2 numMips = glm::ceil(glm::log2(glm::vec2(viewportSize)));
				m_SSROptions.NumDepthMips = glm::max(numMips.x, numMips.y);

				const glm::uvec2 hzbSize = BIT(numMips);
				m_HierarchicalDepthTexture.Texture->Resize(hzbSize.x, hzbSize.y);

				const glm::vec2 hzbUVFactor = { (glm::vec2)viewportSize / (glm::vec2)hzbSize };
				m_SSROptions.HZBUvFactor = hzbUVFactor;

				// Image Views (per-mip)
				ImageViewSpecification imageViewSpec;
				uint32_t mipCount = m_HierarchicalDepthTexture.Texture->GetMipLevelCount();
				m_HierarchicalDepthTexture.ImageViews.resize(mipCount);
				for (uint32_t mip = 0; mip < mipCount; mip++)
				{
					imageViewSpec.DebugName = fmt::format("HierarchicalDepthTexture-{}", mip);
					imageViewSpec.Image = m_HierarchicalDepthTexture.Texture->GetImage();
					imageViewSpec.Mip = mip;
					m_HierarchicalDepthTexture.ImageViews[mip] = ImageView::Create(imageViewSpec);
				}

				CreateHZBPassMaterials();
			}

			// Pre-Integration
			{
				// Image Views (per-mip)
				ImageViewSpecification imageViewSpec;
				uint32_t mipCount = m_PreIntegrationVisibilityTexture.Texture->GetMipLevelCount();
				m_PreIntegrationVisibilityTexture.ImageViews.resize(mipCount);
				for (uint32_t mip = 0; mip < mipCount - 1; mip++)
				{
					imageViewSpec.DebugName = fmt::format("PreIntegrationVisibilityTexture-{}", mip);
					imageViewSpec.Image = m_PreIntegrationVisibilityTexture.Texture->GetImage();
					imageViewSpec.Mip = mip + 1; // Start from mip 1 not 0
					m_PreIntegrationVisibilityTexture.ImageViews[mip] = ImageView::Create(imageViewSpec);
				}

				CreatePreIntegrationPassMaterials();
			}

			// Light culling
			{
				constexpr uint32_t TILE_SIZE = 16u;
				glm::uvec2 size = viewportSize;
				size += TILE_SIZE - viewportSize % TILE_SIZE;
				m_LightCullingWorkGroups = { size / TILE_SIZE, 1 };
				RendererDataUB.TilesCountX = m_LightCullingWorkGroups.x;

				m_SBSVisiblePointLightIndicesBuffer->Resize(m_LightCullingWorkGroups.x * m_LightCullingWorkGroups.y * 4 * 1024);
				m_SBSVisibleSpotLightIndicesBuffer->Resize(m_LightCullingWorkGroups.x * m_LightCullingWorkGroups.y * 4 * 1024);
			}

			// GTAO
			{
				glm::uvec2 gtaoSize = GTAODataCB.HalfRes ? (viewportSize + 1u) / 2u : viewportSize;
				glm::uvec2 denoiseSize = gtaoSize;
				const ImageFormat gtaoImageFormat = m_Options.GTAOBentNormals ? ImageFormat::RED32UI : ImageFormat::RED8UI;
				m_GTAOOutputImage->GetSpecification().Format = gtaoImageFormat;
				m_GTAODenoiseImage->GetSpecification().Format = gtaoImageFormat;

				m_GTAOOutputImage->Resize(gtaoSize);
				m_GTAODenoiseImage->Resize(gtaoSize);
				m_GTAOEdgesOutputImage->Resize(gtaoSize);

				constexpr uint32_t WORK_GROUP_SIZE = 16u;
				gtaoSize += WORK_GROUP_SIZE - gtaoSize % WORK_GROUP_SIZE;
				m_GTAOWorkGroups.x = gtaoSize.x / WORK_GROUP_SIZE;
				m_GTAOWorkGroups.y = gtaoSize.y / WORK_GROUP_SIZE;

				constexpr uint32_t DENOISE_WORK_GROUP_SIZE = 8u;
				denoiseSize += DENOISE_WORK_GROUP_SIZE - denoiseSize % DENOISE_WORK_GROUP_SIZE;
				m_GTAODenoiseWorkGroups.x = (denoiseSize.x + 2u * DENOISE_WORK_GROUP_SIZE - 1u) / (DENOISE_WORK_GROUP_SIZE * 2u); // 2 horizontal pixels at a time.
				m_GTAODenoiseWorkGroups.y = denoiseSize.y / DENOISE_WORK_GROUP_SIZE;
			}

			//SSR
			{
				constexpr uint32_t WORK_GROUP_SIZE = 8u;
				glm::uvec2 ssrSize = m_SSROptions.HalfRes ? (viewportSize + 1u) / 2u : viewportSize;
				m_SSRImage->Resize(ssrSize);

				ssrSize += WORK_GROUP_SIZE - ssrSize % WORK_GROUP_SIZE;
				m_SSRWorkGroups.x = ssrSize.x / WORK_GROUP_SIZE;
				m_SSRWorkGroups.y = ssrSize.y / WORK_GROUP_SIZE;

				// Pre-Convolution
				m_PreConvolutedTexture.Texture->Resize(ssrSize.x, ssrSize.y);

				// Image Views (per-mip)
				ImageViewSpecification imageViewSpec;
				imageViewSpec.DebugName = fmt::format("PreConvolutionCompute");
				uint32_t mipCount = m_PreConvolutedTexture.Texture->GetMipLevelCount();
				m_PreConvolutedTexture.ImageViews.resize(mipCount);
				for (uint32_t mip = 0; mip < mipCount; mip++)
				{
					imageViewSpec.Image = m_PreConvolutedTexture.Texture->GetImage();
					imageViewSpec.Mip = mip;
					m_PreConvolutedTexture.ImageViews[mip] = ImageView::Create(imageViewSpec);
				}

				// Re-setup materials with new image views
				CreatePreConvolutionPassMaterials();
			}

			// Bloom
			{
				glm::uvec2 bloomSize = (viewportSize + 1u) / 2u;
				bloomSize += m_BloomComputeWorkgroupSize - bloomSize % m_BloomComputeWorkgroupSize;

				m_BloomComputeTextures[0].Texture->Resize(bloomSize);
				m_BloomComputeTextures[1].Texture->Resize(bloomSize);
				m_BloomComputeTextures[2].Texture->Resize(bloomSize);

				// Image Views (per-mip)
				ImageViewSpecification imageViewSpec;
				for (int i = 0; i < 3; i++)
				{
					imageViewSpec.DebugName = fmt::format("BloomCompute-{}", i);
					uint32_t mipCount = m_BloomComputeTextures[i].Texture->GetMipLevelCount();
					m_BloomComputeTextures[i].ImageViews.resize(mipCount);
					for (uint32_t mip = 0; mip < mipCount; mip++)
					{
						imageViewSpec.Image = m_BloomComputeTextures[i].Texture->GetImage();
						imageViewSpec.Mip = mip;
						m_BloomComputeTextures[i].ImageViews[mip] = ImageView::Create(imageViewSpec);
					}
				}

				// Re-setup materials with new image views
				CreateBloomPassMaterials();
			}

			for (auto& tempFB : m_TempFramebuffers)
				tempFB->Resize(m_ViewportWidth, m_ViewportHeight);

			// TODO: if (m_ExternalCompositeRenderPass)
			// TODO: 	m_ExternalCompositeRenderPass->GetSpecification().TargetFramebuffer->Resize(m_ViewportWidth, m_ViewportHeight);
		}

		// Update uniform buffers
		UBCamera& cameraData = CameraDataUB;
		UBScene& sceneData = SceneDataUB;
		UBShadow& shadowData = ShadowData;
		UBRendererData& rendererData = RendererDataUB;
		UBPointLights& pointLightData = PointLightsUB;
		UBScreenData& screenData = m_ScreenDataUB;
		UBSpotLights& spotLightData = SpotLightUB;
		UBSpotShadowData& spotShadowData = SpotShadowDataUB;

		auto& sceneCamera = m_SceneData.SceneCamera;
		const auto viewProjection = sceneCamera.Camera.GetProjectionMatrix() * sceneCamera.ViewMatrix;
		const glm::mat4 viewInverse = glm::inverse(sceneCamera.ViewMatrix);
		const glm::mat4 projectionInverse = glm::inverse(sceneCamera.Camera.GetProjectionMatrix());
		const glm::vec3 cameraPosition = viewInverse[3];

		cameraData.ViewProjection = viewProjection;
		cameraData.Projection = sceneCamera.Camera.GetProjectionMatrix();
		cameraData.InverseProjection = projectionInverse;
		cameraData.View = sceneCamera.ViewMatrix;
		cameraData.InverseView = viewInverse;
		cameraData.InverseViewProjection = viewInverse * cameraData.InverseProjection;

		float depthLinearizeMul = (-cameraData.Projection[3][2]);     // float depthLinearizeMul = ( clipFar * clipNear ) / ( clipFar - clipNear );
		float depthLinearizeAdd = (cameraData.Projection[2][2]);     // float depthLinearizeAdd = clipFar / ( clipFar - clipNear );
		// correct the handedness issue.
		if (depthLinearizeMul * depthLinearizeAdd < 0)
			depthLinearizeAdd = -depthLinearizeAdd;
		cameraData.DepthUnpackConsts = { depthLinearizeMul, depthLinearizeAdd };
		const float* P = glm::value_ptr(m_SceneData.SceneCamera.Camera.GetProjectionMatrix());
		const glm::vec4 projInfoPerspective = {
				 2.0f / (P[4 * 0 + 0]),                  // (x) * (R - L)/N
				 2.0f / (P[4 * 1 + 1]),                  // (y) * (T - B)/N
				-(1.0f - P[4 * 2 + 0]) / P[4 * 0 + 0],  // L/N
				-(1.0f + P[4 * 2 + 1]) / P[4 * 1 + 1],  // B/N
		};
		float tanHalfFOVY = 1.0f / cameraData.Projection[1][1];    // = tanf( drawContext.Camera.GetYFOV( ) * 0.5f );
		float tanHalfFOVX = 1.0f / cameraData.Projection[0][0];    // = tanHalfFOVY * drawContext.Camera.GetAspect( );
		cameraData.CameraTanHalfFOV = { tanHalfFOVX, tanHalfFOVY };
		cameraData.NDCToViewMul = { projInfoPerspective[0], projInfoPerspective[1] };
		cameraData.NDCToViewAdd = { projInfoPerspective[2], projInfoPerspective[3] };

		Ref<SceneRenderer> instance = this;
		Renderer::Submit([instance, cameraData]() mutable
		{
			instance->m_UBSCamera->RT_Get()->RT_SetData(&cameraData, sizeof(cameraData));
		});

		const auto lightEnvironment = m_SceneData.SceneLightEnvironment;
		const std::vector<PointLight>& pointLightsVec = lightEnvironment.PointLights;
		pointLightData.Count = int(pointLightsVec.size());
		std::memcpy(pointLightData.PointLights, pointLightsVec.data(), lightEnvironment.GetPointLightsSize()); //(Karim) Do we really have to copy that?
		Renderer::Submit([instance, &pointLightData]() mutable
		{
			Ref<UniformBuffer> uniformBuffer = instance->m_UBSPointLights->RT_Get();
			uniformBuffer->RT_SetData(&pointLightData, 16ull + sizeof(PointLight) * pointLightData.Count);
		});

		const std::vector<SpotLight>& spotLightsVec = lightEnvironment.SpotLights;
		spotLightData.Count = int(spotLightsVec.size());
		std::memcpy(spotLightData.SpotLights, spotLightsVec.data(), lightEnvironment.GetSpotLightsSize()); //(Karim) Do we really have to copy that?
		Renderer::Submit([instance, &spotLightData]() mutable
		{
			Ref<UniformBuffer> uniformBuffer = instance->m_UBSSpotLights->RT_Get();
			uniformBuffer->RT_SetData(&spotLightData, 16ull + sizeof(SpotLight) * spotLightData.Count);
		});

		for (size_t i = 0; i < spotLightsVec.size(); ++i)
		{
			auto& light = spotLightsVec[i];
			if (!light.CastsShadows)
				continue;

			glm::mat4 projection = glm::perspective(glm::radians(light.Angle), 1.f, 0.1f, light.Range);
			// NOTE(Yan): ShadowMatrices[0] because we only support ONE shadow casting spot light at the moment and it MUST be index 0
			spotShadowData.ShadowMatrices[0] = projection * glm::lookAt(light.Position,  light.Position - light.Direction, glm::vec3(0.0f, 1.0f, 0.0f));
		}

		Renderer::Submit([instance, spotShadowData, spotLightsVec]() mutable
		{
			Ref<UniformBuffer> uniformBuffer = instance->m_UBSSpotShadowData->RT_Get();
			uniformBuffer->RT_SetData(&spotShadowData, (uint32_t)(sizeof(glm::mat4) * spotLightsVec.size()));
		});

		const auto& directionalLight = m_SceneData.SceneLightEnvironment.DirectionalLights[0];
		sceneData.Lights.Direction = directionalLight.Direction;
		sceneData.Lights.Radiance = directionalLight.Radiance;
		sceneData.Lights.Intensity = directionalLight.Intensity;
		sceneData.Lights.ShadowAmount = directionalLight.ShadowAmount;

		sceneData.CameraPosition = cameraPosition;
		sceneData.EnvironmentMapIntensity = m_SceneData.SceneEnvironmentIntensity;
		Renderer::Submit([instance, sceneData]() mutable
		{
			instance->m_UBSScene->RT_Get()->RT_SetData(&sceneData, sizeof(sceneData));
		});

		if (m_Options.EnableGTAO)
			UpdateGTAOData();

		Renderer::Submit([instance, screenData]() mutable
		{
			instance->m_UBSScreenData->RT_Get()->RT_SetData(&screenData, sizeof(screenData));
		});

		CascadeData cascades[4];
		if (m_UseManualCascadeSplits)
			CalculateCascadesManualSplit(cascades, sceneCamera, directionalLight.Direction);
		else
			CalculateCascades(cascades, sceneCamera, directionalLight.Direction);


		// TODO: four cascades for now
		for (int i = 0; i < 4; i++)
		{
			CascadeSplits[i] = cascades[i].SplitDepth;
			shadowData.ViewProjection[i] = cascades[i].ViewProj;
		}
		Renderer::Submit([instance, shadowData]() mutable
		{
			instance->m_UBSShadow->RT_Get()->RT_SetData(&shadowData, sizeof(shadowData));
		});

		rendererData.CascadeSplits = CascadeSplits;
		Renderer::Submit([instance, rendererData]() mutable
		{
			instance->m_UBSRendererData->RT_Get()->RT_SetData(&rendererData, sizeof(rendererData));
		});

		// NOTE(Yan): shouldn't be necessary anymore with new Render Passes
		// Renderer::SetSceneEnvironment(this, m_SceneData.SceneEnvironment,
		// 	m_ShadowPassPipelines[0]->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetDepthImage(),
		// 	m_SpotShadowPassPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetDepthImage());
	}

	void SceneRenderer::EndScene()
	{
		ZONG_PROFILE_FUNC();

		ZONG_CORE_ASSERT(m_Active);
#if MULTI_THREAD
		Ref<SceneRenderer> instance = this;
		s_ThreadPool.emplace_back(([instance]() mutable
		{
			instance->FlushDrawList();
		}));
#else 
		FlushDrawList();
#endif

		m_Active = false;
	}

	void SceneRenderer::WaitForThreads()
	{
		for (uint32_t i = 0; i < s_ThreadPool.size(); i++)
			s_ThreadPool[i].join();

		s_ThreadPool.clear();
	}

	void SceneRenderer::SubmitMesh(Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable, const glm::mat4& transform, const std::vector<glm::mat4>& boneTransforms, Ref<Material> overrideMaterial)
	{
		ZONG_PROFILE_FUNC();

		// TODO: Culling, sorting, etc.

		const auto meshSource = mesh->GetMeshSource();
		const auto& submeshes = meshSource->GetSubmeshes();
		const auto& submesh = submeshes[submeshIndex];
		uint32_t materialIndex = submesh.MaterialIndex;
		bool isRigged = submesh.IsRigged;

		AssetHandle materialHandle = materialTable->HasMaterial(materialIndex) ? materialTable->GetMaterial(materialIndex) : mesh->GetMaterials()->GetMaterial(materialIndex);
		Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(materialHandle);

		MeshKey meshKey = { mesh->Handle, materialHandle, submeshIndex, false };
		auto& transformStorage = m_MeshTransformMap[meshKey].Transforms.emplace_back();

		transformStorage.MRow[0] = { transform[0][0], transform[1][0], transform[2][0], transform[3][0] };
		transformStorage.MRow[1] = { transform[0][1], transform[1][1], transform[2][1], transform[3][1] };
		transformStorage.MRow[2] = { transform[0][2], transform[1][2], transform[2][2], transform[3][2] };

		if (isRigged)
		{
			CopyToBoneTransformStorage(meshKey, meshSource, boneTransforms);
		}
		// Main geo
		{
			bool isTransparent = material->IsTransparent();
			auto& destDrawList = !isTransparent ? m_DrawList : m_TransparentDrawList;
			auto& dc = destDrawList[meshKey];
			dc.Mesh = mesh;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialTable = materialTable;
			dc.OverrideMaterial = overrideMaterial;
			dc.InstanceCount++;
			dc.IsRigged = isRigged;  // TODO: would it be better to have separate draw list for rigged meshes, or this flag is OK?
		}

		// Shadow pass
		if (material->IsShadowCasting())
		{
			auto& dc = m_ShadowPassDrawList[meshKey];
			dc.Mesh = mesh;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialTable = materialTable;
			dc.OverrideMaterial = overrideMaterial;
			dc.InstanceCount++;
			dc.IsRigged = isRigged;
		}
	}

	void SceneRenderer::SubmitStaticMesh(Ref<StaticMesh> staticMesh, Ref<MaterialTable> materialTable, const glm::mat4& transform, Ref<Material> overrideMaterial)
	{
		ZONG_PROFILE_FUNC();

		Ref<MeshSource> meshSource = staticMesh->GetMeshSource();
		const auto& submeshData = meshSource->GetSubmeshes();
		for (uint32_t submeshIndex : staticMesh->GetSubmeshes())
		{
			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].Transform;

			const auto& submeshes = staticMesh->GetMeshSource()->GetSubmeshes();
			uint32_t materialIndex = submeshes[submeshIndex].MaterialIndex;

			AssetHandle materialHandle = materialTable->HasMaterial(materialIndex) ? materialTable->GetMaterial(materialIndex) : staticMesh->GetMaterials()->GetMaterial(materialIndex);
			ZONG_CORE_VERIFY(materialHandle);
			Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(materialHandle);

			MeshKey meshKey = { staticMesh->Handle, materialHandle, submeshIndex, false };
			auto& transformStorage = m_MeshTransformMap[meshKey].Transforms.emplace_back();

			transformStorage.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformStorage.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformStorage.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };


			// Main geo
			{
				bool isTransparent = material->IsTransparent();
				auto& destDrawList = !isTransparent ? m_StaticMeshDrawList : m_TransparentStaticMeshDrawList;
				auto& dc = destDrawList[meshKey];
				dc.StaticMesh = staticMesh;
				dc.SubmeshIndex = submeshIndex;
				dc.MaterialTable = materialTable;
				dc.OverrideMaterial = overrideMaterial;
				dc.InstanceCount++;
			}

			// Shadow pass
			if (material->IsShadowCasting())
			{
				auto& dc = m_StaticMeshShadowPassDrawList[meshKey];
				dc.StaticMesh = staticMesh;
				dc.SubmeshIndex = submeshIndex;
				dc.MaterialTable = materialTable;
				dc.OverrideMaterial = overrideMaterial;
				dc.InstanceCount++;
			}
		}

	}

	void SceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable, const glm::mat4& transform, const std::vector<glm::mat4>& boneTransforms, Ref<Material> overrideMaterial)
	{
		ZONG_PROFILE_FUNC();

		// TODO: Culling, sorting, etc.

		const auto meshSource = mesh->GetMeshSource();
		const auto& submeshes = meshSource->GetSubmeshes();
		const auto& submesh = submeshes[submeshIndex];
		uint32_t materialIndex = submesh.MaterialIndex;
		bool isRigged = submesh.IsRigged;

		AssetHandle materialHandle = materialTable->HasMaterial(materialIndex) ? materialTable->GetMaterial(materialIndex) : mesh->GetMaterials()->GetMaterial(materialIndex);
		ZONG_CORE_VERIFY(materialHandle);
		Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(materialHandle);

		MeshKey meshKey = { mesh->Handle, materialHandle, submeshIndex, true };
		auto& transformStorage = m_MeshTransformMap[meshKey].Transforms.emplace_back();

		transformStorage.MRow[0] = { transform[0][0], transform[1][0], transform[2][0], transform[3][0] };
		transformStorage.MRow[1] = { transform[0][1], transform[1][1], transform[2][1], transform[3][1] };
		transformStorage.MRow[2] = { transform[0][2], transform[1][2], transform[2][2], transform[3][2] };

		if (isRigged)
		{
			CopyToBoneTransformStorage(meshKey, meshSource, boneTransforms);
		}

		uint32_t instanceIndex = 0;

		// Main geo
		{
			bool isTransparent = material->IsTransparent();
			auto& destDrawList = !isTransparent ? m_DrawList : m_TransparentDrawList;
			auto& dc = destDrawList[meshKey];
			dc.Mesh = mesh;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialTable = materialTable;
			dc.OverrideMaterial = overrideMaterial;

			instanceIndex = dc.InstanceCount;
			dc.InstanceCount++;
			dc.IsRigged = isRigged;
		}

		// Selected mesh list
		{
			auto& dc = m_SelectedMeshDrawList[meshKey];
			dc.Mesh = mesh;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialTable = materialTable;
			dc.OverrideMaterial = overrideMaterial;
			dc.InstanceCount++;
			dc.InstanceOffset = instanceIndex;
			dc.IsRigged = isRigged;
		}

		// Shadow pass
		if (material->IsShadowCasting())
		{
			auto& dc = m_ShadowPassDrawList[meshKey];
			dc.Mesh = mesh;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialTable = materialTable;
			dc.OverrideMaterial = overrideMaterial;
			dc.InstanceCount++;
			dc.IsRigged = isRigged;
		}
	}

	void SceneRenderer::SubmitSelectedStaticMesh(Ref<StaticMesh> staticMesh, Ref<MaterialTable> materialTable, const glm::mat4& transform, Ref<Material> overrideMaterial)
	{
		ZONG_PROFILE_FUNC();

		Ref<MeshSource> meshSource = staticMesh->GetMeshSource();
		const auto& submeshData = meshSource->GetSubmeshes();
		for (uint32_t submeshIndex : staticMesh->GetSubmeshes())
		{
			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].Transform;

			const auto& submeshes = staticMesh->GetMeshSource()->GetSubmeshes();
			uint32_t materialIndex = submeshes[submeshIndex].MaterialIndex;

			AssetHandle materialHandle = materialTable->HasMaterial(materialIndex) ? materialTable->GetMaterial(materialIndex) : staticMesh->GetMaterials()->GetMaterial(materialIndex);
			ZONG_CORE_VERIFY(materialHandle);
			Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(materialHandle);

			MeshKey meshKey = { staticMesh->Handle, materialHandle, submeshIndex, true };
			auto& transformStorage = m_MeshTransformMap[meshKey].Transforms.emplace_back();

			transformStorage.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformStorage.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformStorage.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };

			// Main geo
			{
				bool isTransparent = material->IsTransparent();
				auto& destDrawList = !isTransparent ? m_StaticMeshDrawList : m_TransparentStaticMeshDrawList;
				auto& dc = destDrawList[meshKey];
				dc.StaticMesh = staticMesh;
				dc.SubmeshIndex = submeshIndex;
				dc.MaterialTable = materialTable;
				dc.OverrideMaterial = overrideMaterial;
				dc.InstanceCount++;
			}

			// Selected mesh list
			{
				auto& dc = m_SelectedStaticMeshDrawList[meshKey];
				dc.StaticMesh = staticMesh;
				dc.SubmeshIndex = submeshIndex;
				dc.MaterialTable = materialTable;
				dc.OverrideMaterial = overrideMaterial;
				dc.InstanceCount++;
			}

			// Shadow pass
			if (material->IsShadowCasting())
			{
				auto& dc = m_StaticMeshShadowPassDrawList[meshKey];
				dc.StaticMesh = staticMesh;
				dc.SubmeshIndex = submeshIndex;
				dc.MaterialTable = materialTable;
				dc.OverrideMaterial = overrideMaterial;
				dc.InstanceCount++;
			}
		}
	}

	void SceneRenderer::SubmitPhysicsDebugMesh(Ref<Mesh> mesh, uint32_t submeshIndex, const glm::mat4& transform)
	{
		ZONG_CORE_VERIFY(mesh->Handle);

		Ref<MeshSource> meshSource = mesh->GetMeshSource();
		const auto& submeshData = meshSource->GetSubmeshes();
		glm::mat4 submeshTransform = transform * submeshData[submeshIndex].Transform;

		MeshKey meshKey = { mesh->Handle, 5, submeshIndex, false };
		auto& transformStorage = m_MeshTransformMap[meshKey].Transforms.emplace_back();

		transformStorage.MRow[0] = { transform[0][0], transform[1][0], transform[2][0], transform[3][0] };
		transformStorage.MRow[1] = { transform[0][1], transform[1][1], transform[2][1], transform[3][1] };
		transformStorage.MRow[2] = { transform[0][2], transform[1][2], transform[2][2], transform[3][2] };

		{
			auto& dc = m_ColliderDrawList[meshKey];
			dc.Mesh = mesh;
			dc.SubmeshIndex = submeshIndex;
			dc.InstanceCount++;
		}
	}

	void SceneRenderer::SubmitPhysicsStaticDebugMesh(Ref<StaticMesh> staticMesh, const glm::mat4& transform, const bool isPrimitiveCollider)
	{
		ZONG_CORE_VERIFY(staticMesh->Handle);
		Ref<MeshSource> meshSource = staticMesh->GetMeshSource();
		const auto& submeshData = meshSource->GetSubmeshes();
		for (uint32_t submeshIndex : staticMesh->GetSubmeshes())
		{
			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].Transform;

			MeshKey meshKey = { staticMesh->Handle, 5, submeshIndex, false };
			auto& transformStorage = m_MeshTransformMap[meshKey].Transforms.emplace_back();

			transformStorage.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformStorage.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformStorage.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };

			{
				auto& dc = m_StaticColliderDrawList[meshKey];
				dc.StaticMesh = staticMesh;
				dc.SubmeshIndex = submeshIndex;
				dc.OverrideMaterial = isPrimitiveCollider ? m_SimpleColliderMaterial : m_ComplexColliderMaterial;
				dc.InstanceCount++;
			}

		}
	}

	void SceneRenderer::ClearPass(Ref<RenderPass> renderPass, bool explicitClear)
	{
		ZONG_PROFILE_FUNC();
		Renderer::BeginRenderPass(m_CommandBuffer, renderPass, explicitClear);
		Renderer::EndRenderPass(m_CommandBuffer);
	}

	void SceneRenderer::ShadowMapPass()
	{
		ZONG_PROFILE_FUNC();

		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		m_GPUTimeQueries.DirShadowMapPassQuery = m_CommandBuffer->BeginTimestampQuery();

		auto& directionalLights = m_SceneData.SceneLightEnvironment.DirectionalLights;
		if (directionalLights[0].Intensity == 0.0f || !directionalLights[0].CastShadows)
		{
			// Clear shadow maps
			for (uint32_t i = 0; i < m_Specification.NumShadowCascades; i++)
				ClearPass(m_DirectionalShadowMapPass[i]);

			return;
		}

		for (uint32_t i = 0; i < m_Specification.NumShadowCascades; i++)
		{
			Renderer::BeginRenderPass(m_CommandBuffer, m_DirectionalShadowMapPass[i]);

			// Render entities
			const Buffer cascade(&i, sizeof(uint32_t));
			for (auto& [mk, dc] : m_StaticMeshShadowPassDrawList)
			{
				ZONG_CORE_VERIFY(m_MeshTransformMap.find(mk) != m_MeshTransformMap.end());
				const auto& transformData = m_MeshTransformMap.at(mk);
				Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelines[i], dc.StaticMesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount, m_ShadowPassMaterial, cascade);
			}
			for (auto& [mk, dc] : m_ShadowPassDrawList)
			{
				ZONG_CORE_VERIFY(m_MeshTransformMap.find(mk) != m_MeshTransformMap.end());
				const auto& transformData = m_MeshTransformMap.at(mk);
				if (!dc.IsRigged)
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelines[i], dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, dc.InstanceCount, m_ShadowPassMaterial, cascade);
			}

			Renderer::EndRenderPass(m_CommandBuffer);
		}

		for (uint32_t i = 0; i < m_Specification.NumShadowCascades; i++)
		{
			Renderer::BeginRenderPass(m_CommandBuffer, m_DirectionalShadowMapAnimPass[i]);

			// Render entities
			const Buffer cascade(&i, sizeof(uint32_t));
			for (auto& [mk, dc] : m_ShadowPassDrawList)
			{
				ZONG_CORE_VERIFY(m_MeshTransformMap.find(mk) != m_MeshTransformMap.end());
				const auto& transformData = m_MeshTransformMap.at(mk);
				if (dc.IsRigged)
				{
					const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelinesAnim[i], dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, boneTransformsData.BoneTransformsBaseIndex, dc.InstanceCount, m_ShadowPassMaterial, cascade);
				}
			}

			Renderer::EndRenderPass(m_CommandBuffer);
		}

		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.DirShadowMapPassQuery);
	}

	void SceneRenderer::SpotShadowMapPass()
	{
		ZONG_PROFILE_FUNC();

		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		m_GPUTimeQueries.SpotShadowMapPassQuery = m_CommandBuffer->BeginTimestampQuery();

		// Spot shadow maps
		Renderer::BeginRenderPass(m_CommandBuffer, m_SpotShadowPass);
		for (uint32_t i = 0; i < 1; i++)
		{

			const Buffer lightIndex(&i, sizeof(uint32_t));
			for (auto& [mk, dc] : m_StaticMeshShadowPassDrawList)
			{
				ZONG_CORE_VERIFY(m_MeshTransformMap.find(mk) != m_MeshTransformMap.end());
				const auto& transformData = m_MeshTransformMap.at(mk);
				Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_SpotShadowPassPipeline, dc.StaticMesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount, m_SpotShadowPassMaterial, lightIndex);
			}
			for (auto& [mk, dc] : m_ShadowPassDrawList)
			{
				ZONG_CORE_VERIFY(m_MeshTransformMap.find(mk) != m_MeshTransformMap.end());
				const auto& transformData = m_MeshTransformMap.at(mk);
				if (dc.IsRigged)
				{
					const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_SpotShadowPassAnimPipeline, dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset,boneTransformsData.BoneTransformsBaseIndex, dc.InstanceCount, m_SpotShadowPassMaterial, lightIndex);
				}
				else
				{
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_SpotShadowPassPipeline, dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, dc.InstanceCount, m_SpotShadowPassMaterial, lightIndex);
				}
			}

		}
		Renderer::EndRenderPass(m_CommandBuffer);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.SpotShadowMapPassQuery);
	}

	void SceneRenderer::PreDepthPass()
	{
		ZONG_PROFILE_FUNC();

		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		m_GPUTimeQueries.DepthPrePassQuery = m_CommandBuffer->BeginTimestampQuery();
		SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "PreDepthPass");
		Renderer::BeginRenderPass(m_CommandBuffer, m_PreDepthPass);
		for (auto& [mk, dc] : m_StaticMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_PreDepthPipeline, dc.StaticMesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount, m_PreDepthMaterial);
		}
		for (auto& [mk, dc] : m_DrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (!dc.IsRigged)
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_PreDepthPipeline, dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, dc.InstanceCount, m_PreDepthMaterial);
		}
		
		Renderer::EndRenderPass(m_CommandBuffer);

		Renderer::BeginRenderPass(m_CommandBuffer, m_PreDepthAnimPass);
		for (auto& [mk, dc] : m_DrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (dc.IsRigged)
			{
				const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_PreDepthPipelineAnim, dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, boneTransformsData.BoneTransformsBaseIndex, dc.InstanceCount, m_PreDepthMaterial);
			}
		}

		Renderer::EndRenderPass(m_CommandBuffer);

#if 0
		Renderer::BeginRenderPass(m_CommandBuffer, m_PreDepthTransparentPass);
		for (auto& [mk, dc] : m_TransparentStaticMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_PreDepthTransparentPipeline, m_UniformBufferSet, nullptr, dc.StaticMesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, {}, 0, dc.InstanceCount, m_PreDepthMaterial);
		}
		for (auto& [mk, dc] : m_TransparentDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (dc.IsRigged)
			{
				const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
				
				// TODO(0x): This needs to be pre-depth transparent-anim pipeline
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_PreDepthPipelineAnim, m_UniformBufferSet, nullptr, dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, m_BoneTransformStorageBuffers, boneTransformsData.BoneTransformsBaseIndex, dc.InstanceCount, m_PreDepthMaterial);
			}
			else
			{
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_PreDepthTransparentPipeline, m_UniformBufferSet, nullptr, dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, {}, 0, dc.InstanceCount, m_PreDepthMaterial);
			}
		}

		Renderer::EndRenderPass(m_CommandBuffer);
#endif
		SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);

#if 0 // Is this necessary?
		Renderer::Submit([cb = m_CommandBuffer, image = m_PreDepthPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetDepthImage().As<VulkanImage2D>()]()
		{
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			imageMemoryBarrier.image = image->GetImageInfo().Image;
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, image->GetSpecification().Mips, 0, 1 };
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				cb.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(Renderer::GetCurrentFrameIndex()),
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
	});
#endif

		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.DepthPrePassQuery);
}

	void SceneRenderer::HZBCompute()
	{
		ZONG_PROFILE_FUNC();

		m_GPUTimeQueries.HierarchicalDepthQuery = m_CommandBuffer->BeginTimestampQuery();

		constexpr uint32_t maxMipBatchSize = 4;
		const uint32_t hzbMipCount = m_HierarchicalDepthTexture.Texture->GetMipLevelCount();

		Renderer::BeginGPUPerfMarker(m_CommandBuffer, "HZB");
		Renderer::BeginComputePass(m_CommandBuffer, m_HierarchicalDepthPass);

		auto ReduceHZB = [commandBuffer = m_CommandBuffer, hierarchicalDepthPass = m_HierarchicalDepthPass, hierarchicalDepthTexture = m_HierarchicalDepthTexture.Texture, hzbMaterials = m_HZBMaterials]
		(const uint32_t startDestMip, const uint32_t parentMip, const glm::vec2& DispatchThreadIdToBufferUV, const glm::vec2& InputViewportMaxBound, const bool isFirstPass)
		{
			struct HierarchicalZComputePushConstants
			{
				glm::vec2 DispatchThreadIdToBufferUV;
				glm::vec2 InputViewportMaxBound;
				glm::vec2 InvSize;
				int FirstLod;
				bool IsFirstPass;
				char Padding[3]{ 0, 0, 0 };
			} hierarchicalZComputePushConstants;

			hierarchicalZComputePushConstants.IsFirstPass = isFirstPass;
			hierarchicalZComputePushConstants.FirstLod = (int)startDestMip;
			hierarchicalZComputePushConstants.DispatchThreadIdToBufferUV = DispatchThreadIdToBufferUV;
			hierarchicalZComputePushConstants.InputViewportMaxBound = InputViewportMaxBound;

			const glm::ivec2 srcSize(Math::DivideAndRoundUp(hierarchicalDepthTexture->GetSize(), 1u << parentMip));
			const glm::ivec2 dstSize(Math::DivideAndRoundUp(hierarchicalDepthTexture->GetSize(), 1u << startDestMip));
			hierarchicalZComputePushConstants.InvSize = glm::vec2{ 1.0f / (float)srcSize.x, 1.0f / (float)srcSize.y };

			glm::uvec3 workGroups(Math::DivideAndRoundUp(dstSize.x, 8), Math::DivideAndRoundUp(dstSize.y, 8), 1);
			Renderer::DispatchCompute(commandBuffer, hierarchicalDepthPass, hzbMaterials[startDestMip / 4], workGroups, Buffer(&hierarchicalZComputePushConstants, sizeof(hierarchicalZComputePushConstants)));
		};

		Renderer::BeginGPUPerfMarker(m_CommandBuffer, "HZB-FirstPass");

		// Reduce first 4 mips
		glm::ivec2 srcSize = m_PreDepthPass->GetDepthOutput()->GetSize();
		ReduceHZB(0, 0, { 1.0f / glm::vec2{ srcSize } }, { (glm::vec2{ srcSize } - 0.5f) / glm::vec2{ srcSize } }, true);
		Renderer::EndGPUPerfMarker(m_CommandBuffer);
		
		// Reduce the next mips
		for (uint32_t startDestMip = maxMipBatchSize; startDestMip < hzbMipCount; startDestMip += maxMipBatchSize)
		{
			Renderer::BeginGPUPerfMarker(m_CommandBuffer, fmt::format("HZB-Pass-({})", startDestMip));
			srcSize = Math::DivideAndRoundUp(m_HierarchicalDepthTexture.Texture->GetSize(), 1u << uint32_t(startDestMip - 1));
			ReduceHZB(startDestMip, startDestMip - 1, { 2.0f / glm::vec2{ srcSize } }, glm::vec2{ 1.0f }, false);
			Renderer::EndGPUPerfMarker(m_CommandBuffer);
		}

		Renderer::EndGPUPerfMarker(m_CommandBuffer);

		Renderer::EndComputePass(m_CommandBuffer, m_HierarchicalDepthPass);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.HierarchicalDepthQuery);
	}

	void SceneRenderer::PreIntegration()
	{
		ZONG_PROFILE_FUNC();

		m_GPUTimeQueries.PreIntegrationQuery = m_CommandBuffer->BeginTimestampQuery();
		glm::vec2 projectionParams = { m_SceneData.SceneCamera.Far, m_SceneData.SceneCamera.Near }; // Reversed 

		Ref<Texture2D> visibilityTexture = m_PreIntegrationVisibilityTexture.Texture;

		ImageClearValue clearValue = { glm::vec4(1.0f) };
		ImageSubresourceRange subresourceRange{};
		subresourceRange.BaseMip = 0;
		subresourceRange.MipCount = 1;
		Renderer::ClearImage(m_CommandBuffer, visibilityTexture->GetImage(), clearValue, subresourceRange);

		struct PreIntegrationComputePushConstants
		{
			glm::vec2 HZBResFactor;
			glm::vec2 ResFactor;
			glm::vec2 ProjectionParams; //(x) = Near plane, (y) = Far plane
			int PrevLod = 0;
		} preIntegrationComputePushConstants;

		Renderer::BeginGPUPerfMarker(m_CommandBuffer, "PreIntegration");

		Renderer::BeginComputePass(m_CommandBuffer, m_PreIntegrationPass);

		for (uint32_t mip = 1; mip < visibilityTexture->GetMipLevelCount(); mip++)
		{
			Renderer::BeginGPUPerfMarker(m_CommandBuffer, fmt::format("PreIntegration-Pass({})", mip));
			auto [mipWidth, mipHeight] = visibilityTexture->GetMipSize(mip);
			glm::uvec3 workGroups = { (uint32_t)glm::ceil((float)mipWidth / 8.0f), (uint32_t)glm::ceil((float)mipHeight / 8.0f), 1 };

			auto [width, height] = visibilityTexture->GetMipSize(mip);
			glm::vec2 resFactor = 1.0f / glm::vec2{ width, height };
			preIntegrationComputePushConstants.HZBResFactor = resFactor * m_SSROptions.HZBUvFactor;
			preIntegrationComputePushConstants.ResFactor = resFactor;
			preIntegrationComputePushConstants.ProjectionParams = projectionParams;
			preIntegrationComputePushConstants.PrevLod = (int)mip - 1;

			Buffer pushConstants(&preIntegrationComputePushConstants, sizeof(PreIntegrationComputePushConstants));
			Renderer::DispatchCompute(m_CommandBuffer, m_PreIntegrationPass, m_PreIntegrationMaterials[mip - 1], workGroups, pushConstants);

			Renderer::EndGPUPerfMarker(m_CommandBuffer);
		}
		Renderer::EndComputePass(m_CommandBuffer, m_PreIntegrationPass);
		Renderer::EndGPUPerfMarker(m_CommandBuffer);

		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.PreIntegrationQuery);
	}

	void SceneRenderer::LightCullingPass()
	{
		m_GPUTimeQueries.LightCullingPassQuery = m_CommandBuffer->BeginTimestampQuery();
		SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "LightCulling", { 1.0f, 1.0f, 1.0f, 1.0f });
		
		Renderer::BeginComputePass(m_CommandBuffer, m_LightCullingPass);
		Renderer::DispatchCompute(m_CommandBuffer, m_LightCullingPass, m_LightCullingMaterial, m_LightCullingWorkGroups);
		Renderer::EndComputePass(m_CommandBuffer, m_LightCullingPass);

		// NOTE(Yan): ideally this would be done automatically by RenderPass/ComputePass system
		Ref<PipelineCompute> pipeline = m_LightCullingPass->GetPipeline();
		pipeline->BufferMemoryBarrier(m_CommandBuffer, m_SBSVisiblePointLightIndicesBuffer->Get(),
			PipelineStage::ComputeShader, ResourceAccessFlags::ShaderWrite,
			PipelineStage::FragmentShader, ResourceAccessFlags::ShaderRead);
		pipeline->BufferMemoryBarrier(m_CommandBuffer, m_SBSVisibleSpotLightIndicesBuffer->Get(),
			PipelineStage::ComputeShader, ResourceAccessFlags::ShaderWrite,
			PipelineStage::FragmentShader, ResourceAccessFlags::ShaderRead);

		SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.LightCullingPassQuery);
	}

	void SceneRenderer::SkyboxPass()
	{
		ZONG_PROFILE_FUNC();

		Renderer::BeginRenderPass(m_CommandBuffer, m_SkyboxPass);

		// Skybox
		m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SceneData.SkyboxLod);
		m_SkyboxMaterial->Set("u_Uniforms.Intensity", m_SceneData.SceneEnvironmentIntensity);

		const Ref<TextureCube> radianceMap = m_SceneData.SceneEnvironment ? m_SceneData.SceneEnvironment->RadianceMap : Renderer::GetBlackCubeTexture();
		m_SkyboxMaterial->Set("u_Texture", radianceMap);

		SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Skybox", { 0.3f, 0.0f, 1.0f, 1.0f });
		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_SkyboxPipeline, m_SkyboxMaterial);
		SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
		Renderer::EndRenderPass(m_CommandBuffer);
	}

	void SceneRenderer::GeometryPass()
	{
		ZONG_PROFILE_FUNC();

		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		m_GPUTimeQueries.GeometryPassQuery = m_CommandBuffer->BeginTimestampQuery();

		Renderer::BeginRenderPass(m_CommandBuffer, m_SelectedGeometryPass);
		for (auto& [mk, dc] : m_SelectedStaticMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_SelectedGeometryPass->GetSpecification().Pipeline, dc.StaticMesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset + dc.InstanceOffset * sizeof(TransformVertexData), dc.InstanceCount, m_SelectedGeometryMaterial);
		}
		for (auto& [mk, dc] : m_SelectedMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (!dc.IsRigged)
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_SelectedGeometryPass->GetPipeline(), dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset + dc.InstanceOffset * sizeof(TransformVertexData), 0, dc.InstanceCount, m_SelectedGeometryMaterial);
		}
		Renderer::EndRenderPass(m_CommandBuffer);

		Renderer::BeginRenderPass(m_CommandBuffer, m_SelectedGeometryAnimPass);
		for (auto& [mk, dc] : m_SelectedMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (dc.IsRigged)
			{
				const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_SelectedGeometryAnimPass->GetPipeline(), dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset + dc.InstanceOffset * sizeof(TransformVertexData), boneTransformsData.BoneTransformsBaseIndex + dc.InstanceOffset, dc.InstanceCount, m_SelectedGeometryMaterial);
			}
		}
		Renderer::EndRenderPass(m_CommandBuffer);

		Renderer::BeginRenderPass(m_CommandBuffer, m_GeometryPass);
		// Render static meshes
		SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Static Meshes");
		for (auto& [mk, dc] : m_StaticMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			Renderer::RenderStaticMesh(m_CommandBuffer, m_GeometryPipeline, dc.StaticMesh, dc.SubmeshIndex, dc.MaterialTable ? dc.MaterialTable : dc.StaticMesh->GetMaterials(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount);
		}
		SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);

		// Render dynamic meshes
		SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Dynamic Meshes");
		for (auto& [mk, dc] : m_DrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (!dc.IsRigged)
				Renderer::RenderSubmeshInstanced(m_CommandBuffer, m_GeometryPipeline, dc.Mesh, dc.SubmeshIndex, dc.MaterialTable ? dc.MaterialTable : dc.Mesh->GetMaterials(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, dc.InstanceCount);
		}
		SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);

#if 0
		{
			// Render static meshes
			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Static Transparent Meshes");
			for (auto& [mk, dc] : m_TransparentStaticMeshDrawList)
			{
				const auto& transformData = m_MeshTransformMap.at(mk);
				Renderer::RenderStaticMesh(m_CommandBuffer, m_TransparentGeometryPipeline, dc.StaticMesh, dc.SubmeshIndex, dc.MaterialTable ? dc.MaterialTable : dc.StaticMesh->GetMaterials(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount);
			}
			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);

			// Render dynamic meshes
			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Dynamic Transparent Meshes");
			for (auto& [mk, dc] : m_TransparentDrawList)
			{
				const auto& transformData = m_MeshTransformMap.at(mk);
				//Renderer::RenderSubmesh(m_CommandBuffer, m_GeometryPipeline, m_UniformBufferSet, m_StorageBufferSet, dc.Mesh, dc.SubmeshIndex, dc.MaterialTable ? dc.MaterialTable : dc.Mesh->GetMaterials(), dc.Transform);
				Renderer::RenderSubmeshInstanced(m_CommandBuffer, m_TransparentGeometryPipeline, dc.Mesh, dc.SubmeshIndex, dc.MaterialTable ? dc.MaterialTable : dc.Mesh->GetMaterials(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, dc.InstanceCount);
			}
			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
		}
#endif
		Renderer::EndRenderPass(m_CommandBuffer);

		Renderer::BeginRenderPass(m_CommandBuffer, m_GeometryAnimPass);
		for (auto& [mk, dc] : m_DrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (dc.IsRigged)
			{
				const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
				Renderer::RenderSubmeshInstanced(m_CommandBuffer, m_GeometryPipelineAnim, dc.Mesh, dc.SubmeshIndex, dc.MaterialTable ? dc.MaterialTable : dc.Mesh->GetMaterials(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, boneTransformsData.BoneTransformsBaseIndex, dc.InstanceCount);
			}
		}
		Renderer::EndRenderPass(m_CommandBuffer);
	}

	void SceneRenderer::PreConvolutionCompute()
	{
		ZONG_PROFILE_FUNC();

		// TODO: Other techniques might need it in the future
		if (!m_Options.EnableSSR)
			return;

		struct PreConvolutionComputePushConstants
		{
			int PrevLod = 0;
			int Mode = 0; // 0 = Copy, 1 = GaussianHorizontal, 2 = GaussianVertical
		} preConvolutionComputePushConstants;

		// Might change to be maximum res used by other techniques other than SSR
		int halfRes = int(m_SSROptions.HalfRes);

		m_GPUTimeQueries.PreConvolutionQuery = m_CommandBuffer->BeginTimestampQuery();

		glm::uvec3 workGroups(0);

		Renderer::BeginComputePass(m_CommandBuffer, m_PreConvolutionComputePass);

		auto inputImage = m_SkyboxPass->GetOutput(0);
		workGroups = { (uint32_t)glm::ceil((float)inputImage->GetWidth() / 16.0f), (uint32_t)glm::ceil((float)inputImage->GetHeight() / 16.0f), 1 };
		Renderer::DispatchCompute(m_CommandBuffer, m_PreConvolutionComputePass, m_PreConvolutionMaterials[0], workGroups, Buffer(&preConvolutionComputePushConstants, sizeof(preConvolutionComputePushConstants)));

		const uint32_t mipCount = m_PreConvolutedTexture.Texture->GetMipLevelCount();
		for (uint32_t mip = 1; mip < mipCount; mip++)
		{
			Renderer::BeginGPUPerfMarker(m_CommandBuffer, fmt::format("Pre-Convolution-Mip({})", mip));

			auto [mipWidth, mipHeight] = m_PreConvolutedTexture.Texture->GetMipSize(mip);
			workGroups = { (uint32_t)glm::ceil((float)mipWidth / 16.0f), (uint32_t)glm::ceil((float)mipHeight / 16.0f), 1 };
			preConvolutionComputePushConstants.PrevLod = (int)mip - 1;

			auto blur = [&](const int mip, const int mode)
			{
				Renderer::BeginGPUPerfMarker(m_CommandBuffer, fmt::format("Pre-Convolution-Mode({})", mode));
				preConvolutionComputePushConstants.Mode = (int)mode;
				Renderer::DispatchCompute(m_CommandBuffer, m_PreConvolutionComputePass, m_PreConvolutionMaterials[mip], workGroups, Buffer(&preConvolutionComputePushConstants, sizeof(preConvolutionComputePushConstants)));
				Renderer::EndGPUPerfMarker(m_CommandBuffer);
			};

			blur(mip, 1); // Horizontal blur
			blur(mip, 2); // Vertical Blur

			Renderer::EndGPUPerfMarker(m_CommandBuffer);
		}

		Renderer::EndComputePass(m_CommandBuffer, m_PreConvolutionComputePass);
	}

	void SceneRenderer::GTAOCompute()
	{
		const Buffer pushConstantBuffer(&GTAODataCB, sizeof GTAODataCB);

		m_GPUTimeQueries.GTAOPassQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginComputePass(m_CommandBuffer, m_GTAOComputePass);
		Renderer::DispatchCompute(m_CommandBuffer, m_GTAOComputePass, nullptr, m_GTAOWorkGroups, pushConstantBuffer);
		Renderer::EndComputePass(m_CommandBuffer, m_GTAOComputePass);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.GTAOPassQuery);
	}

	void SceneRenderer::GTAODenoiseCompute()
	{
		m_GTAODenoiseConstants.DenoiseBlurBeta = GTAODataCB.DenoiseBlurBeta;
		m_GTAODenoiseConstants.HalfRes = GTAODataCB.HalfRes;
		const Buffer pushConstantBuffer(&m_GTAODenoiseConstants, sizeof(GTAODenoiseConstants));

		m_GPUTimeQueries.GTAODenoisePassQuery = m_CommandBuffer->BeginTimestampQuery();
		for (uint32_t pass = 0; pass < (uint32_t)m_Options.GTAODenoisePasses; pass++)
		{
			auto renderPass = m_GTAODenoisePass[uint32_t(pass % 2 != 0)];
			Renderer::BeginComputePass(m_CommandBuffer, renderPass);
			Renderer::DispatchCompute(m_CommandBuffer, renderPass, nullptr, m_GTAODenoiseWorkGroups, pushConstantBuffer);
			Renderer::EndComputePass(m_CommandBuffer, renderPass);
		}
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.GTAODenoisePassQuery);
	}

	void SceneRenderer::AOComposite()
	{
		// if (m_Options.EnableHBAO)
		//	m_AOCompositeMaterial->Set("u_HBAOTex", m_HBAOBlurPipelines[1]->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage());
		m_GPUTimeQueries.AOCompositePassQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginRenderPass(m_CommandBuffer, m_AOCompositePass);
		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_AOCompositePass->GetSpecification().Pipeline, m_AOCompositeMaterial);
		Renderer::EndRenderPass(m_CommandBuffer);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.AOCompositePassQuery);
	}

	void SceneRenderer::JumpFloodPass()
	{
		ZONG_PROFILE_FUNC();

		m_GPUTimeQueries.JumpFloodPassQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodInitPass);

		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_JumpFloodInitPass->GetSpecification().Pipeline, m_JumpFloodInitMaterial);
		Renderer::EndRenderPass(m_CommandBuffer);

		int steps = 2;
		int step = (int)glm::round(glm::pow<int>(steps - 1, 2));
		int index = 0;
		Buffer vertexOverrides;
		Ref<Framebuffer> passFB = m_JumpFloodPass[0]->GetTargetFramebuffer();
		glm::vec2 texelSize = { 1.0f / (float)passFB->GetWidth(), 1.0f / (float)passFB->GetHeight() };
		vertexOverrides.Allocate(sizeof(glm::vec2) + sizeof(int));
		vertexOverrides.Write(glm::value_ptr(texelSize), sizeof(glm::vec2));
		while (step != 0)
		{
			vertexOverrides.Write(&step, sizeof(int), sizeof(glm::vec2));

			Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodPass[index]);
			Renderer::SubmitFullscreenQuadWithOverrides(m_CommandBuffer, m_JumpFloodPass[index]->GetSpecification().Pipeline, m_JumpFloodPassMaterial[index], vertexOverrides, Buffer());
			Renderer::EndRenderPass(m_CommandBuffer);

			index = (index + 1) % 2;
			step /= 2;
		}

		vertexOverrides.Release();

		//m_JumpFloodCompositeMaterial->Set("u_Texture", m_TempFramebuffers[1]->GetImage());
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.JumpFloodPassQuery);
	}

	void SceneRenderer::SSRCompute()
	{
		ZONG_PROFILE_FUNC();

		const Buffer pushConstantsBuffer(&m_SSROptions, sizeof(SSROptionsUB));

		m_GPUTimeQueries.SSRQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginComputePass(m_CommandBuffer, m_SSRPass);
		Renderer::DispatchCompute(m_CommandBuffer, m_SSRPass, nullptr, m_SSRWorkGroups, pushConstantsBuffer);
		Renderer::EndComputePass(m_CommandBuffer, m_SSRPass);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.SSRQuery);
	}

	void SceneRenderer::SSRCompositePass()
	{
		// Currently scales the SSR, renders with transparency.
		// The alpha channel is the confidence.

		m_GPUTimeQueries.SSRCompositeQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginRenderPass(m_CommandBuffer, m_SSRCompositePass);
		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_SSRCompositePass->GetPipeline(), nullptr);
		Renderer::EndRenderPass(m_CommandBuffer);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.SSRCompositeQuery);
	}

	void SceneRenderer::BloomCompute()
	{
		glm::uvec3 workGroups(0);

		struct BloomComputePushConstants
		{
			glm::vec4 Params;
			float LOD = 0.0f;
			int Mode = 0; // 0 = prefilter, 1 = downsample, 2 = firstUpsample, 3 = upsample
		} bloomComputePushConstants;
		bloomComputePushConstants.Params = { m_BloomSettings.Threshold, m_BloomSettings.Threshold - m_BloomSettings.Knee, m_BloomSettings.Knee * 2.0f, 0.25f / m_BloomSettings.Knee };
		bloomComputePushConstants.Mode = 0;
	
		m_GPUTimeQueries.BloomComputePassQuery = m_CommandBuffer->BeginTimestampQuery();

		Renderer::BeginComputePass(m_CommandBuffer, m_BloomComputePass);

		// ===================
		//      Prefilter
		// ===================
		Renderer::BeginGPUPerfMarker(m_CommandBuffer, "Bloom-Prefilter");
		{
			workGroups = { m_BloomComputeTextures[0].Texture->GetWidth() / m_BloomComputeWorkgroupSize, m_BloomComputeTextures[0].Texture->GetHeight() / m_BloomComputeWorkgroupSize, 1 };
			Renderer::DispatchCompute(m_CommandBuffer, m_BloomComputePass, m_BloomComputeMaterials.PrefilterMaterial, workGroups, Buffer(&bloomComputePushConstants, sizeof(bloomComputePushConstants)));
			m_BloomComputePipeline->ImageMemoryBarrier(m_CommandBuffer, m_BloomComputeTextures[0].Texture->GetImage(), ResourceAccessFlags::ShaderWrite, ResourceAccessFlags::ShaderRead);
		}
		Renderer::EndGPUPerfMarker(m_CommandBuffer);

		// ===================
		//      Downsample
		// ===================
		bloomComputePushConstants.Mode = 1;
		uint32_t mips = m_BloomComputeTextures[0].Texture->GetMipLevelCount() - 2;
		Renderer::BeginGPUPerfMarker(m_CommandBuffer, "Bloom-DownSample");
		{
			for (uint32_t i = 1; i < mips; i++)
			{
				auto [mipWidth, mipHeight] = m_BloomComputeTextures[0].Texture->GetMipSize(i);
				workGroups = { (uint32_t)glm::ceil((float)mipWidth / (float)m_BloomComputeWorkgroupSize) ,(uint32_t)glm::ceil((float)mipHeight / (float)m_BloomComputeWorkgroupSize), 1 };

				bloomComputePushConstants.LOD = i - 1.0f;
				Renderer::DispatchCompute(m_CommandBuffer, m_BloomComputePass, m_BloomComputeMaterials.DownsampleAMaterials[i], workGroups, Buffer(&bloomComputePushConstants, sizeof(bloomComputePushConstants)));

				m_BloomComputePipeline->ImageMemoryBarrier(m_CommandBuffer, m_BloomComputeTextures[1].Texture->GetImage(), ResourceAccessFlags::ShaderWrite, ResourceAccessFlags::ShaderRead);

				bloomComputePushConstants.LOD = (float)i;
				Renderer::DispatchCompute(m_CommandBuffer, m_BloomComputePass, m_BloomComputeMaterials.DownsampleBMaterials[i], workGroups, Buffer(&bloomComputePushConstants, sizeof(bloomComputePushConstants)));

				m_BloomComputePipeline->ImageMemoryBarrier(m_CommandBuffer, m_BloomComputeTextures[0].Texture->GetImage(), ResourceAccessFlags::ShaderWrite, ResourceAccessFlags::ShaderRead);
			}
		}
		Renderer::EndGPUPerfMarker(m_CommandBuffer);


		// ===================
		//   First Upsample
		// ===================
		bloomComputePushConstants.Mode = 2;
		bloomComputePushConstants.LOD--;
		Renderer::BeginGPUPerfMarker(m_CommandBuffer, "Bloom-FirstUpsamle");
		{
			auto [mipWidth, mipHeight] = m_BloomComputeTextures[2].Texture->GetMipSize(mips - 2);
			workGroups.x = (uint32_t)glm::ceil((float)mipWidth / (float)m_BloomComputeWorkgroupSize);
			workGroups.y = (uint32_t)glm::ceil((float)mipHeight / (float)m_BloomComputeWorkgroupSize);

			Renderer::DispatchCompute(m_CommandBuffer, m_BloomComputePass, m_BloomComputeMaterials.FirstUpsampleMaterial, workGroups, Buffer(&bloomComputePushConstants, sizeof(bloomComputePushConstants)));
			m_BloomComputePipeline->ImageMemoryBarrier(m_CommandBuffer, m_BloomComputeTextures[2].Texture->GetImage(), ResourceAccessFlags::ShaderWrite, ResourceAccessFlags::ShaderRead);
		}
		Renderer::EndGPUPerfMarker(m_CommandBuffer);

		// ===================
		//      Upsample
		// ===================
		Renderer::BeginGPUPerfMarker(m_CommandBuffer, "Bloom-Upsample");
		{
			bloomComputePushConstants.Mode = 3;
			for (int32_t mip = mips - 3; mip >= 0; mip--)
			{
				auto [mipWidth, mipHeight] = m_BloomComputeTextures[2].Texture->GetMipSize(mip);
				workGroups.x = (uint32_t)glm::ceil((float)mipWidth / (float)m_BloomComputeWorkgroupSize);
				workGroups.y = (uint32_t)glm::ceil((float)mipHeight / (float)m_BloomComputeWorkgroupSize);

				bloomComputePushConstants.LOD = (float)mip;
				Renderer::DispatchCompute(m_CommandBuffer, m_BloomComputePass, m_BloomComputeMaterials.UpsampleMaterials[mip], workGroups, Buffer(&bloomComputePushConstants, sizeof(bloomComputePushConstants)));

				m_BloomComputePipeline->ImageMemoryBarrier(m_CommandBuffer, m_BloomComputeTextures[2].Texture->GetImage(), ResourceAccessFlags::ShaderWrite, ResourceAccessFlags::ShaderRead);
			}
		}
		Renderer::EndGPUPerfMarker(m_CommandBuffer);

		Renderer::EndComputePass(m_CommandBuffer, m_BloomComputePass);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.BloomComputePassQuery);
	}
	
	void SceneRenderer::EdgeDetectionPass()
	{
		Renderer::BeginRenderPass(m_CommandBuffer, m_EdgeDetectionPass);
		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_EdgeDetectionPass->GetPipeline(), nullptr);
		Renderer::EndRenderPass(m_CommandBuffer);
	}

	void SceneRenderer::CompositePass()
	{
		ZONG_PROFILE_FUNC();

		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		m_GPUTimeQueries.CompositePassQuery = m_CommandBuffer->BeginTimestampQuery();

		Renderer::BeginRenderPass(m_CommandBuffer, m_CompositePass);

		//auto framebuffer = m_GeometryPass->GetTargetFramebuffer();
		auto framebuffer = m_SkyboxPass->GetTargetFramebuffer();
		float exposure = m_SceneData.SceneCamera.Camera.GetExposure();
		int textureSamples = framebuffer->GetSpecification().Samples;

		m_CompositeMaterial->Set("u_Uniforms.Exposure", exposure);
		if (m_BloomSettings.Enabled)
		{
			m_CompositeMaterial->Set("u_Uniforms.BloomIntensity", m_BloomSettings.Intensity);
			m_CompositeMaterial->Set("u_Uniforms.BloomDirtIntensity", m_BloomSettings.DirtIntensity);
		}
		else
		{
			m_CompositeMaterial->Set("u_Uniforms.BloomIntensity", 0.0f);
			m_CompositeMaterial->Set("u_Uniforms.BloomDirtIntensity", 0.0f);
		}

		m_CompositeMaterial->Set("u_Uniforms.Opacity", m_Opacity);
		m_CompositeMaterial->Set("u_Uniforms.Time", Application::Get().GetTime());

		SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Composite");
		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_CompositePass->GetPipeline(), m_CompositeMaterial);
		SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
		
		Renderer::EndRenderPass(m_CommandBuffer);

		if (m_DOFSettings.Enabled)
		{
			//Renderer::EndRenderPass(m_CommandBuffer);

			Renderer::BeginRenderPass(m_CommandBuffer, m_DOFPass);
			//m_DOFMaterial->Set("u_Texture", m_CompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage());
			//m_DOFMaterial->Set("u_DepthTexture", m_PreDepthPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetDepthImage());
			m_DOFMaterial->Set("u_Uniforms.DOFParams", glm::vec2(m_DOFSettings.FocusDistance, m_DOFSettings.BlurSize));

			Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_DOFPass->GetPipeline(), m_DOFMaterial);

			//SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "JumpFlood-Composite");
			//Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_JumpFloodCompositePipeline, nullptr, m_JumpFloodCompositeMaterial);
			//SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);

			Renderer::EndRenderPass(m_CommandBuffer);

			// Copy DOF image to composite pipeline
			Renderer::CopyImage(m_CommandBuffer,
				m_DOFPass->GetTargetFramebuffer()->GetImage(),
				m_CompositePass->GetTargetFramebuffer()->GetImage());

			// WIP - will later be used for debugging/editor mouse picking
#if 0
			if (m_ReadBackImage)
			{
				Renderer::CopyImage(m_CommandBuffer,
					m_DOFPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage(),
					m_ReadBackImage);

				{
					auto alloc = m_ReadBackImage.As<VulkanImage2D>()->GetImageInfo().MemoryAlloc;
					VulkanAllocator allocator("SceneRenderer");
					glm::vec4* mappedMem = allocator.MapMemory<glm::vec4>(alloc);
					delete[] m_ReadBackBuffer;
					m_ReadBackBuffer = new glm::vec4[m_ReadBackImage->GetWidth() * m_ReadBackImage->GetHeight()];
					memcpy(m_ReadBackBuffer, mappedMem, sizeof(glm::vec4) * m_ReadBackImage->GetWidth() * m_ReadBackImage->GetHeight());
					allocator.UnmapMemory(alloc);
				}
			}
#endif
		}

		// Grid
		if (GetOptions().ShowGrid)
		{
			Renderer::BeginRenderPass(m_CommandBuffer, m_GridRenderPass);
			const static glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(8.0f));
			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Grid");
			Renderer::RenderQuad(m_CommandBuffer, m_GridRenderPass->GetPipeline(), m_GridMaterial, transform);
			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
			Renderer::EndRenderPass(m_CommandBuffer);
		}

		// TODO(Yan): don't do this in runtime!
		if (m_Specification.JumpFloodPass)
		{
			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "JumpFlood-Composite");

			Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodCompositePass);
			Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_JumpFloodCompositePass->GetSpecification().Pipeline, m_JumpFloodCompositeMaterial);
			Renderer::EndRenderPass(m_CommandBuffer);

			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
		}

		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.CompositePassQuery);

		if (m_Options.ShowSelectedInWireframe)
		{
			Renderer::BeginRenderPass(m_CommandBuffer, m_GeometryWireframePass);

			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Static Meshes Wireframe");
			for (auto& [mk, dc] : m_SelectedStaticMeshDrawList)
			{
				const auto& transformData = m_MeshTransformMap.at(mk);
				Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_GeometryWireframePass->GetPipeline(), dc.StaticMesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset + dc.InstanceOffset * sizeof(TransformVertexData), dc.InstanceCount, m_WireframeMaterial);
			}

			for (auto& [mk, dc] : m_SelectedMeshDrawList)
			{
				if (!dc.IsRigged)
				{
					const auto& transformData = m_MeshTransformMap.at(mk);
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_GeometryWireframePass->GetPipeline(), dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset + dc.InstanceOffset * sizeof(TransformVertexData), 0, dc.InstanceCount, m_WireframeMaterial);
				}
			}

			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
			Renderer::EndRenderPass(m_CommandBuffer);

			Renderer::BeginRenderPass(m_CommandBuffer, m_GeometryWireframeAnimPass);
			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Dynamic Meshes Wireframe");
			for (auto& [mk, dc] : m_SelectedMeshDrawList)
			{
				const auto& transformData = m_MeshTransformMap.at(mk);
				if (dc.IsRigged)
				{
					const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_GeometryWireframeAnimPass->GetPipeline(), dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset + dc.InstanceOffset * sizeof(TransformVertexData), boneTransformsData.BoneTransformsBaseIndex + dc.InstanceOffset, dc.InstanceCount, m_WireframeMaterial);
				}
			}
			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);

			Renderer::EndRenderPass(m_CommandBuffer);
		}

		if (m_Options.ShowPhysicsColliders)
		{
			auto staticPass = m_Options.ShowPhysicsCollidersOnTop ? m_GeometryWireframeOnTopPass : m_GeometryWireframePass;
			auto animPass = m_Options.ShowPhysicsCollidersOnTop? m_GeometryWireframeOnTopAnimPass : m_GeometryWireframeAnimPass;

			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Static Meshes Collider");
			Renderer::BeginRenderPass(m_CommandBuffer, staticPass);
			for (auto& [mk, dc] : m_StaticColliderDrawList)
			{
				ZONG_CORE_VERIFY(m_MeshTransformMap.find(mk) != m_MeshTransformMap.end());
				const auto& transformData = m_MeshTransformMap.at(mk);
				Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, staticPass->GetPipeline(), dc.StaticMesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount, dc.OverrideMaterial);
			}

			for (auto& [mk, dc] : m_ColliderDrawList)
			{
				ZONG_CORE_VERIFY(m_MeshTransformMap.find(mk) != m_MeshTransformMap.end());
				const auto& transformData = m_MeshTransformMap.at(mk);
				if (!dc.IsRigged)
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, staticPass->GetPipeline(), dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, dc.InstanceCount, m_SimpleColliderMaterial);
			}

			Renderer::EndRenderPass(m_CommandBuffer);
			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);

			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Animated Meshes Collider");
			Renderer::BeginRenderPass(m_CommandBuffer, animPass);
			for (auto& [mk, dc] : m_ColliderDrawList)
			{
				ZONG_CORE_VERIFY(m_MeshTransformMap.find(mk) != m_MeshTransformMap.end());
				const auto& transformData = m_MeshTransformMap.at(mk);
				if (dc.IsRigged)
				{
					const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, animPass->GetPipeline(), dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, boneTransformsData.BoneTransformsBaseIndex, dc.InstanceCount, m_SimpleColliderMaterial);
				}
				else
				{
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, animPass->GetPipeline(), dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, {}, dc.InstanceCount, m_SimpleColliderMaterial);
				}
			}

			Renderer::EndRenderPass(m_CommandBuffer);
			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
		}
	}

	void SceneRenderer::FlushDrawList()
	{
		if (m_ResourcesCreated && m_ViewportWidth > 0 && m_ViewportHeight > 0)
		{
			// Reset GPU time queries
			m_GPUTimeQueries = SceneRenderer::GPUTimeQueries();

			PreRender();

			m_CommandBuffer->Begin();

			// Main render passes
			ShadowMapPass();
			SpotShadowMapPass();
			PreDepthPass();
			HZBCompute();
			PreIntegration();
			LightCullingPass();
			SkyboxPass();
			GeometryPass();

		
			// GTAO
			if (m_Options.EnableGTAO)
			{
				GTAOCompute();
				GTAODenoiseCompute();
			}

			if (m_Options.EnableGTAO)
				AOComposite();

			PreConvolutionCompute();

			// Post-processing
			if (m_Specification.JumpFloodPass)
				JumpFloodPass();	

			//SSR
			if (m_Options.EnableSSR)
			{
				SSRCompute();
				SSRCompositePass();
			}

			if (m_Specification.EnableEdgeOutlineEffect)
				EdgeDetectionPass();

			BloomCompute();
			CompositePass();

			m_CommandBuffer->End();
			m_CommandBuffer->Submit();
		}
		else
		{
			// Empty pass
			m_CommandBuffer->Begin();

			ClearPass();

			m_CommandBuffer->End();
			m_CommandBuffer->Submit();
		}

		UpdateStatistics();

		m_DrawList.clear();
		m_TransparentDrawList.clear();
		m_SelectedMeshDrawList.clear();
		m_ShadowPassDrawList.clear();

		m_StaticMeshDrawList.clear();
		m_TransparentStaticMeshDrawList.clear();
		m_SelectedStaticMeshDrawList.clear();
		m_StaticMeshShadowPassDrawList.clear();

		m_ColliderDrawList.clear();
		m_StaticColliderDrawList.clear();
		m_SceneData = {};

		m_MeshTransformMap.clear();
		m_MeshBoneTransformsMap.clear();
	}

	void SceneRenderer::PreRender()
	{
		ZONG_PROFILE_FUNC();

		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		uint32_t offset = 0;
		for (auto& [key, transformData] : m_MeshTransformMap)
		{
			transformData.TransformOffset = offset * sizeof(TransformVertexData);
			for (const auto& transform : transformData.Transforms)
			{
				m_SubmeshTransformBuffers[frameIndex].Data[offset] = transform;
				offset++;
			}

		}

		m_SubmeshTransformBuffers[frameIndex].Buffer->SetData(m_SubmeshTransformBuffers[frameIndex].Data, offset * sizeof(TransformVertexData));

		uint32_t index = 0;
		for (auto& [key, boneTransformsData] : m_MeshBoneTransformsMap)
		{
			boneTransformsData.BoneTransformsBaseIndex = index;
			for (const auto& boneTransforms : boneTransformsData.BoneTransformsData)
			{
				m_BoneTransformsData[index++] = boneTransforms;
			}
		}

		if (index > 0)
		{
			Ref<SceneRenderer> instance = this;
			Renderer::Submit([instance, index]() mutable
			{
				instance->m_SBSBoneTransforms->RT_Get()->RT_SetData(instance->m_BoneTransformsData, static_cast<uint32_t>(index * sizeof(BoneTransforms)));
			});
		}
	}

	void SceneRenderer::CopyToBoneTransformStorage(const MeshKey& meshKey, const Ref<MeshSource>& meshSource, const std::vector<glm::mat4>& boneTransforms)
	{
		auto& boneTransformStorage = m_MeshBoneTransformsMap[meshKey].BoneTransformsData.emplace_back();
		if (boneTransforms.empty())
		{
			boneTransformStorage.fill(glm::identity<glm::mat4>());
		}
		else
		{
			for (size_t i = 0; i < meshSource->m_BoneInfo.size(); ++i)
			{
				const auto submeshInvTransform = meshSource->m_BoneInfo[i].SubMeshInverseTransform;
				const auto boneTransform = boneTransforms[meshSource->m_BoneInfo[i].BoneIndex];
				const auto invBindPose = meshSource->m_BoneInfo[i].InverseBindPose;
				boneTransformStorage[i] = submeshInvTransform * boneTransform * invBindPose;
			}
		}
	}

	void SceneRenderer::CreateBloomPassMaterials()
	{
		auto inputImage = m_SkyboxPass->GetOutput(0);

		// Prefilter
		m_BloomComputeMaterials.PrefilterMaterial = Material::Create(m_BloomComputePass->GetShader());
		m_BloomComputeMaterials.PrefilterMaterial->Set("o_Image", m_BloomComputeTextures[0].ImageViews[0]);
		m_BloomComputeMaterials.PrefilterMaterial->Set("u_Texture", inputImage);
		m_BloomComputeMaterials.PrefilterMaterial->Set("u_BloomTexture", inputImage);

		// Downsample
		uint32_t mips = m_BloomComputeTextures[0].Texture->GetMipLevelCount() - 2;
		m_BloomComputeMaterials.DownsampleAMaterials.resize(mips);
		m_BloomComputeMaterials.DownsampleBMaterials.resize(mips);
		for (uint32_t i = 1; i < mips; i++)
		{
			m_BloomComputeMaterials.DownsampleAMaterials[i] = Material::Create(m_BloomComputePass->GetShader());
			m_BloomComputeMaterials.DownsampleAMaterials[i]->Set("o_Image", m_BloomComputeTextures[1].ImageViews[i]);
			m_BloomComputeMaterials.DownsampleAMaterials[i]->Set("u_Texture", m_BloomComputeTextures[0].Texture);
			m_BloomComputeMaterials.DownsampleAMaterials[i]->Set("u_BloomTexture", inputImage);

			m_BloomComputeMaterials.DownsampleBMaterials[i] = Material::Create(m_BloomComputePass->GetShader());
			m_BloomComputeMaterials.DownsampleBMaterials[i]->Set("o_Image", m_BloomComputeTextures[0].ImageViews[i]);
			m_BloomComputeMaterials.DownsampleBMaterials[i]->Set("u_Texture", m_BloomComputeTextures[1].Texture);
			m_BloomComputeMaterials.DownsampleBMaterials[i]->Set("u_BloomTexture", inputImage);
		}

		// Upsample
		m_BloomComputeMaterials.FirstUpsampleMaterial = Material::Create(m_BloomComputePass->GetShader());
		m_BloomComputeMaterials.FirstUpsampleMaterial->Set("o_Image", m_BloomComputeTextures[2].ImageViews[mips - 2]);
		m_BloomComputeMaterials.FirstUpsampleMaterial->Set("u_Texture", m_BloomComputeTextures[0].Texture);
		m_BloomComputeMaterials.FirstUpsampleMaterial->Set("u_BloomTexture", inputImage);

		m_BloomComputeMaterials.UpsampleMaterials.resize(mips - 3 + 1);
		for (int32_t mip = mips - 3; mip >= 0; mip--)
		{
			m_BloomComputeMaterials.UpsampleMaterials[mip] = Material::Create(m_BloomComputePass->GetShader());
			m_BloomComputeMaterials.UpsampleMaterials[mip]->Set("o_Image", m_BloomComputeTextures[2].ImageViews[mip]);
			m_BloomComputeMaterials.UpsampleMaterials[mip]->Set("u_Texture", m_BloomComputeTextures[0].Texture);
			m_BloomComputeMaterials.UpsampleMaterials[mip]->Set("u_BloomTexture", m_BloomComputeTextures[2].Texture);
		}
	}

	void SceneRenderer::CreatePreConvolutionPassMaterials()
	{
		auto inputImage = m_SkyboxPass->GetOutput(0);

		uint32_t mips = m_PreConvolutedTexture.Texture->GetMipLevelCount();
		m_PreConvolutionMaterials.resize(mips);

		for (uint32_t i = 0; i < mips; i++)
		{
			m_PreConvolutionMaterials[i] = Material::Create(m_PreConvolutionComputePass->GetShader());
			m_PreConvolutionMaterials[i]->Set("o_Image", m_PreConvolutedTexture.ImageViews[i]);
			m_PreConvolutionMaterials[i]->Set("u_Input", i == 0 ? inputImage : m_PreConvolutedTexture.Texture->GetImage());
		}

	}

	void SceneRenderer::CreateHZBPassMaterials()
	{
		constexpr uint32_t maxMipBatchSize = 4;
		const uint32_t hzbMipCount = m_HierarchicalDepthTexture.Texture->GetMipLevelCount();

		Ref<Shader> hzbShader = Renderer::GetShaderLibrary()->Get("HZB");

		uint32_t materialIndex = 0;
		m_HZBMaterials.resize(Math::DivideAndRoundUp(hzbMipCount, 4u));
		for (uint32_t startDestMip = 0; startDestMip < hzbMipCount; startDestMip += maxMipBatchSize)
		{
			Ref<Material> material = Material::Create(hzbShader);
			m_HZBMaterials[materialIndex++] = material;
			
			if (startDestMip == 0)
				material->Set("u_InputDepth", m_PreDepthPass->GetDepthOutput());
			else
				material->Set("u_InputDepth", m_HierarchicalDepthTexture.Texture);

			const uint32_t endDestMip = glm::min(startDestMip + maxMipBatchSize, hzbMipCount);
			uint32_t destMip;
			for (destMip = startDestMip; destMip < endDestMip; ++destMip)
			{
				uint32_t index = destMip - startDestMip;
				material->Set("o_HZB", m_HierarchicalDepthTexture.ImageViews[destMip], index);
			}

			// Fill the rest .. or we could enable the null descriptor feature
			destMip -= startDestMip;
			for (; destMip < maxMipBatchSize; ++destMip)
			{
				material->Set("o_HZB", m_HierarchicalDepthTexture.ImageViews[hzbMipCount - 1], destMip); // could be white texture?
			}
		}
	}

	void SceneRenderer::CreatePreIntegrationPassMaterials()
	{
		Ref<Shader> preIntegrationShader = Renderer::GetShaderLibrary()->Get("Pre-Integration");

		Ref<Texture2D> visibilityTexture = m_PreIntegrationVisibilityTexture.Texture;
		m_PreIntegrationMaterials.resize(visibilityTexture->GetMipLevelCount() - 1);
		for (uint32_t i = 0; i < visibilityTexture->GetMipLevelCount() - 1; i++)
		{
			Ref<Material> material = Material::Create(preIntegrationShader);
			m_PreIntegrationMaterials[i] = material;

			material->Set("o_VisibilityImage", m_PreIntegrationVisibilityTexture.ImageViews[i]);
			material->Set("u_VisibilityTex", visibilityTexture);
			material->Set("u_HZB", m_HierarchicalDepthTexture.Texture);
		}
	}

	void SceneRenderer::ClearPass()
	{
		ZONG_PROFILE_FUNC();

		Renderer::BeginRenderPass(m_CommandBuffer, m_PreDepthPass, true);
		Renderer::EndRenderPass(m_CommandBuffer);

		Renderer::BeginRenderPass(m_CommandBuffer, m_CompositePass, true);
		Renderer::EndRenderPass(m_CommandBuffer);

		//Renderer::BeginRenderPass(m_CommandBuffer, m_DOFPipeline->GetSpecification().RenderPass, true);
		//Renderer::EndRenderPass(m_CommandBuffer);
	}

	Ref<Pipeline> SceneRenderer::GetFinalPipeline()
	{
		return m_CompositePass->GetSpecification().Pipeline;
	}

	Ref<RenderPass> SceneRenderer::GetFinalRenderPass()
	{
		return m_CompositePass;
	}

	Ref<Image2D> SceneRenderer::GetFinalPassImage()
	{
		if (!m_ResourcesCreated)
			return nullptr;

//		return GetFinalPipeline()->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage();
		return m_CompositePass->GetOutput(0);
	}

	SceneRendererOptions& SceneRenderer::GetOptions()
	{
		return m_Options;
	}

	void SceneRenderer::CalculateCascades(CascadeData* cascades, const SceneRendererCamera& sceneCamera, const glm::vec3& lightDirection) const
	{
		//TODO: Reversed Z projection?

		float scaleToOrigin = m_ScaleShadowCascadesToOrigin;

		glm::mat4 viewMatrix = sceneCamera.ViewMatrix;
		constexpr glm::vec4 origin = glm::vec4(glm::vec3(0.0f), 1.0f);
		viewMatrix[3] = glm::lerp(viewMatrix[3], origin, scaleToOrigin);

		auto viewProjection = sceneCamera.Camera.GetUnReversedProjectionMatrix() * viewMatrix;

		const int SHADOW_MAP_CASCADE_COUNT = 4;
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

		float nearClip = sceneCamera.Near;
		float farClip = sceneCamera.Far;
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = CascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		cascadeSplits[3] = 0.3f;

		// Manually set cascades here
		// cascadeSplits[0] = 0.05f;
		// cascadeSplits[1] = 0.15f;
		// cascadeSplits[2] = 0.3f;
		// cascadeSplits[3] = 1.0f;

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] =
			{
				glm::vec3(-1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
			};

			// Project frustum corners into world space
			glm::mat4 invCam = glm::inverse(viewProjection);
			for (uint32_t i = 0; i < 8; i++)
			{
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++)
			{
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++)
				frustumCenter += frustumCorners[i];

			frustumCenter /= 8.0f;

			//frustumCenter *= 0.01f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++)
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = -lightDirection;
			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + CascadeNearPlaneOffset, maxExtents.z - minExtents.z + CascadeFarPlaneOffset);

			// Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
			glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			float ShadowMapResolution = (float)m_DirectionalShadowMapPass[0]->GetTargetFramebuffer()->GetWidth();

			glm::vec4 shadowOrigin = (shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution / 2.0f;
			glm::vec4 roundedOrigin = glm::round(shadowOrigin);
			glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * 2.0f / ShadowMapResolution;
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;

			lightOrthoMatrix[3] += roundOffset;

			// Store split distance and matrix in cascade
			cascades[i].SplitDepth = (nearClip + splitDist * clipRange) * -1.0f;
			cascades[i].ViewProj = lightOrthoMatrix * lightViewMatrix;
			cascades[i].View = lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}
	}

	void SceneRenderer::CalculateCascadesManualSplit(CascadeData* cascades, const SceneRendererCamera& sceneCamera, const glm::vec3& lightDirection) const
	{
		//TODO: Reversed Z projection?

		float scaleToOrigin = m_ScaleShadowCascadesToOrigin;

		glm::mat4 viewMatrix = sceneCamera.ViewMatrix;
		constexpr glm::vec4 origin = glm::vec4(glm::vec3(0.0f), 1.0f);
		viewMatrix[3] = glm::lerp(viewMatrix[3], origin, scaleToOrigin);

		auto viewProjection = sceneCamera.Camera.GetUnReversedProjectionMatrix() * viewMatrix;

		const int SHADOW_MAP_CASCADE_COUNT = 4;

		float nearClip = sceneCamera.Near;
		float farClip = sceneCamera.Far;
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float splitDist = m_ShadowCascadeSplits[0];
			lastSplitDist = 0.0;

			glm::vec3 frustumCorners[8] =
			{
				glm::vec3(-1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
			};

			// Project frustum corners into world space
			glm::mat4 invCam = glm::inverse(viewProjection);
			for (uint32_t i = 0; i < 8; i++)
			{
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++)
			{
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++)
				frustumCenter += frustumCorners[i];

			frustumCenter /= 8.0f;

			//frustumCenter *= 0.01f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++)
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;
			radius *= m_ShadowCascadeSplits[1];

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = -lightDirection;
			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + CascadeNearPlaneOffset, maxExtents.z - minExtents.z + CascadeFarPlaneOffset);

			// Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
			glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			float ShadowMapResolution = (float)m_DirectionalShadowMapPass[0]->GetTargetFramebuffer()->GetWidth();
			glm::vec4 shadowOrigin = (shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution / 2.0f;
			glm::vec4 roundedOrigin = glm::round(shadowOrigin);
			glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * 2.0f / ShadowMapResolution;
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;

			lightOrthoMatrix[3] += roundOffset;

			// Store split distance and matrix in cascade
			cascades[i].SplitDepth = (nearClip + splitDist * clipRange) * -1.0f;
			cascades[i].ViewProj = lightOrthoMatrix * lightViewMatrix;
			cascades[i].View = lightViewMatrix;

			lastSplitDist = m_ShadowCascadeSplits[0];
		}
	}

	void SceneRenderer::UpdateStatistics()
	{
		m_Statistics.DrawCalls = 0;
		m_Statistics.Instances = 0;
		m_Statistics.Meshes = 0;

		for (auto& [mk, dc] : m_SelectedStaticMeshDrawList)
		{
			m_Statistics.Instances += dc.InstanceCount;
			m_Statistics.DrawCalls++;
			m_Statistics.Meshes++;
		}

		for (auto& [mk, dc] : m_StaticMeshDrawList)
		{
			m_Statistics.Instances += dc.InstanceCount;
			m_Statistics.DrawCalls++;
			m_Statistics.Meshes++;
		}

		for (auto& [mk, dc] : m_SelectedMeshDrawList)
		{
			m_Statistics.Instances += dc.InstanceCount;
			m_Statistics.DrawCalls++;
			m_Statistics.Meshes++;
		}

		for (auto& [mk, dc] : m_DrawList)
		{
			m_Statistics.Instances += dc.InstanceCount;
			m_Statistics.DrawCalls++;
			m_Statistics.Meshes++;
		}

		m_Statistics.SavedDraws = m_Statistics.Instances - m_Statistics.DrawCalls;

		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		m_Statistics.TotalGPUTime = m_CommandBuffer->GetExecutionGPUTime(frameIndex);
	}

	void SceneRenderer::SetLineWidth(const float width)
	{
		m_LineWidth = width;

		if (m_GeometryWireframePass)
			m_GeometryWireframePass->GetPipeline()->GetSpecification().LineWidth = width;
		if (m_GeometryWireframeOnTopPass)
			m_GeometryWireframeOnTopPass->GetPipeline()->GetSpecification().LineWidth = width;
		if (m_GeometryWireframeAnimPass)
			m_GeometryWireframeAnimPass->GetPipeline()->GetSpecification().LineWidth = width;
		if (m_GeometryWireframeOnTopAnimPass)
			m_GeometryWireframeOnTopAnimPass->GetPipeline()->GetSpecification().LineWidth = width;
	}

}
