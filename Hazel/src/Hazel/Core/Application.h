#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/TimeStep.h"
#include "Hazel/Core/Timer.h"
#include "Hazel/Core/Window.h"
#include "Hazel/Core/LayerStack.h"
#include "Hazel/Renderer/RendererConfig.h"

#include "Hazel/Core/ApplicationSettings.h"
#include "Hazel/Core/Events/ApplicationEvent.h"
#include "Hazel/Core/RenderThread.h"

#include "Hazel/ImGui/ImGuiLayer.h"
#include <deque>

namespace Hazel {

	struct ApplicationSpecification
	{
		std::string Name = "Hazel";
		uint32_t WindowWidth = 1600, WindowHeight = 900;
		bool WindowDecorated = false;
		bool Fullscreen = false;
		bool VSync = true;
		std::string WorkingDirectory;
		bool StartMaximized = true;
		bool Resizable = true;
		bool EnableImGui = true;
		//ScriptEngineConfig ScriptConfig;
		RendererConfig RenderConfig;
		ThreadingPolicy CoreThreadingPolicy = ThreadingPolicy::MultiThreaded;
		std::filesystem::path IconPath;
	};

	class Application
	{
		using EventCallbackFn = std::function<void(Event&)>;
	public:
		struct PerformanceTimers
		{
			float MainThreadWorkTime = 0.0f;
			float MainThreadWaitTime = 0.0f;
			float RenderThreadWorkTime = 0.0f;
			float RenderThreadWaitTime = 0.0f;
			float RenderThreadGPUWaitTime = 0.0f;

			float ScriptUpdate = 0.0f;
			float PhysicsStepTime = 0.0f;
		};
	public:
		Application(const ApplicationSpecification& specification);
		virtual ~Application();

		void Run();
		void Close();

		virtual void OnInit() {}
		virtual void OnShutdown();
		virtual void OnUpdate(Timestep ts) {}

		virtual void OnEvent(Event& event);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* layer);
		void RenderImGui();

		void AddEventCallback(const EventCallbackFn& eventCallback) { m_EventCallbacks.push_back(eventCallback); }

		void SetShowStats(bool show) { m_ShowStats = show; }

		template<typename Func>
		void QueueEvent(Func&& func)
		{
			std::scoped_lock<std::mutex> lock(m_EventQueueMutex);
			m_EventQueue.emplace_back(true, func);
		}

		// Creates & Dispatches an event either immediately, or adds it to an event queue which will be processed after the next call
		// to SyncEvents().
		// Waiting until after next sync gives the application some control over _when_ the events will be processed.
		// An example of where this is useful:
		// Suppose an asset thread is loading assets and dispatching "AssetReloaded" events.
		// We do not want those events to be processed until the asset thread has synced its assets back to the main thread.
		template<typename TEvent, bool DispatchImmediately = false, typename... TEventArgs>
		void DispatchEvent(TEventArgs&&... args)
		{
#ifndef HZ_COMPILER_GCC
			// TODO(Emily): GCC causes this to fail for AnimationGraphCompiledEvent for some reason. Investigate.
			static_assert(std::is_assignable_v<Event, TEvent>);
#endif

			std::shared_ptr<TEvent> event = std::make_shared<TEvent>(std::forward<TEventArgs>(args)...);
			if constexpr (DispatchImmediately)
			{
				OnEvent(*event);
			}
			else
			{
				std::scoped_lock<std::mutex> lock(m_EventQueueMutex);
				m_EventQueue.emplace_back(false, [event](){ Application::Get().OnEvent(*event); });
			}
		}

		// Mark all waiting events as sync'd.
		// Thus allowing them to be processed on next call to ProcessEvents()
		void SyncEvents();

		inline Window& GetWindow() { return *m_Window; }
		
		static inline Application& Get() { return *s_Instance; }

		Timestep GetTimestep() const { return m_TimeStep; }
		Timestep GetFrametime() const { return m_Frametime; }
		float GetTime() const; // TODO: This should be in "Platform"

		static std::thread::id GetMainThreadID();
		static bool IsMainThread();

		static const char* GetConfigurationName();
		static const char* GetPlatformName();

		const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		PerformanceProfiler* GetPerformanceProfiler() { return m_Profiler; }

		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		RenderThread& GetRenderThread() { return m_RenderThread; }
		uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
		const PerformanceTimers& GetPerformanceTimers() const { return m_PerformanceTimers; }
		PerformanceTimers& GetPerformanceTimers() { return m_PerformanceTimers; }
		const std::unordered_map<const char*, PerformanceProfiler::PerFrameData>& GetProfilerPreviousFrameData() const { return m_ProfilerPreviousFrameData; }

		ApplicationSettings& GetSettings() { return m_AppSettings; }
		const ApplicationSettings& GetSettings() const { return m_AppSettings; }
		
		static bool IsRuntime() { return s_IsRuntime; }
	private:
		void ProcessEvents();

		bool OnWindowResize(WindowResizeEvent& e);
		bool OnWindowMinimize(WindowMinimizeEvent& e);
		bool OnWindowClose(WindowCloseEvent& e);
	private:
		std::unique_ptr<Window> m_Window;
		ApplicationSpecification m_Specification;
		bool m_Running = true, m_Minimized = false;
		LayerStack m_LayerStack;
		ImGuiLayer* m_ImGuiLayer;
		Timestep m_Frametime;
		Timestep m_TimeStep;
		PerformanceProfiler* m_Profiler = nullptr; // TODO: Should be null in Dist
		std::unordered_map<const char*, PerformanceProfiler::PerFrameData> m_ProfilerPreviousFrameData;
		bool m_ShowStats = true;

		RenderThread m_RenderThread;

		std::mutex m_EventQueueMutex;
		std::deque<std::pair<bool, std::function<void()>>> m_EventQueue;
		std::vector<EventCallbackFn> m_EventCallbacks;

		float m_LastFrameTime = 0.0f;
		uint32_t m_CurrentFrameIndex = 0;

		PerformanceTimers m_PerformanceTimers; // TODO(Yan): remove for Dist

		ApplicationSettings m_AppSettings;

		static Application* s_Instance;

		friend class Renderer;
	protected:
		inline static bool s_IsRuntime = false;
	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);
}
