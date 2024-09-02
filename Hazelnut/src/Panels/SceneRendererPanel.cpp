#include "SceneRendererPanel.h"

#include "Hazel/Core/Application.h"
#include "Hazel/Debug/Profiler.h"
#include "Hazel/ImGui/ImGui.h"
#include "Hazel/Renderer/Renderer.h"

#include <imgui/imgui.h>

#include <format>

namespace Hazel {

	void SceneRendererPanel::OnImGuiRender(bool& isOpen)
	{
		HZ_PROFILE_FUNC();

		if (ImGui::Begin("Scene Renderer", &isOpen) && m_Context)
		{
			auto& options = m_Context->m_Options;

			ImGui::Text("Viewport Size: %d, %d", m_Context->m_ViewportWidth, m_Context->m_ViewportHeight);

			bool vsync = Application::Get().GetWindow().IsVSync();
			if (UI::Checkbox("Vsync", &vsync))
				Application::Get().GetWindow().SetVSync(vsync);

			const float headerSpacingOffset = -(ImGui::GetStyle().ItemSpacing.y + 1.0f);
			const bool shadersTreeNode = UI::PropertyGridHeader("Shaders", false);

			// TODO(Karim): Better placement of the button?
			const float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::SameLine(ImGui::GetWindowWidth() - lineHeight * 4.0f);
			if (UI::Button("Reload All"))
			{
				auto& shaders = Renderer::GetShaderLibrary()->GetShaders();
				for (auto& [name, shader] : shaders)
					shader->Reload(true);
			}
			if (shadersTreeNode)
			{
				static std::string searchedString;
				UI::Widgets::SearchWidget(searchedString);
				ImGui::Indent();
				auto& shaders = Renderer::GetShaderLibrary()->GetShaders();
				for (auto& [name, shader] : shaders)
				{
					if (!UI::IsMatchingSearch(name, searchedString))
						continue;

					ImGui::Columns(2);
					ImGui::Text(name.c_str());
					ImGui::NextColumn();
					std::string buttonName = std::format("Reload##{0}", name);
					if (ImGui::Button(buttonName.c_str()))
						shader->Reload(true);
					ImGui::Columns(1);
				}
				ImGui::Unindent();
				UI::EndTreeNode();
				UI::ShiftCursorY(18.0f);
			}
			else
				UI::ShiftCursorY(headerSpacingOffset);

			if (UI::PropertyGridHeader("Render Statistics"))
			{
				const auto& commandBuffer = m_Context->m_CommandBuffer;
				const auto& gpuTimeQueries = m_Context->m_GPUTimeQueries;

				uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
				ImGui::Text("GPU time: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex));

				ImGui::Text("Dir Shadow Map Pass: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.DirShadowMapPassQuery));
				ImGui::Text("Spot Shadow Map Pass: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.SpotShadowMapPassQuery));
				ImGui::Text("Depth Pre-Pass: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.DepthPrePassQuery));
				ImGui::Text("Hierarchical Depth: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.HierarchicalDepthQuery));
				ImGui::Text("Pre-Integration: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.PreIntegrationQuery));
				ImGui::Text("Light Culling Pass: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.LightCullingPassQuery));
				ImGui::Text("Geometry Pass: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.GeometryPassQuery));
				ImGui::Text("Pre-Convoluted Pass: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.PreConvolutionQuery));
				ImGui::Text("GTAO Pass: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.GTAOPassQuery));
				ImGui::Text("GTAO Denoise Pass: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.GTAODenoisePassQuery));
				ImGui::Text("AO Composite Pass: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.AOCompositePassQuery));
				ImGui::Text("Bloom Pass: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.BloomComputePassQuery));
				ImGui::Text("SSR Pass: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.SSRQuery));
				ImGui::Text("SSR Composite Pass: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.SSRCompositeQuery));
				ImGui::Text("Jump Flood Pass: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.JumpFloodPassQuery));
				ImGui::Text("Composite Pass: %.3fms", commandBuffer->GetExecutionGPUTime(frameIndex, gpuTimeQueries.CompositePassQuery));

				if (UI::BeginTreeNode("Pipeline Statistics"))
				{
					const PipelineStatistics& pipelineStats = commandBuffer->GetPipelineStatistics(frameIndex);
					ImGui::Text("Input Assembly Vertices: %llu", pipelineStats.InputAssemblyVertices);
					ImGui::Text("Input Assembly Primitives: %llu", pipelineStats.InputAssemblyPrimitives);
					ImGui::Text("Vertex Shader Invocations: %llu", pipelineStats.VertexShaderInvocations);
					ImGui::Text("Clipping Invocations: %llu", pipelineStats.ClippingInvocations);
					ImGui::Text("Clipping Primitives: %llu", pipelineStats.ClippingPrimitives);
					ImGui::Text("Fragment Shader Invocations: %llu", pipelineStats.FragmentShaderInvocations);
					ImGui::Text("Compute Shader Invocations: %llu", pipelineStats.ComputeShaderInvocations);
					UI::EndTreeNode();
				}

				UI::EndTreeNode();
			}
			else
				UI::ShiftCursorY(headerSpacingOffset);

			if (UI::PropertyGridHeader("Visualization"))
			{
				UI::BeginPropertyGrid();
				UI::Property("Show Light Complexity", m_Context->RendererDataUB.ShowLightComplexity);
				UI::Property("Show Shadow Cascades", m_Context->RendererDataUB.ShowCascades);
				// static int maxDrawCall = 1000;
				// UI::PropertySlider("Selected Draw", VulkanRenderer::GetSelectedDrawCall(), -1, maxDrawCall);
				// UI::Property("Max Draw Call", maxDrawCall);
				UI::EndPropertyGrid();
				UI::EndTreeNode();
			}
			else
				UI::ShiftCursorY(headerSpacingOffset);

#if 0
			if (UI::PropertyGridHeader("Edge Detection"))
			{
				const float size = ImGui::GetContentRegionAvail().x;
				UI::Image(m_Context->m_EdgeDetectionPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage(), { size, size * (0.9f / 1.6f) }, { 0, 1 }, { 1, 0 });

				UI::EndTreeNode();
			}
#endif

			if (UI::PropertyGridHeader("Ambient Occlusion"))
			{
				UI::BeginPropertyGrid();
				UI::Property("ShadowTolerance", options.AOShadowTolerance, 0.001f, 0.0f, 1.0f);
				UI::EndPropertyGrid();

				if (UI::BeginTreeNode("Ground-Truth Ambient Occlusion"))
				{
					UI::BeginPropertyGrid();
					if (UI::Property("Enable", options.EnableGTAO))
					{
						if (!options.EnableGTAO)
							*(int*)&options.ReflectionOcclusionMethod &= ~(int)ShaderDef::AOMethod::GTAO;

						// NOTE(Yan): updating shader defines causes Vulkan render pass issues, and we don't need them here any more,
						//            but probably worth looking into at some point
						// Renderer::SetGlobalMacroInShaders("__HZ_AO_METHOD", std::format("{}", ShaderDef::GetAOMethod(options.EnableGTAO)));
						// Renderer::SetGlobalMacroInShaders("__HZ_REFLECTION_OCCLUSION_METHOD", std::to_string((int)options.ReflectionOcclusionMethod));
					}
				
					UI::Property("Radius", m_Context->GTAODataCB.EffectRadius, 0.001f, 0.1f, 10000.f);
					UI::Property("Falloff Range", m_Context->GTAODataCB.EffectFalloffRange, 0.001f, 0.01f, 1.0f);
					UI::Property("Radius Multiplier", m_Context->GTAODataCB.RadiusMultiplier, 0.001f, 0.3f, 3.0f);
					UI::Property("Sample Distribution Power", m_Context->GTAODataCB.SampleDistributionPower, 0.001f, 1.0f, 3.0f);
					UI::Property("Thin Occluder Compensation", m_Context->GTAODataCB.ThinOccluderCompensation, 0.001f, 0.0f, 0.7f);
					UI::Property("Final Value Power", m_Context->GTAODataCB.FinalValuePower, 0.001f, 0.0f, 5.f);
					UI::Property("Denoise Blur Beta", m_Context->GTAODataCB.DenoiseBlurBeta, 0.001f, 0.0f, 30.f);
					UI::Property("Depth MIP Sampling Offset", m_Context->GTAODataCB.DepthMIPSamplingOffset, 0.005f, 0.0f, 30.f);
					UI::PropertySlider("Denoise Passes", options.GTAODenoisePasses, 0, 8);
					if (UI::Property("Half Resolution", m_Context->GTAODataCB.HalfRes))
						m_Context->m_NeedsResize = true;
					UI::EndPropertyGrid();

					UI::EndTreeNode();
				}
				
				UI::EndTreeNode();
			}
			else
				UI::ShiftCursorY(headerSpacingOffset);

			if (UI::PropertyGridHeader("SSR"))
			{
				auto& ssrOptions = m_Context->m_SSROptions;

				UI::BeginPropertyGrid();
				UI::Property("Enable SSR", options.EnableSSR);
				UI::Property("Enable Cone Tracing", ssrOptions.EnableConeTracing, "Enable rough reflections.");

				const static char* aoMethods[4] = { "Disabled", "Ground-Truth Ambient Occlusion", "Horizon-Based Ambient Occlusion", "All" };

				//TODO(Karim): Disable disabled methods in ImGui
				int methodIndex = ShaderDef::GetMethodIndex(options.ReflectionOcclusionMethod);
				if (UI::PropertyDropdown("Reflection Occlusion method", aoMethods, 4, methodIndex))
				{
					options.ReflectionOcclusionMethod = ShaderDef::ROMETHODS[methodIndex];
					if ((int)options.ReflectionOcclusionMethod & (int)ShaderDef::AOMethod::GTAO)
						options.EnableGTAO = true;
					Renderer::SetGlobalMacroInShaders("__HZ_REFLECTION_OCCLUSION_METHOD", std::format("{}", (int)options.ReflectionOcclusionMethod));
					Renderer::SetGlobalMacroInShaders("__HZ_AO_METHOD", std::format("{}", (int)ShaderDef::GetAOMethod(options.EnableGTAO)));
				}

				UI::Property("Brightness", ssrOptions.Brightness, 0.001f, 0.0f, 1.0f);
				UI::Property("Depth Tolerance", ssrOptions.DepthTolerance, 0.01f, 0.0f, std::numeric_limits<float>::max());
				UI::Property("Roughness Depth Tolerance", ssrOptions.RoughnessDepthTolerance, 0.33f, 0.0f, std::numeric_limits<float>::max(),
					"The higher the roughness the higher the depth tolerance.\nWorks best with cone tracing enabled.\nReduce as much as possible.");
				UI::Property("Horizontal Fade In", ssrOptions.FadeIn.x, 0.005f, 0.0f, 10.0f);
				UI::Property("Vertical Fade In", ssrOptions.FadeIn.y, 0.005f, 0.0f, 10.0f);
				UI::Property("Facing Reflections Fading", ssrOptions.FacingReflectionsFading, 0.01f, 0.0f, 2.0f);
				UI::Property("Luminance Factor", ssrOptions.LuminanceFactor, 0.001f, 0.0f, 2.0f, "Can break energy conservation law!");
				UI::PropertySlider("Maximum Steps", ssrOptions.MaxSteps, 1, 200);
				if (UI::Property("Half Resolution", ssrOptions.HalfRes))
					m_Context->m_NeedsResize = true;
				UI::EndPropertyGrid();

				if (UI::BeginTreeNode("Debug Views", false))
				{
					if (m_Context->m_ResourcesCreated)
					{
						const float size = ImGui::GetContentRegionAvail().x;
						UI::Image(m_Context->m_SSRImage, { size, size * (0.9f / 1.6f) }, { 0, 1 }, { 1, 0 });
						static int32_t mip = 0;
						UI::PropertySlider("Pre-convoluted Mip", mip, 0, (int)m_Context->m_PreConvolutedTexture.Texture->GetMipLevelCount() - 1);
						UI::ImageMip(m_Context->m_PreConvolutedTexture.Texture->GetImage(), mip, { size, size * (0.9f / 1.6f) }, { 0, 1 }, { 1, 0 });

					}
					UI::EndTreeNode();
				}
				UI::EndTreeNode();
			}
			else
				UI::ShiftCursorY(headerSpacingOffset);

			if (UI::PropertyGridHeader("Bloom Settings"))
			{
				auto& bloomSettings = m_Context->m_BloomSettings;

				UI::BeginPropertyGrid();
				UI::Property("Bloom Enabled", bloomSettings.Enabled);
				UI::Property("Threshold", bloomSettings.Threshold);
				UI::Property("Knee", bloomSettings.Knee);
				UI::Property("Upsample Scale", bloomSettings.UpsampleScale);
				UI::Property("Intensity", bloomSettings.Intensity, 0.05f, 0.0f, 20.0f);
				UI::Property("Dirt Intensity", bloomSettings.DirtIntensity, 0.05f, 0.0f, 20.0f);

				// TODO(Yan): move this to somewhere else
				UI::Image(m_Context->m_BloomDirtTexture, ImVec2(64, 64));
				if (ImGui::IsItemHovered())
				{
					if (ImGui::IsItemClicked())
					{
						std::string filename = FileSystem::OpenFileDialog().string();
						if (!filename.empty())
						{
							m_Context->m_BloomDirtTexture = Texture2D::Create(TextureSpecification(), filename);
							m_Context->m_CompositePass->SetInput("u_BloomDirtTexture", m_Context->m_BloomDirtTexture);
						}
					}
				}

				UI::EndPropertyGrid();
				UI::EndTreeNode();
			}
			else
				UI::ShiftCursorY(headerSpacingOffset);

			if (UI::PropertyGridHeader("Depth of Field"))
			{
				auto& dofSettings = m_Context->m_DOFSettings;

				UI::BeginPropertyGrid();
				UI::Property("DOF Enabled", dofSettings.Enabled);
				if (UI::Property("Focus Distance", dofSettings.FocusDistance, 0.1f, 0.0f, std::numeric_limits<float>::max()))
					dofSettings.FocusDistance = glm::max(dofSettings.FocusDistance, 0.0f);
				if (UI::Property("Blur Size", dofSettings.BlurSize, 0.025f, 0.0f, 20.0f))
					dofSettings.BlurSize = glm::clamp(dofSettings.BlurSize, 0.0f, 20.0f);
				UI::EndPropertyGrid();

				UI::EndTreeNode();
			}

			if (UI::PropertyGridHeader("Shadows"))
			{
				auto& rendererDataUB = m_Context->RendererDataUB;

				UI::BeginPropertyGrid();
				UI::Property("Soft Shadows", rendererDataUB.SoftShadows);
				UI::Property("DirLight Size", rendererDataUB.Range, 0.01f);
				UI::Property("Max Shadow Distance", rendererDataUB.MaxShadowDistance, 1.0f);
				UI::Property("Shadow Fade", rendererDataUB.ShadowFade, 5.0f);
				UI::EndPropertyGrid();
				if (UI::BeginTreeNode("Cascade Settings"))
				{
					UI::BeginPropertyGrid();
					UI::Property("Cascade Fading", rendererDataUB.CascadeFading);
					UI::Property("Cascade Transition Fade", rendererDataUB.CascadeTransitionFade, 0.05f, 0.0f, FLT_MAX);
					UI::Property("Cascade Split", m_Context->CascadeSplitLambda, 0.01f);
					UI::Property("CascadeNearPlaneOffset", m_Context->CascadeNearPlaneOffset, 0.1f, -FLT_MAX, 0.0f);
					UI::Property("CascadeFarPlaneOffset", m_Context->CascadeFarPlaneOffset, 0.1f, 0.0f, FLT_MAX);
					UI::Property("ScaleShadowToOrigin", m_Context->m_ScaleShadowCascadesToOrigin, 0.025f, 0.0f, 1.0f);
					UI::Property("Use manual cascade splits", m_Context->m_UseManualCascadeSplits);
					if (m_Context->m_UseManualCascadeSplits)
					{
						UI::Property("Cascade 0", m_Context->m_ShadowCascadeSplits[0], 0.025f, 0.0f);
						UI::Property("Cascade 1", m_Context->m_ShadowCascadeSplits[1], 0.025f, 0.0f);
						UI::Property("Cascade 2", m_Context->m_ShadowCascadeSplits[2], 0.025f, 0.0f);
						UI::Property("Cascade 3", m_Context->m_ShadowCascadeSplits[3], 0.025f, 0.0f);

					}
					UI::EndPropertyGrid();
					UI::EndTreeNode();
				}
				if (UI::BeginTreeNode("Shadow Map", false))
				{
					static int cascadeIndex = 0;
					auto fb = m_Context->m_DirectionalShadowMapPass[cascadeIndex]->GetTargetFramebuffer();
					auto image = fb->GetDepthImage();

					float size = ImGui::GetContentRegionAvail().x; // (float)fb->GetWidth() * 0.5f, (float)fb->GetHeight() * 0.5f
					UI::BeginPropertyGrid();
					UI::PropertySlider("Cascade Index", cascadeIndex, 0, 3);
					UI::EndPropertyGrid();
					if (m_Context->m_ResourcesCreated)
					{
						UI::Image(image, (uint32_t)cascadeIndex, { size, size }, { 0, 1 }, { 1, 0 });
					}
					UI::EndTreeNode();
				}

				if (UI::BeginTreeNode("Spotlight Shadow Map", false))
				{
					auto fb = m_Context->m_SpotShadowPass->GetTargetFramebuffer();
					auto image = fb->GetDepthImage();

					float size = ImGui::GetContentRegionAvail().x; // (float)fb->GetWidth() * 0.5f, (float)fb->GetHeight() * 0.5f
					if (m_Context->m_ResourcesCreated)
					{
						UI::Image(image, { size, size }, { 0, 1 }, { 1, 0 });
					}
					UI::EndTreeNode();
				}

				UI::EndTreeNode();
			}
			else
				UI::ShiftCursorY(headerSpacingOffset);

#if 0
			if (UI::BeginTreeNode("Compute Bloom"))
			{
				float size = ImGui::GetContentRegionAvailWidth();
				if (m_ResourcesCreated)
				{
					static int tex = 0;
					UI::PropertySlider("Texture", tex, 0, 2);
					static int mip = 0;
					auto [mipWidth, mipHeight] = m_BloomComputeTextures[tex]->GetMipSize(mip);
					std::string label = std::format("Mip ({0}x{1})", mipWidth, mipHeight);
					UI::PropertySlider(label.c_str(), mip, 0, m_BloomComputeTextures[tex]->GetMipLevelCount() - 1);
					UI::ImageMip(m_BloomComputeTextures[tex]->GetImage(), mip, { size, size * (1.0f / m_BloomComputeTextures[tex]->GetImage()->GetAspectRatio()) }, { 0, 1 }, { 1, 0 });
				}
				UI::EndTreeNode();
			}
#endif

			if (UI::BeginTreeNode("Pipeline Resources", false))
			{
				if (m_Context->m_ResourcesCreated)
				{
					const float size = ImGui::GetContentRegionAvail().x;
					static int32_t mip = 0;
					UI::PropertySlider("HZB Mip", mip, 0, (int)m_Context->m_HierarchicalDepthTexture.Texture->GetMipLevelCount() - 1);
					UI::ImageMip(m_Context->m_HierarchicalDepthTexture.Texture->GetImage(), mip, { size, size * (0.9f / 1.6f) }, { 0, 1 }, { 1, 0 });

				}
				UI::EndTreeNode();
			}

		}

		ImGui::End();
	}

}
