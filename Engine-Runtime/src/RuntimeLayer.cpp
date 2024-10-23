#include "RuntimeLayer.h"

#include "Engine/Asset/AssetManager.h"

#include "Engine/Audio/SoundObject.h"
#include "Engine/Audio/AudioEvents/AudioCommandRegistry.h"

#include "Engine/Core/Input.h"

#include "Engine/Project/Project.h"
#include "Engine/Project/ProjectSerializer.h"

#include "Engine/Renderer/Renderer.h"

#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Scene/Prefab.h"

#include "Engine/Serialization/AssetPack.h"

#include "Engine/Script/ScriptEngine.h"

#include "Engine/Tiering/TieringSerializer.h"

#include <filesystem>

namespace Hazel {

	static bool s_AudioDisabled = true;

	RuntimeLayer::RuntimeLayer(std::string_view projectPath)
		: m_EditorCamera(45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f)
		, m_ProjectPath(projectPath)
	{
	}

	RuntimeLayer::~RuntimeLayer()
	{
	}

	void RuntimeLayer::OnAttach()
	{
		OpenProject();

		SceneRendererSpecification spec;
		spec.JumpFloodPass = false; // Should always be false for runtime
		spec.NumShadowCascades = 2; // 1 for Dichotomy
		spec.EnableEdgeOutlineEffect = false; // true for Dichotomy

		//Tiering::TieringSettings ts;
		/*if (TieringSerializer::Deserialize(ts, "LD53/Config/Settings.yaml"))
			spec.Tiering = ts.RendererTS;*/

		m_SceneRenderer = Ref<SceneRenderer>::Create(m_RuntimeScene, spec);
		m_SceneRenderer->GetOptions().ShowGrid = false;
		m_SceneRenderer->SetShadowSettings(-5.0f, 5.0f, 0.92f);

		// For LD51
		/*m_SceneRenderer->SetShadowSettings(-5.0f, 5.0f, 0.93f, 0.0f);
		m_SceneRenderer->SetShadowCascades(0.14f, 0.2f, 0.3f, 1.0f);*/

		m_Renderer2D = Ref<Renderer2D>::Create();
		m_Renderer2D->SetLineWidth(2.0f);

		// Setup swapchain render pass
		FramebufferSpecification compFramebufferSpec;
		compFramebufferSpec.DebugName = "SceneComposite";
		compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
		compFramebufferSpec.SwapChainTarget = true;
		compFramebufferSpec.Attachments = { ImageFormat::RGBA };

		Ref<Framebuffer> framebuffer = Framebuffer::Create(compFramebufferSpec);

		PipelineSpecification pipelineSpecification;
		pipelineSpecification.Layout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float2, "a_TexCoord" }
		};
		pipelineSpecification.BackfaceCulling = false;
		pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("TexturePass");
		pipelineSpecification.TargetFramebuffer = framebuffer;
		pipelineSpecification.DebugName = "SceneComposite";
		pipelineSpecification.DepthWrite = false;

		RenderPassSpecification renderPassSpec;
		renderPassSpec.DebugName = "SceneComposite";
		renderPassSpec.Pipeline = Pipeline::Create(pipelineSpecification);
		m_SwapChainRP = RenderPass::Create(renderPassSpec);
		HZ_CORE_VERIFY(m_SwapChainRP->Validate());
		m_SwapChainRP->Bake();

		m_SwapChainMaterial = Material::Create(pipelineSpecification.Shader);

		m_CommandBuffer = RenderCommandBuffer::CreateFromSwapChain("RuntimeLayer");

		OnScenePlay();
	}

	void RuntimeLayer::OnDetach()
	{
		OnSceneStop();

		ScriptEngine::SetSceneContext(nullptr, nullptr);
		m_SceneRenderer->SetScene(nullptr);

		HZ_CORE_VERIFY(m_RuntimeScene->GetRefCount() == 1);
		m_RuntimeScene = nullptr;
	}

	void RuntimeLayer::OnScenePlay()
	{
		m_RuntimeScene->SetSceneTransitionCallback([this](AssetHandle handle) { QueueSceneTransition(handle); });
		ScriptEngine::SetSceneContext(m_RuntimeScene, m_SceneRenderer);
		m_RuntimeScene->OnRuntimeStart();
	}

	void RuntimeLayer::OnSceneStop()
	{
		m_RuntimeScene->OnRuntimeStop();
	}

	void RuntimeLayer::OnRender2D()
	{
		bool shouldRender = ShouldShowIntroVersion() || m_ShowDebugDisplay;
		if (!shouldRender)
			return;

		m_Renderer2D->SetTargetFramebuffer(m_SceneRenderer->GetExternalCompositeFramebuffer());
		m_Renderer2D->BeginScene(m_Renderer2DProj, glm::mat4(1.0f));


		if (m_ShowDebugDisplay)
		{
			DrawDebugStats();
			DrawFPSStats();
		}

		if (ShouldShowIntroVersion())
			DrawVersionInfo();

		m_Renderer2D->EndScene();
	}

	void RuntimeLayer::QueueSceneTransition(AssetHandle handle)
	{
		m_PostSceneUpdateQueue.push_back([this, handle]()
		{
			m_RuntimeScene->OnRuntimeStop();
			LoadScene(handle);
			m_SceneRenderer->SetScene(m_RuntimeScene);
			m_RuntimeScene->OnRuntimeStart();
		});
	}

	void RuntimeLayer::UpdateWindowTitle(const std::string& sceneName)
	{
		Application::Get().GetWindow().SetTitle(sceneName);
	}
		
	void RuntimeLayer::LoadSceneAssets()
	{
	}

#if PRELOAD
	void RuntimeLayer::LoadSceneAssets()
	{
		auto assetList = m_RuntimeScene->GetAssetList();
		for (AssetHandle handle : assetList)
		{
			const AssetMetadata& metadata = AssetManager::GetMetadata(handle);
			AssetManager::ReloadData(handle);
			if (metadata.Type == AssetType::Prefab)
			{
				Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(handle);
				LoadPrefabAssets(prefab);
			}
		}
	}

	void RuntimeLayer::LoadPrefabAssets(Ref<Prefab> prefab)
	{
		auto assetList = prefab->GetAssetList();
		for (AssetHandle handle : assetList)
		{
			const AssetMetadata& metadata = AssetManager::GetMetadata(handle);
			AssetManager::ReloadData(handle);
			if (metadata.Type == AssetType::Prefab)
			{
				Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(handle);
				LoadPrefabAssets(prefab);
			}
		}
	}
#endif

	void RuntimeLayer::LoadPrefabAssets(Ref<Prefab> prefab)
	{
	}

	void RuntimeLayer::DrawFPSStats()
	{
		float renderScale = m_SceneRenderer->GetSpecification().Tiering.RendererScale;
		float unscaledViewportWidth = m_SceneRenderer->GetViewportWidth() * (1.0f / renderScale);
		float unscaledViewportHeight = m_SceneRenderer->GetViewportHeight() * (1.0f / renderScale);

		const float fontSize = 20.0f;
		glm::vec2 pos = { unscaledViewportWidth - 250.0f, unscaledViewportHeight - 40.0f };
		DrawString(fmt::format("{} fps", m_FramesPerSecond), pos, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 20.0f);
		pos.y -= fontSize;
		DrawString(fmt::format("{:.2f} ms frame", m_FrameTime), pos, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 20.0f);
		pos.y -= fontSize;
		DrawString(fmt::format("{:.2f} ms CPU", m_CPUTime), pos, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 20.0f);
		pos.y -= fontSize;
		DrawString(fmt::format("{:.2f} ms GPU", m_GPUTime), pos, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 20.0f);
		
		{
			auto stats = Renderer::GetGPUMemoryStats();
			std::string freeGPUMem = Utils::BytesToString(stats.TotalAvailable);
			std::string usedGPUMem = Utils::BytesToString(stats.Used);
			std::string bufferMem = Utils::BytesToString(stats.BufferAllocationSize);
			std::string imageMem = Utils::BytesToString(stats.ImageAllocationSize);
			pos.y -= fontSize;
			DrawString(fmt::format("{0}/{1} GPU", usedGPUMem, freeGPUMem), pos, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 20.0f);
			pos.y -= fontSize;
			DrawString(fmt::format("{0} BUF", bufferMem), pos, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 20.0f);
			pos.y -= fontSize;
			DrawString(fmt::format("{0} IMG", imageMem), pos, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 20.0f);
		}
	}

	void RuntimeLayer::DrawDebugStats()
	{
		auto& app = Application::Get();
		
		// Add font size to this after each line
		glm::vec2 pos = { 20.0f, 50.0f };

		float fontSize = 25.0f;

		if (false)
		{
			const auto& perFrameData = app.GetProfilerPreviousFrameData();
			for (const auto& [name, time] : perFrameData)
			{
				DrawString(fmt::format("{}: {:.3f} ms ({}x)", name, time.Time, time.Samples), pos, glm::vec4(1.0f), fontSize);
				pos.y += fontSize;
			}
		}

		fontSize = 30.0f;

		DrawString(fmt::format("{:.2f} ms ({:.2f}ms GPU)", m_FrameTime, m_GPUTime), pos, glm::vec4(1.0f), fontSize);
		pos.y += fontSize;

		// Render Thread
		fontSize = 25.0f;
		DrawString(fmt::format("GPU wait {:.2f} ms", m_PerformanceTimers.RenderThreadGPUWaitTime), pos, glm::vec4(1.0f), fontSize);
		pos.y += fontSize;
		DrawString(fmt::format("MT wait {:.2f} ms", m_PerformanceTimers.RenderThreadWaitTime), pos, glm::vec4(1.0f), fontSize);
		pos.y += fontSize;
		DrawString(fmt::format("CPU-Only {:.2f} ms", m_PerformanceTimers.RenderThreadWorkTime - m_PerformanceTimers.RenderThreadGPUWaitTime), pos, glm::vec4(1.0f), fontSize);
		pos.y += fontSize;
		fontSize = 30.0f;
		DrawString(fmt::format("Render Thread {:.2f} ms", m_PerformanceTimers.RenderThreadWorkTime), pos, glm::vec4(1.0f), fontSize);
		pos.y += fontSize;

		// Main Thread
		fontSize = 25.0f;
		DrawString(fmt::format("Physics {:.2f} ms", m_PerformanceTimers.PhysicsStepTime), pos, glm::vec4(1.0f), fontSize);
		pos.y += fontSize;
		DrawString(fmt::format("Script Update {:.2f} ms", m_PerformanceTimers.ScriptUpdate), pos, glm::vec4(1.0f), fontSize);
		pos.y += fontSize;
		DrawString(fmt::format("Wait for RT {:.2f} ms", m_PerformanceTimers.MainThreadWaitTime), pos, glm::vec4(1.0f), fontSize);
		pos.y += fontSize;
		fontSize = 30.0f;
		DrawString(fmt::format("Main Thread {:.2f} ms", m_PerformanceTimers.MainThreadWorkTime), pos, glm::vec4(1.0f), fontSize);
		pos.y += fontSize;

		DrawString(fmt::format("{} fps", (uint32_t)m_FramesPerSecond), pos, glm::vec4(1.0f), fontSize);
		pos.y += fontSize;
		DrawString(fmt::format("{} entities", (uint32_t)m_RuntimeScene->GetEntityMap().size()), pos, glm::vec4(1.0f), fontSize);
		pos.y += fontSize;
		DrawString(fmt::format("{} script entities", (uint32_t)ScriptEngine::GetEntityInstances().size()), pos, glm::vec4(1.0f), fontSize);
		pos.y += fontSize;
		DrawString(fmt::format("AssetPack {}", m_AssetPack->GetBuildVersion()), pos, glm::vec4(1.0f), fontSize * 0.8f);
		pos.y += fontSize * 0.8f;
		DrawString(fmt::format("{} ({})", m_RuntimeScene->GetName(), (uint64_t)m_RuntimeScene->GetUUID()), pos, glm::vec4(1.0f), fontSize * 0.8f);
		pos.y += fontSize * 0.8f;
	}

	void RuntimeLayer::DrawVersionInfo()
	{
		float alpha = glm::clamp(4.0f - (Application::Get().GetTime() * 0.5f), 0.0f, 0.5f);
		DrawString(fmt::format("1.3.0.0 (AP {})", m_AssetPack->GetBuildVersion()), { 20.0f, 20.0f }, glm::vec4(1, 1, 1, alpha), 30.0f, false);
	}

	void RuntimeLayer::DrawString(const std::string& string, const glm::vec2& position, const glm::vec4& color, float size, bool shadow)
	{
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(size));

		// Shadow
		if (shadow)
		{
			const float shadowOffset = 1.0f;
			glm::mat4 transformShadow = glm::translate(glm::mat4(1.0f), { position.x + shadowOffset, position.y - shadowOffset, -0.2f }) * scale;
			m_Renderer2D->DrawString(string, Font::GetDefaultMonoSpacedFont(), transformShadow, 1000.0f, glm::vec4(0, 0, 0, 1));
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), { position.x, position.y, -0.21f }) * scale;
		m_Renderer2D->DrawString(string, Font::GetDefaultMonoSpacedFont(), transform, 1000.0f, color);
	}

	void RuntimeLayer::UpdateFPSStat()
	{
		auto& app = Application::Get();
		m_FramesPerSecond = static_cast<uint32_t>(1.0f / (float)app.GetFrametime());
	}

	void RuntimeLayer::UpdatePerformanceTimers()
	{
		auto& app = Application::Get();
		m_FrameTime = (float)app.GetFrametime().GetMilliseconds();
		m_PerformanceTimers = app.GetPerformanceTimers();
		m_CPUTime = m_FrameTime - m_PerformanceTimers.RenderThreadGPUWaitTime;
		m_GPUTime = m_SceneRenderer->GetStatistics().TotalGPUTime;
	}

	bool RuntimeLayer::ShouldShowIntroVersion() const
	{
		return Application::Get().GetTime() < m_IntroVersionDuration;
	}

	void RuntimeLayer::OnUpdate(Timestep ts)
	{
		m_UpdateFPSTimer -= ts;
		if (m_UpdateFPSTimer <= 0.0f)
		{
			UpdateFPSStat();
			m_UpdateFPSTimer = 1.0f;
		}

		m_UpdatePerformanceTimer -= ts;
		if (m_UpdatePerformanceTimer <= 0.0f)
		{
			UpdatePerformanceTimers();
			m_UpdatePerformanceTimer = 0.2f;
		}

		auto& app = Application::Get();

		auto [width, height] = app.GetWindow().GetSize();
		m_SceneRenderer->SetViewportSize(width, height);
		m_RuntimeScene->SetViewportSize(width, height);
		m_EditorCamera.SetViewportSize(width, height);
		m_Renderer2DProj = glm::ortho(0.0f, (float)width, 0.0f, (float)height);

		if (m_Width != width || m_Height != height)
		{
			m_Width = width;
			m_Height = height;
			m_Renderer2D->OnRecreateSwapchain();

			// Retrieve new main command buffer
			m_CommandBuffer = RenderCommandBuffer::CreateFromSwapChain("RuntimeLayer");
		}

		if (m_ViewportPanelFocused)
			m_EditorCamera.OnUpdate(ts);

		m_RuntimeScene->OnUpdateRuntime(ts);
		m_RuntimeScene->OnRenderRuntime(m_SceneRenderer, ts);

		OnRender2D();

		// Render final image to swapchain
		Ref<Image2D> finalImage = m_SceneRenderer->GetFinalPassImage();
		if (finalImage)
		{
			m_SwapChainMaterial->Set("u_Texture", finalImage);

			m_CommandBuffer->Begin();
			Renderer::BeginRenderPass(m_CommandBuffer, m_SwapChainRP);
			Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_SwapChainRP->GetPipeline(), m_SwapChainMaterial);
			Renderer::EndRenderPass(m_CommandBuffer);
			m_CommandBuffer->End();
		}
		else
		{
			// Clear render pass if no image is present
			m_CommandBuffer->Begin();
			Renderer::BeginRenderPass(m_CommandBuffer, m_SwapChainRP);
			Renderer::EndRenderPass(m_CommandBuffer);
			m_CommandBuffer->End();
		}

		auto sceneUpdateQueue = m_PostSceneUpdateQueue;
		m_PostSceneUpdateQueue.clear();
		for (auto& fn : sceneUpdateQueue)
			fn();
	}

	void RuntimeLayer::OpenProject()
	{
		Ref<Project> project = Ref<Project>::Create();
		ProjectSerializer serializer(project);
		serializer.Deserialize(m_ProjectPath);

		// Load asset pack
		m_AssetPack = AssetPack::Load(std::filesystem::path(project->GetConfig().ProjectDirectory) / project->GetConfig().AssetDirectory / "AssetPack.hap");
		//Ref<AssetPack> assetPack = AssetPack::Load("C:/Dev/Hazel/Projects/LD51/Assets/AssetPack.hap");
		//Ref<AssetPack> assetPack = AssetPack::Load("SandboxProject/Assets/AssetPack.hap");
		Project::SetActiveRuntime(project, m_AssetPack);

		//if (!s_AudioDisabled)
		//AudioCommandRegistry::Init();

		Buffer appBinary = m_AssetPack->ReadAppBinary();
		ScriptEngine::LoadAppAssemblyRuntime(appBinary);
		appBinary.Release();

		// Reset cameras
		m_EditorCamera = EditorCamera(45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f);

		// PrefabSandbox=5335834865867409016
		// AnimationTest=13221344830749982769
		// SponzaDemo=8428220783520776882
		// DOFTest=8428220783520776882
		//LoadScene(5335834865867409016);
		
		// Load Game Settings


		// LD53
		LoadScene(14327706792315080871);

		//if (!project->GetConfig().StartScene.empty())
		//	OpenScene((Project::GetAssetDirectory() / project->GetConfig().StartScene).string());
	}

	void RuntimeLayer::OpenScene(const std::string& filepath)
	{
		HZ_CORE_VERIFY(false); // Not supported in runtime
		Ref<Scene> newScene = Ref<Scene>::Create("New Scene", false);
		SceneSerializer serializer(newScene);
		serializer.Deserialize(filepath);
		m_RuntimeScene = newScene;
		LoadSceneAssets();

		std::filesystem::path path = filepath;
		//UpdateWindowTitle(path.filename().string());
		ScriptEngine::SetSceneContext(m_RuntimeScene, m_SceneRenderer);
	}

	void RuntimeLayer::LoadScene(AssetHandle sceneHandle)
	{
		Ref<Scene> scene = Project::GetRuntimeAssetManager()->LoadScene(sceneHandle);
		m_RuntimeScene = scene;
		m_RuntimeScene->SetSceneTransitionCallback([this](AssetHandle handle) { QueueSceneTransition(handle); });
		ScriptEngine::SetSceneContext(m_RuntimeScene, m_SceneRenderer);
	}

	void RuntimeLayer::OnEvent(Event& e)
	{
		m_RuntimeScene->OnEvent(e);

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(HZ_BIND_EVENT_FN(RuntimeLayer::OnKeyPressedEvent));
		dispatcher.Dispatch<MouseButtonPressedEvent>(HZ_BIND_EVENT_FN(RuntimeLayer::OnMouseButtonPressed));
	}

	bool RuntimeLayer::OnKeyPressedEvent(KeyPressedEvent& e)
	{
		switch (e.GetKeyCode())
		{
			case KeyCode::Escape:
				break;
		}

		if (e.GetRepeatCount() == 0 && Input::IsKeyDown(KeyCode::LeftControl))
		{
			switch (e.GetKeyCode())
			{
				case KeyCode::F3:
					m_ShowDebugDisplay = !m_ShowDebugDisplay;
					break;
			}
		}

		return false;
	}

	bool RuntimeLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		return false;
	}

}
