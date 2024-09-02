#pragma once

#include "Thread.h"

#include <atomic>

namespace Hazel {

	struct RenderThreadData;

	enum class ThreadingPolicy
	{
		// MultiThreaded will create a Render Thread
		None = 0, SingleThreaded, MultiThreaded
	};

	class RenderThread
	{
	public:
		enum class State
		{
			Idle = 0,
			Busy,
			Kick
		};
	public:
		RenderThread(ThreadingPolicy coreThreadingPolicy);
		~RenderThread();

		void Run();
		bool IsRunning() const { return m_IsRunning; }
		void Terminate();

		void Wait(State waitForState);
		void WaitAndSet(State waitForState, State setToState);
		void Set(State setToState);

		void NextFrame();
		void BlockUntilRenderComplete();
		void Kick();
		
		void Pump();

		static bool IsCurrentThreadRT();
	private:
		RenderThreadData* m_Data;
		ThreadingPolicy m_ThreadingPolicy;

		Thread m_RenderThread;

		bool m_IsRunning = false;

		std::atomic<uint32_t> m_AppThreadFrame = 0;
	};

}
